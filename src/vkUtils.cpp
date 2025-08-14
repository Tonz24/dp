//
// Created by Tonz on 14.08.2025.
//

#include "vkUtils.h"
#include <iostream>

void VkUtils::createBuffer(vk::DeviceSize bufferSize, vk::raii::Buffer &buffer, vk::BufferUsageFlags bufferUsage, vk::raii::DeviceMemory &bufferMemory, vk::MemoryPropertyFlags properties){
    vk::BufferCreateInfo bufferInfo{
        .size = bufferSize,
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

VkUtils::BufferWithMemory VkUtils::createBuffer(vk::DeviceSize bufferSize, vk::BufferUsageFlags bufferUsage, vk::MemoryPropertyFlags properties) {
    vk::raii::Buffer buffer{nullptr};
    vk::raii::DeviceMemory bufferMemory{nullptr};

    vk::BufferCreateInfo bufferInfo{
        .size = bufferSize,
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

    return BufferWithMemory{
        .buffer = std::move(buffer),
        .memory = std::move(bufferMemory)
    };
}



void VkUtils::copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size) {

    vk::BufferCopy region{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };

    copyBuffer(srcBuffer,dstBuffer, region);
}


void VkUtils::copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::BufferCopy region) {
    auto cmdBuf = VkUtils::beginSingleTimeCommand();

    cmdBuf.copyBuffer(srcBuffer,dstBuffer,region);

    endSingleTimeCommand(cmdBuf,VkUtils::QueueType::graphics);
}

void VkUtils::copyBufferToImage(const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height) {

    auto cmdBuf = beginSingleTimeCommand();

    vk::BufferImageCopy region{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageOffset = {
            .x = 0,
            .y = 0,
            .z = 0
        },
        .imageExtent = {
            .width = width,
            .height = height,
            .depth = 1
        }
    };

    cmdBuf.copyBufferToImage(buffer,image,vk::ImageLayout::eTransferDstOptimal,region);
    endSingleTimeCommand(cmdBuf,QueueType::graphics);
}


uint32_t VkUtils::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    for (uint32_t i = 0; i < memoryProperties_.memoryTypeCount; ++i) {
        if (typeFilter & (1 << i) && (memoryProperties_.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    std::cerr << "ERROR: Failed to find suitable memory type!";
    exit(EXIT_FAILURE);
}

void VkUtils::init(const vk::raii::Device* device, const vk::raii::PhysicalDevice* physicalDevice, const std::vector<const vk::raii::Queue*>&& queueHandles, const vk::
                   raii::CommandPool* commandPool) {
    device_ = device;
    physicalDevice_ = physicalDevice;
    memoryProperties_ = physicalDevice->getMemoryProperties();
    queueHandles_ = queueHandles;
    commandPool_ = commandPool;
}


vk::raii::CommandBuffer VkUtils::beginSingleTimeCommand() {
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool =  *commandPool_,
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

void VkUtils::endSingleTimeCommand(const vk::raii::CommandBuffer& cmdBuf, QueueType queueType) {
    cmdBuf.end();

    vk::SubmitInfo submitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers = &*cmdBuf,
    };
    const auto queueHandle = queueHandles_[static_cast<int>(queueType)];

    queueHandle->submit(submitInfo, nullptr);
    queueHandle->waitIdle();
}

void VkUtils::transitionImageLayout(const vk::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::PipelineStageFlags2 srcStageMask,
                                    vk::AccessFlags2 srcAccessMask, vk::PipelineStageFlags2 dstStageMask, vk::AccessFlags2 dstAccessMask, vk::ImageAspectFlags imageAspectFlags, vk::raii::CommandBuffer& cmdBuf) {



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
            .aspectMask = imageAspectFlags,
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
