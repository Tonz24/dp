//
// Created by Tonz on 14.08.2025.
//

#include "vkUtils.h"
#include <iostream>
#include <ranges>

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

void VkUtils::copyBufferToImage(const BufferAlloc& buffer, const ImageAlloc& image, uint32_t width, uint32_t height, vk::raii::CommandBuffer& cmdBuf) {
    vk::BufferImageCopy region {
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

    cmdBuf.copyBufferToImage(buffer.buffer,image.image,vk::ImageLayout::eTransferDstOptimal,region);
}

void VkUtils::copyImageToBuffer(const ImageAlloc& image, const BufferAlloc& buffer, int32_t offsetX, uint32_t width,
                                int32_t offsetY, uint32_t height, vk::raii::CommandBuffer& cmdBuf)
{
    vk::BufferImageCopy region {
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
            .x = offsetX,
            .y = offsetY,
            .z = 0
        },
        .imageExtent = {
            .width = width,
            .height = height,
            .depth = 1
        }
    };

    cmdBuf.copyImageToBuffer(image.image,vk::ImageLayout::eTransferSrcOptimal,buffer.buffer,region);
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
                                    vk::AccessFlags2 srcAccessMask, vk::PipelineStageFlags2 dstStageMask, vk::AccessFlags2 dstAccessMask,
                                    vk::ImageAspectFlags imageAspectFlags, vk::raii::CommandBuffer& cmdBuf) {

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
