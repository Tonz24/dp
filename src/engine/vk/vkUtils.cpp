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

    if (bufferUsage & vk::BufferUsageFlagBits::eShaderDeviceAddress)
        bufferAlloc.deviceAddress = getDevice().getBufferAddress({.buffer =  bufferAlloc.buffer});
    return bufferAlloc;
}

VkUtils::BufferAlloc VkUtils::createBufferVMA(vk::DeviceSize bufferSize, vk::BufferUsageFlags bufferUsage, vk::DeviceSize alignment,
    VmaAllocationCreateFlags allocationFlags) {
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
    vk::Result createResult = static_cast<vk::Result>(vmaCreateBufferWithAlignment(allocator_,&*bufferInfo,&allocInfo,alignment,reinterpret_cast<VkBuffer*>(&bufferAlloc.buffer) ,&bufferAlloc.allocation,&bufferAlloc.allocationInfo));

    if (createResult != vk::Result::eSuccess)
        throw std::runtime_error("ERROR: failed to create buffer!");

    if (bufferUsage & vk::BufferUsageFlagBits::eShaderDeviceAddress)
        bufferAlloc.deviceAddress = getDevice().getBufferAddress({.buffer =  bufferAlloc.buffer});

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

void VkUtils::destroyImageVMA(ImageAlloc&& image) {
    if (image.image && image.allocation)
        vmaDestroyImage(allocator_,image.image,image.allocation);

    image.image = nullptr;
    image.allocation = nullptr;
    image.allocationInfo = {};
}

void VkUtils::mapMemory(const BufferAlloc& buffer, void*& ptr) {
    vmaMapMemory(allocator_,buffer.allocation,&ptr);
}

void VkUtils::unmapMemory(const BufferAlloc& buffer) {
    vmaUnmapMemory(allocator_,buffer.allocation);
}

void VkUtils::destroyBufferVMA(BufferAlloc&& buffer) {
    if (buffer.buffer && buffer.allocation)
        vmaDestroyBuffer(allocator_,buffer.buffer,buffer.allocation);

    buffer.buffer = nullptr;
    buffer.allocation = nullptr;
    buffer.allocationInfo = {};
    buffer.deviceAddress = 0;
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
        .flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT | VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
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
                                    vk::ImageAspectFlags imageAspectFlags, vk::raii::CommandBuffer& cmdBuf, TransitionMipInfo mipInfo) {

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
            .baseMipLevel = mipInfo.baseLevel,
            .levelCount = mipInfo.levelCount,
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

void VkUtils::blit(const vk::raii::CommandBuffer& cmdBuf, const vk::Image& srcImage, const glm::vec<2, int32_t>& srcSize,
    vk::ImageAspectFlags srcAspect, const vk::Image& dstImage, const glm::vec<2, int32_t>& dstSize, vk::ImageAspectFlags dstAspect,
    vk::Filter filter) {

    std::array srcOffsets{
        vk::Offset3D{
            .x = 0,
            .y = 0,
            .z = 0
        },
        vk::Offset3D{
            .x = srcSize.x,
            .y = srcSize.y,
            .z = 1
        }
    };

    std::array dstOffsets{
        vk::Offset3D{
            .x = 0,
            .y = 0,
            .z = 0
        },
        vk::Offset3D{
            .x = dstSize.x,
            .y = dstSize.y,
            .z = 1
        }
    };

    vk::ImageBlit blitRegion{
        .srcSubresource = {
            .aspectMask = srcAspect,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .srcOffsets =  srcOffsets,
        .dstSubresource = {
            .aspectMask = dstAspect,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .dstOffsets =  dstOffsets
    };
    cmdBuf.blitImage(srcImage,
                     vk::ImageLayout::eTransferSrcOptimal,
                     dstImage,
                     vk::ImageLayout::eTransferDstOptimal,
                     blitRegion,
                     filter);
}

void VkUtils::blit(const vk::raii::CommandBuffer& cmdBuf, const vk::Image& srcImage, const vk::Image& dstImage, vk::ImageBlit blitRegion,
    vk::Filter filter) {

    cmdBuf.blitImage(srcImage,
                     vk::ImageLayout::eTransferSrcOptimal,
                     dstImage,
                     vk::ImageLayout::eTransferDstOptimal,
                     blitRegion,
                     filter);
}
