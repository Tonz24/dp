//
// Created by Tonz on 04.08.2025.
//

#pragma once

#include <vector>
#include "mesh.h"
#include "camera.h"
#include "../engine/iDrawGui.h"

class Scene : public IDrawGui {
public:

    explicit Scene(const std::vector<std::shared_ptr<Mesh>>& meshes, const std::shared_ptr<Camera> &camera) : meshes_(meshes), camera_(camera) {

    }

    [[nodiscard]] Camera& getCamera() const { return *camera_; }
    void setCamera(std::shared_ptr<Camera> camera) { std::swap(camera_, camera);}

    [[nodiscard]] const std::vector<std::shared_ptr<Mesh>>& getMeshes() const { return meshes_; }
    void setMeshes(std::vector<std::shared_ptr<Mesh>> models) { meshes_ = std::move(models); }

    bool drawGUI() override;

private:
    std::vector<std::shared_ptr<Mesh>> meshes_{};
    std::shared_ptr<Camera> camera_{};

    uint32_t selectedObjectIndex_{0};
};
