//
// Created by Tonz on 06.08.2025.
//

#pragma once
#include <vector>

#include "mesh.h"
#include "../transform.h"

//TODO: move memory management from per mesh to per Model and then later to central management
class Model {
public:
    explicit Model(const std::vector<std::shared_ptr<Mesh>> &meshes) : meshes_(meshes) {}

    [[nodiscard]] const std::vector<std::shared_ptr<Mesh>>& getMeshes() const { return meshes_; }
    void setMeshes(std::vector<std::shared_ptr<Mesh>> meshes) { meshes_ = std::move(meshes); }

private:
    Transform transform_;

    std::vector<std::shared_ptr<Mesh>> meshes_{};

    vk::raii::DeviceMemory bufferMemory{nullptr};

};