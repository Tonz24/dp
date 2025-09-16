//
// Created by Tonz on 04.08.2025.
//

#pragma once

#include <vector>
#include "mesh.h"
#include "camera.h"
#include "../engine/iDrawGui.h"
#include "../engine/managers/resourceManager.h"

class Scene : public IDrawGui {
public:

    explicit Scene(const std::vector<std::shared_ptr<Mesh>>&& meshes, std::shared_ptr<Camera> camera, std::shared_ptr<Texture> sky = {nullptr});

    [[nodiscard]] Camera& getCamera() const { return *camera_; }
    void setCamera(std::shared_ptr<Camera> camera) { std::swap(camera_, camera);}

    [[nodiscard]] const std::vector<std::shared_ptr<Mesh>>& getMeshes() const { return meshes_; }
    //void setMeshes(std::vector<std::shared_ptr<Mesh>> models) { meshes_ = std::move(models); }

    bool drawGUI() override;

    void setSelectedObject(uint32_t objectCId) { selectedObject_ = MeshManager::getInstance()->getResource(objectCId); }
    void setSelectedObject(std::shared_ptr<Mesh> object) { selectedObject_ = std::move(object);}

    void setSky(std::shared_ptr<Texture> newSky) {
        sky_ = std::move(newSky);
        initDescriptorSet();
    }

    [[nodiscard]] const std::shared_ptr<Texture>& getSky() const { return sky_; }

private:

    void initDescriptorSet() const;
    void initTLAS();

    std::vector<std::shared_ptr<Mesh>> meshes_{};
    std::shared_ptr<Camera> camera_{};
    std::shared_ptr<Texture> sky_{};

    std::shared_ptr<Mesh> selectedObject_{};

    AccelerationStructure tlas_{};
};

