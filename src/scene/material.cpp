//
// Created by Tonz on 28.07.2025.
//

#include "material.h"

#include "../engine/engine.h"

void Material::recordDescriptorSet() {

    std::vector<vk::WriteDescriptorSet> descriptorWrites{};
    for (uint32_t i = 0; i < textures_.size(); ++i) {
        if (textures_[i]) {

            vk::DescriptorImageInfo imageInfo{
                .sampler = textures_[i]->getVkSampler(),
                .imageView = textures_[i]->getVkImageView(),
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
            };

            vk::WriteDescriptorSet writeDescriptorSet{
                .dstSet = descriptorSet_, //  which descriptor to update
                .dstBinding = i, // which binding to update
                .dstArrayElement = 0, //  what element the update starts at
                .descriptorCount = 1, //  how many descriptors are affected
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo = &imageInfo,
            };

            descriptorWrites.emplace_back(writeDescriptorSet);
        }
    }

    Engine::getInstance().getDevice().updateDescriptorSets(descriptorWrites,{});
}

void Material::allocateDescriptorSet() {
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = Engine::getInstance().getDescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &*Engine::getInstance().getDescriptorSetLayoutMaterial(),
    };
    std::cout << "h" << std::endl;

    auto h = Engine::getInstance().getDevice().allocateDescriptorSets(allocInfo);

    descriptorSet_ = std::move(h.front());
}
