//
// Created by Tonz on 28.07.2025.
//

#include "modelLoader.h"
#include <iostream>

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

#include "../managers/materialManager.h"
#include "../managers/meshManager.h"
#include "../managers/textureManager.h"

std::vector<std::shared_ptr<Mesh>> ModelLoader::loadModel(std::string_view path) {
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

    {
        const auto materials = loadMaterials(directory, *scene);

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

            uint32_t matIndex = mesh->mMaterialIndex;
            auto material = materials.at(mesh->mMaterialIndex);


            std::shared_ptr<Mesh> parsedMesh = MeshManager::getInstance()->getResource(mesh->mName.C_Str());

            if (parsedMesh == nullptr)
                parsedMesh = MeshManager::getInstance()->createResource(mesh->mName.C_Str(),std::move(vertices),std::move(indices),std::move(material));

            meshes.emplace_back(parsedMesh);
        }
    }
    return meshes;
}

std::vector<std::shared_ptr<Material>> ModelLoader::loadMaterials(const std::string &directory, const aiScene &scene) {

    std::vector<std::shared_ptr<Material>> loadedMaterials{};

    for (uint32_t i = 0; i < scene.mNumMaterials; ++i){
        const aiMaterial* const assimpMaterial = scene.mMaterials[i];

        aiString name;

        //skip material_ if it doesn't have a name
        if (assimpMaterial->Get(AI_MATKEY_NAME, name) != aiReturn_SUCCESS)
            continue;

        auto mat = MaterialManager::getInstance()->getResource(name.C_Str());


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
        //texture
        aiString texName;

        if (assimpMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), texName) == AI_SUCCESS){
            std::string fullName{ directory +  std::string{texName.C_Str()} };

            const auto texHandle = TextureManager::getInstance()->getResource(fullName);

            if (ModelLoader::gammaCorrectOnLoad)
                texHandle->expand();

            mat->setTexture(texHandle,Material::TextureMapSlot::DIFFUSE_MAP_SLOT);
        }


        if (assimpMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_SPECULAR, 0), texName) == AI_SUCCESS) {
            std::string fullName{ directory +  std::string{texName.C_Str()} };

            const auto texHandle = TextureManager::getInstance()->getResource(fullName);

            if (ModelLoader::gammaCorrectOnLoad)
                texHandle->expand();

            mat->setTexture(texHandle,Material::TextureMapSlot::SPECULAR_MAP_SLOT);
        }


        if (assimpMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_SHININESS, 0), texName) == AI_SUCCESS) {
            std::string fullName{ directory + std::string{texName.C_Str()} };

            const auto texHandle = TextureManager::getInstance()->getResource(fullName);
            mat->setTexture(texHandle,Material::TextureMapSlot::SHININESS_MAP_SLOT);

        }

        if (assimpMaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), texName) == AI_SUCCESS) {
            std::string fullName{ directory + std::string{texName.C_Str()} };

            const auto texHandle = TextureManager::getInstance()->getResource(fullName);
            mat->setTexture(texHandle,Material::TextureMapSlot::NORMAL_MAP_SLOT);
        }
        //====================================================

        mat->recordDescriptorSet();
        loadedMaterials.emplace_back(mat);
    }
    return loadedMaterials;
}
