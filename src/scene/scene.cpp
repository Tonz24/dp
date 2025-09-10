//
// Created by Tonz on 04.08.2025.
//

#include "scene.h"

#include "../engine/engine.h"
#include <imgui/imgui.h>

#include "../engine/renderers/renderer.h"


Scene::Scene(const std::vector<std::shared_ptr<Mesh>>&& meshes, std::shared_ptr<Camera> camera, std::shared_ptr<Texture> sky): meshes_(std::move(meshes)), camera_(std::move(camera)), sky_(std::move(sky)) {

    initDescriptorSet();
    initTLAS();

    if (!meshes_.empty())
        selectedObject_ = meshes_[0];
}

bool Scene::drawGUI() {

    if (ImGui::CollapsingHeader("Scene")) {
        ImGui::Indent();
        ImGui::Text("Selected mesh: ");
        ImGui::SameLine();

        if (selectedObject_ != nullptr) {
            ImGui::Text(selectedObject_->getResourceName().c_str());
            selectedObject_->drawGUI();
        }

        ImGui::Unindent();
    }

    return false;
}

void Scene::initDescriptorSet() {
    if (sky_) {
        VkUtils::BufferAlloc stagingBuffer = VkUtils::createBufferVMA(sky_->getTotalSize(),vk::BufferUsageFlagBits::eTransferSrc,VkUtils::stagingAllocFlagsVMA);
        sky_->stage(stagingBuffer);
        VkUtils::destroyBufferVMA(std::move(stagingBuffer));

        vk::DescriptorSetAllocateInfo allocInfo{
            .descriptorPool = Engine::getInstance().getDescriptorPool(),
            .descriptorSetCount = 1,
            .pSetLayouts = &*Renderer::getDescSetLayoutSky()
        };
        skyDescriptorSet_ = std::move(VkUtils::getDevice().allocateDescriptorSets(allocInfo).front());

        vk::DescriptorImageInfo descInfo{
            .sampler = sky_->getVkSampler(),
            .imageView = sky_->getVkImageView(),
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };

        vk::WriteDescriptorSet writeDescriptorSet{
            .dstSet = skyDescriptorSet_,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &descInfo
        };

        VkUtils::getDevice().updateDescriptorSets(writeDescriptorSet,{});
    }
}

void Scene::initTLAS() {

    std::vector<vk::AccelerationStructureInstanceKHR> instances{};
    instances.reserve(meshes_.size());

    for (const auto & mesh : meshes_)
        instances.emplace_back(mesh->getBLASInstance());

    vk::DeviceSize instanceBufferSize = meshes_.size() * sizeof(vk::AccelerationStructureInstanceKHR);

    //  gather mesh BLAS instance infos into a staging buffer
    VkUtils::BufferAlloc stagingBuffer = VkUtils::createBufferVMA(instanceBufferSize,vk::BufferUsageFlagBits::eTransferSrc,VkUtils::stagingAllocFlagsVMA);
    VkUtils::BufferAlloc instanceBuffer = VkUtils::createBufferVMA(instanceBufferSize,vk::BufferUsageFlagBits::eTransferDst | VkUtils::accelStructInputFlags);
    memcpy(stagingBuffer.allocationInfo.pMappedData,instances.data(),instanceBufferSize);

    //  Copy the staging buffer into the TLAS input buffer
    VkUtils::copyBuffer(stagingBuffer,instanceBuffer,instanceBufferSize);
    VkUtils::destroyBufferVMA(std::move(stagingBuffer));


    vk::AccelerationStructureGeometryInstancesDataKHR instanceData;
    instanceData.setArrayOfPointers(false);
    instanceData.setData(instanceBuffer.deviceAddress);

    vk::AccelerationStructureGeometryKHR instanceGeometry;
    instanceGeometry.setGeometryType(vk::GeometryTypeKHR::eInstances);
    instanceGeometry.setGeometry({instanceData});
    instanceGeometry.setFlags(vk::GeometryFlagBitsKHR::eOpaque);

    tlas_ = AccelerationStructure(vk::AccelerationStructureTypeKHR::eTopLevel,instanceGeometry,instances.size());
    VkUtils::destroyBufferVMA(std::move(instanceBuffer));
}
