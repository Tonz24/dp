//
// Created by Tonz on 28.07.2025.
//

#include "modelLoader.h"
#include <iostream>
#include <thread>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>


#include "utils.h"

#include "managers/resourceManager.h"

std::vector<std::shared_ptr<Mesh>> ModelLoader::loadModel(std::string_view path, bool multithread) {
    std::string fullPath{ModelLoader::modelPathPrefix + std::string{path}};

    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(fullPath.c_str(),
                                             aiProcess_Triangulate |
                                             //aiProcess_GenSmoothNormals |
                                             aiProcess_JoinIdenticalVertices |
                                             aiProcess_OptimizeGraph |
                                             aiProcess_OptimizeMeshes |
                                             aiProcess_CalcTangentSpace);

    if (scene == nullptr){
        std::cerr << importer.GetErrorString() << std::endl;
        exit(EXIT_FAILURE);
    }

    auto lastSlashPos = std::string{ fullPath }.find_last_of('/');
    std::string directory{fullPath};
    directory = directory.substr(0, lastSlashPos + 1);


    std::vector<std::shared_ptr<Mesh>> meshes{};
    std::vector<std::shared_ptr<Material>> materials{};

    uint32_t workerCount = std::thread::hardware_concurrency();
    uint32_t numMaterials =  scene->mNumMaterials;
    uint32_t chunkSize = numMaterials / workerCount;

    std::vector<std::thread> workers{};
    workers.reserve(workerCount);


    if (multithread) {
        for (int i = 0; i < workerCount; ++i) {
            std::thread thread(loadMaterials,directory,std::ref(*scene), i* chunkSize, chunkSize, std::ref(materials));
            workers.emplace_back(std::move(thread));
        }
        for (auto& worker : workers) {
            worker.join();
        }
    }
    else
        loadMaterials(directory, *scene, 0, scene->mNumMaterials, materials);

    //  in mesh
    uint32_t stagingBufferSize{0};

    for (uint32_t i = 0; i < scene->mNumMeshes; ++i) {
        const auto& mesh = scene->mMeshes[i];

        //  load vertices
        std::vector<Vertex> vertices{};
        vertices.reserve(mesh->mNumVertices);
        for (uint32_t j = 0; j < mesh->mNumVertices; ++j) {
            aiVector3D position_{}, normal_{}, tangent_{}, texCoord_{};
            glm::vec3 position{}, normal{}, tangent{};
            glm::vec2 texCoord{};

            if (mesh->mVertices) {
                position_ = mesh->mVertices[j];
                position = {position_.x, position_.y, position_.z};
            }
            if (mesh->mNormals) {
                normal_ = mesh->mNormals[j];
                normal = {normal_.x, normal_.y, normal_.z};
            }
            if (mesh->mTangents) {
                tangent_ = mesh->mTangents[j];
                tangent = {tangent_.x, tangent_.y, tangent_.z};
            }
            if (mesh->mTextureCoords[0]) {
                texCoord_ = mesh->mTextureCoords[0][j];
                texCoord = {texCoord_.x, texCoord_.y};
            }
            vertices.emplace_back(position, normal, tangent, texCoord);
        }

        //  load indices
        std::vector<uint32_t> indices{};
        indices.reserve(mesh->mNumFaces);
        for (uint32_t j = 0; j < mesh->mNumFaces; ++j) {
            const auto& face = mesh->mFaces[j];

            for (uint32_t k = 0; k < face.mNumIndices; ++k)
                indices.push_back(face.mIndices[k]);
        }

        //load material
        std::shared_ptr<Material> material = materials.at(mesh->mMaterialIndex);

        std::shared_ptr<Mesh> parsedMesh = MeshManager::getInstance()->getResource(mesh->mName.C_Str());
        if (!parsedMesh) {
            auto mm = new Mesh(std::move(vertices),std::move(indices),material);
            parsedMesh = MeshManager::getInstance()->registerResource(mm,mesh->mName.C_Str());
        }

        //  if this mesh is larger than current largest mesh, update the value so that the staging buffer can later contain all the data
        uint32_t verticesSize = parsedMesh->getVertices().size() * sizeof(parsedMesh->getVertices()[0]);
        uint32_t indicesSize = parsedMesh->getIndices().size() * sizeof(parsedMesh->getIndices()[0]);
        if (verticesSize > stagingBufferSize)
            stagingBufferSize = verticesSize;
        if (indicesSize > stagingBufferSize)
            stagingBufferSize = indicesSize;


        meshes.emplace_back(parsedMesh);
    }

    VkUtils::BufferAlloc stagingBuffer = VkUtils::createBufferVMA(stagingBufferSize,vk::BufferUsageFlagBits::eTransferSrc,VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    void* data{nullptr};
    VkUtils::mapMemory(stagingBuffer,data);

    for (auto& mesh : meshes) {
        mesh->stage(stagingBuffer, data);
    }

    // unmap, destroy staging buffer
    VkUtils::unmapMemory(stagingBuffer);
    VkUtils::destroyBuffer(stagingBuffer);

    return meshes;
}

void ModelLoader::loadMaterials(const std::string& directory, const aiScene& scene, uint32_t startIndex, uint32_t materialCount,
                                std::vector<std::shared_ptr<Material> >& materials) {


    for (uint32_t i = startIndex; i < startIndex + materialCount && i < scene.mNumMaterials; ++i){
        const aiMaterial* const assimpMaterial = scene.mMaterials[i];

        aiString name;

        //skip material_ if it doesn't have a name
        if (assimpMaterial->Get(AI_MATKEY_NAME, name) != aiReturn_SUCCESS)
            continue;


        std::shared_ptr<Material> mat = MaterialManager::getInstance()->getResource(name.C_Str());

        if (!mat) {
            auto mm = new Material();
            mat = MaterialManager::getInstance()->registerResource(mm,name.C_Str());
        }

        //====================================================
        //gamma correctable values first
        if (aiColor3D aiDiffuse{}; assimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiDiffuse) == aiReturn_SUCCESS) {
            mat->setDiffuseAlbedo({ aiDiffuse.r,aiDiffuse.g,aiDiffuse.b });
            if (ModelLoader::gammaCorrectOnLoad)
                mat->setDiffuseAlbedo(Utils::expand(mat->getDiffuseAlbedo()));
        }

        if (aiColor3D aiSpecular{}; assimpMaterial->Get(AI_MATKEY_COLOR_SPECULAR, aiSpecular) == aiReturn_SUCCESS) {
            mat->setSpecularAlbedo({ aiSpecular.r,aiSpecular.g,aiSpecular.b });
            if (ModelLoader::gammaCorrectOnLoad)
                mat->setSpecularAlbedo(Utils::expand(mat->getSpecularAlbedo()));
        }
        //====================================================

        //====================================================
        //remaining material_ attributes
        if (aiColor3D aiEmissive{}; assimpMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, aiEmissive) == aiReturn_SUCCESS)
            mat->setEmission({ aiEmissive.r,aiEmissive.g,aiEmissive.b });

        if (float aiShininess{}; assimpMaterial->Get(AI_MATKEY_SHININESS, aiShininess) == aiReturn_SUCCESS)
            mat->setShininess(aiShininess);

        if (float aiIOR{}; assimpMaterial->Get(AI_MATKEY_REFRACTI, aiIOR) == aiReturn_SUCCESS)
            mat->setIor(aiIOR);

        if (aiColor3D aiAttenuation{}; assimpMaterial->Get(AI_MATKEY_COLOR_TRANSPARENT, aiAttenuation) == aiReturn_SUCCESS)
            mat->setAttenuation({ aiAttenuation.r, aiAttenuation.g, aiAttenuation.b });

        //====================================================

        //====================================================
        //textures
        aiString texName;
        for (uint32_t j = 0; j < textureTypes.size(); ++j) {
            auto texType = ModelLoader::textureTypes[j];
            auto slot = ModelLoader::slots[j];

            if (assimpMaterial->Get(AI_MATKEY_TEXTURE(texType, 0), texName) == AI_SUCCESS){

                std::string fullName{ directory + std::string{texName.C_Str()}};
                std::shared_ptr<Texture> t = TextureManager::getInstance()->getResource(fullName);

                if (!t) {
                    auto tex = new Texture(fullName);
                    t  = TextureManager::getInstance()->registerResource(tex,fullName);
                }

                mat->setTexture(t,slot);
            }
        }

        mat->recordDescriptorSet();
        mat->updateUBONow();

        {
            std::lock_guard<std::mutex> lockGuard(materialVectorMutex_);
            materials.emplace_back(mat);
        }
    }
}
