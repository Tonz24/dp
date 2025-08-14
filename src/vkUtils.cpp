//
// Created by Tonz on 14.08.2025.
//

#include "vkUtils.h"
#include <iostream>

void VkUtils::createBuffer(vk::DeviceSize size, vk::raii::Buffer &buffer, vk::BufferUsageFlags bufferUsage, vk::raii::DeviceMemory &bufferMemory, vk::MemoryPropertyFlags properties){
    vk::BufferCreateInfo bufferInfo{
        .size = size,
        .usage = bufferUsage,
        .sharingMode = vk::SharingMode::eExclusive
    };
    buffer = vk::raii::Buffer(*device_, bufferInfo);

    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
    };

    bufferMemory = vk::raii::DeviceMemory(*device_, allocInfo);
    buffer.bindMemory(*bufferMemory, 0);

}

uint32_t VkUtils::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    for (uint32_t i = 0; i < memoryProperties_.memoryTypeCount; ++i) {
        if (typeFilter & (1 << i) && (memoryProperties_.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    std::cerr << "ERROR: Failed to find suitable memory type!";
    exit(EXIT_FAILURE);
}

void VkUtils::init(const vk::raii::Device* device, const vk::raii::PhysicalDevice* physicalDevice) {
    device_ = device;
    physicalDevice_ = physicalDevice;

    memoryProperties_ = physicalDevice->getMemoryProperties();
}


vk::raii::CommandBuffer VkUtils::beginSingleTimeCommand(const vk::raii::CommandPool& commandPool) {
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool =  commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };
    vk::raii::CommandBuffer commandBuffer = std::move(device_->allocateCommandBuffers(allocInfo).front());

    vk::CommandBufferBeginInfo beginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };

    commandBuffer.begin(beginInfo);
    return commandBuffer;
}

void VkUtils::endSingleTimeCommand(const vk::raii::CommandBuffer& cmdBuf, const vk::Queue& queue) {
    cmdBuf.end();

    vk::SubmitInfo submitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers = &*cmdBuf,
    };
    queue.submit(submitInfo, nullptr);
    queue.waitIdle();
}

void VkUtils::transitionImageLayout(const vk::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::PipelineStageFlags2 srcStageMask,
                                    vk::AccessFlags2 srcAccessMask, vk::PipelineStageFlags2 dstStageMask, vk::AccessFlags2 dstAccessMask, vk::raii::CommandBuffer& cmdBuf) {


    vk::ImageMemoryBarrier2 barrier{
        .srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,
        .dstStageMask = dstStageMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = image,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    vk::DependencyInfo dependencyInfo{
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    };

    cmdBuf.pipelineBarrier2(dependencyInfo);
}
