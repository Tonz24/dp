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

    bool drawGUI() override;

    void setSelectedObject(uint32_t objectCId) { selectedObject_ = MeshManager::getInstance()->getResource(objectCId); }
    void setSelectedObject(std::shared_ptr<Mesh> object) { selectedObject_ = std::move(object);}

    void setSky(std::shared_ptr<Texture> newSky) {
        sky_ = std::move(newSky);
        initDescriptorSet();
    }

    [[nodiscard]] const std::shared_ptr<Texture>& getSky() const { return sky_; }

private:

    struct alignas(16) TrianglePacked {
        glm::vec4 v0;
        glm::vec4 v1;
        glm::vec4 v2;
        float area{0.0f};
        float pad[3]{0,0,0};
    };

    struct alignas(4) CDFElement {
        uint32_t triIndex;
        float pdf;
    };

    void initDescriptorSet() const;
    void initTLAS();
    void extractEmissiveMeshes();

    std::vector<std::shared_ptr<Mesh>> meshes_{};
    std::shared_ptr<Camera> camera_{};
    std::shared_ptr<Texture> sky_{};
    std::vector<CDFElement> cdf_{};
    std::vector<TrianglePacked> emissiveTriangles_{};

    VkUtils::BufferAlloc emissiveBuffer_{};
    VkUtils::BufferAlloc cdfBuffer_{};


    std::shared_ptr<Mesh> selectedObject_{};

    AccelerationStructure tlas_{};
};

