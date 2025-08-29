//
// Created by Tonz on 04.08.2025.
//

#include "scene.h"

#include "../engine/engine.h"
#include <imgui/imgui.h>


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
            .pSetLayouts = &*skyDescriptorSetLayout_
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

void Scene::initDescriptorSetLayout() {
    skyDescriptorSetLayout_ = vk::raii::DescriptorSetLayout(VkUtils::getDevice(),skyLayoutInfo);
    isDescSetLayoutInitialized_ = true;
}