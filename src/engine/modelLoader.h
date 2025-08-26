//
// Created by Tonz on 28.07.2025.
//

#pragma once
#include <string>
#include <memory>
#include <mutex>
#include <assimp/scene.h>

#include "../scene/material.h"
#include "../scene/mesh.h"

class ModelLoader {
public:
    static std::vector<std::shared_ptr<Mesh<Vertex3D>>> loadModel(std::string_view path, bool multithread = true);

private:
    static void loadMaterials(const std::string& directory, const aiScene& scene, uint32_t startIndex, uint32_t materialCount,
                              std::vector<std::shared_ptr<Material> >& materials);

    inline static bool gammaCorrectOnLoad{false};

    inline static const std::string modelPathPrefix{"../assets/models/"};

    static constexpr std::array textureTypes = {aiTextureType_SPECULAR, aiTextureType_SHININESS, aiTextureType_NORMALS, aiTextureType_DIFFUSE, };
    static constexpr std::array slots = {Material::TextureMapSlot::specularMapSlot,Material::TextureMapSlot::shininessMapSlot,Material::TextureMapSlot::normalMapSlot, Material::TextureMapSlot::diffuseMapSlot,};

    inline static std::mutex materialVectorMutex_{};
};