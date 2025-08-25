//
// Created by Tonz on 14.08.2025.
//

#include "vkUtils.h"
#include <iostream>
#include <ranges>

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

VkUtils::BufferAlloc VkUtils::createBufferVMA(vk::DeviceSize bufferSize, vk::BufferUsageFlags bufferUsage, VmaAllocationCreateFlags allocationFlags) {
    vk::BufferCreateInfo bufferInfo{
        .size = bufferSize,
        .usage =  bufferUsage,
        .sharingMode = vk::SharingMode::eExclusive
    };

    VmaAllocationCreateInfo allocInfo{
        .flags = allocationFlags,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    BufferAlloc bufferAlloc;
    vk::Result createResult = static_cast<vk::Result>(vmaCreateBuffer(allocator_,&*bufferInfo,&allocInfo,reinterpret_cast<VkBuffer*>(&bufferAlloc.buffer) ,&bufferAlloc.allocation,&bufferAlloc.allocationInfo));

    if (createResult != vk::Result::eSuccess)
        throw std::runtime_error("ERROR: failed to create buffer!");

    return bufferAlloc;
}

VkUtils::ImageAlloc VkUtils::createImageVMA(const vk::ImageCreateInfo& imageInfo, VmaAllocationCreateFlags allocationFlags) {


    VmaAllocationCreateInfo allocInfo{
        .flags = allocationFlags,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    ImageAlloc imageAlloc;
    vk::Result createResult = static_cast<vk::Result>(vmaCreateImage(allocator_,&*imageInfo,&allocInfo,reinterpret_cast<VkImage*>(&imageAlloc.image),&imageAlloc.allocation,&imageAlloc.allocationInfo));

    if (createResult != vk::Result::eSuccess)
        throw std::runtime_error("ERROR: failed to create buffer!");

    return imageAlloc;
}

void VkUtils::destroyImageVMA(const ImageAlloc& image) {
    vmaDestroyImage(allocator_,image.image,image.allocation);
}

void VkUtils::mapMemory(const BufferAlloc& buffer, void*& ptr) {
    vmaMapMemory(allocator_,buffer.allocation,&ptr);
}

void VkUtils::unmapMemory(const BufferAlloc& buffer) {
    vmaUnmapMemory(allocator_,buffer.allocation);
}

void VkUtils::destroyBufferVMA(const BufferAlloc& buffer) {
    vmaDestroyBuffer(allocator_,buffer.buffer,buffer.allocation);
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



void VkUtils::copyBuffer(const vk::raii::Buffer& srcBuffer, const vk::raii::Buffer& dstBuffer, vk::DeviceSize size) {

    vk::BufferCopy region{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };

    copyBuffer(srcBuffer,dstBuffer, region);
}


void VkUtils::copyBuffer(const vk::raii::Buffer& srcBuffer, const vk::raii::Buffer& dstBuffer, const vk::BufferCopy& region) {
    auto cmdBuf = VkUtils::beginSingleTimeCommand();

    cmdBuf.copyBuffer(srcBuffer,dstBuffer,region);

    endSingleTimeCommand(cmdBuf,VkUtils::QueueType::graphics);
}

void VkUtils::copyBuffer(const BufferAlloc& srcBuffer, const BufferAlloc& dstBuffer, vk::DeviceSize size) {
    vk::BufferCopy region{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };

    copyBuffer(srcBuffer,dstBuffer, region);
}

void VkUtils::copyBuffer(const BufferAlloc& srcBuffer, const BufferAlloc& dstBuffer, const vk::BufferCopy& region) {
    auto cmdBuf = beginSingleTimeCommand();

    cmdBuf.copyBuffer(srcBuffer.buffer,dstBuffer.buffer,region);

    endSingleTimeCommand(cmdBuf,QueueType::graphics);
}

void VkUtils::copyBufferToImage(const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height, vk::raii::CommandBuffer& cmdBuf) {

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
}

void VkUtils::copyBufferToImage(const vk::Buffer& buffer, vk::Image& image, uint32_t width, uint32_t height, vk::raii::CommandBuffer& cmdBuf) {
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
}


uint32_t VkUtils::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    for (uint32_t i = 0; i < memoryProperties_.memoryTypeCount; ++i) {
        if (typeFilter & (1 << i) && (memoryProperties_.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    throw std::runtime_error("ERROR: Failed to find suitable memory type!");
}

void VkUtils::init(const vk::raii::Device* device, const vk::raii::PhysicalDevice* physicalDevice, const vk::raii::Instance* instance, const std::vector<const vk::raii::Queue*>&& queueHandles, const vk::
                   raii::CommandPool* commandPool) {
    device_ = device;
    physicalDevice_ = physicalDevice;
    memoryProperties_ = physicalDevice->getMemoryProperties();
    queueHandles_ = queueHandles;
    commandPool_ = commandPool;
    instance_ = instance;

    VmaVulkanFunctions vulkanFunctions{
        .vkGetInstanceProcAddr = &vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = &vkGetDeviceProcAddr
    };

    VmaAllocatorCreateInfo allocatorCreateInfo{
        .flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT,
        .physicalDevice = **physicalDevice_,
        .device = **device,
        .pVulkanFunctions = &vulkanFunctions,
        .instance = **instance_,
        .vulkanApiVersion = VK_API_VERSION_1_4,
    };

    vmaCreateAllocator(&allocatorCreateInfo,&allocator_);

}

void VkUtils::destroy() {
    vmaDestroyAllocator(allocator_);
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
