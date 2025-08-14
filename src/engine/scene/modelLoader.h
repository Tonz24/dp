//
// Created by Tonz on 28.07.2025.
//

#pragma once
#include <string>
#include <memory>
#include <assimp/scene.h>

#include "../material.h"
#include "mesh.h"

class ModelLoader {
public:
    static std::vector<std::shared_ptr<Mesh>> loadModel(std::string_view path);

private:
    static std::vector<std::shared_ptr<Material>> loadMaterials(const std::string &directory, const aiScene& scene);

    inline static bool gammaCorrectOnLoad{true};

    inline static const std::string modelPathPrefix{"../assets/models/"};
};