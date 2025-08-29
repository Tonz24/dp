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

    explicit Scene(const std::vector<std::shared_ptr<Mesh>>&& meshes, std::shared_ptr<Camera> camera, std::shared_ptr<Texture> sky = {nullptr})
        : meshes_(std::move(meshes)), camera_(std::move(camera)), sky_(std::move(sky)) {

        if (!isDescSetLayoutInitialized_)
            initDescriptorSetLayout();
        initDescriptorSet();

        if (!meshes_.empty())
            selectedObject_ = meshes_[0];
    }

    [[nodiscard]] Camera& getCamera() const { return *camera_; }
    void setCamera(std::shared_ptr<Camera> camera) { std::swap(camera_, camera);}

    [[nodiscard]] const std::vector<std::shared_ptr<Mesh>>& getMeshes() const { return meshes_; }
    void setMeshes(std::vector<std::shared_ptr<Mesh>> models) { meshes_ = std::move(models); }

    bool drawGUI() override;

    void setSelectedObject(uint32_t objectCId) { selectedObject_ = MeshManager::getInstance()->getResource(objectCId); }
    void setSelectedObject(std::shared_ptr<Mesh> object) { selectedObject_ = std::move(object);}

    void setSky(std::shared_ptr<Texture> newSky) {
        sky_ = std::move(newSky);
        initDescriptorSet();
    }

    const vk::raii::DescriptorSet& getSkyDescriptorSet() const { return skyDescriptorSet_;}

    static void initDescriptorSetLayout();
    static const vk::raii::DescriptorSetLayout& getDescriptorSetLayout() {return skyDescriptorSetLayout_;}

private:

    void initDescriptorSet();

    std::vector<std::shared_ptr<Mesh>> meshes_{};
    std::shared_ptr<Camera> camera_{};
    std::shared_ptr<Texture> sky_{};

    std::shared_ptr<Mesh> selectedObject_{};

    vk::raii::DescriptorSet skyDescriptorSet_{nullptr};


    static constexpr vk::DescriptorSetLayoutBinding skyBinding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
    };

    static constexpr vk::DescriptorSetLayoutCreateInfo skyLayoutInfo{
        .bindingCount = 1,
        .pBindings = &skyBinding
    };

    static inline vk::raii::DescriptorSetLayout skyDescriptorSetLayout_{nullptr};
    static inline bool isDescSetLayoutInitialized_{false};
};
