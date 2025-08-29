//
// Created by Tonz on 14.08.2025.
//

#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "vmaUsage.h"

class VkUtils {
public:
    struct BufferAlloc {
        vk::Buffer buffer{nullptr};
        VmaAllocation allocation{};
        VmaAllocationInfo allocationInfo;
    };

    struct ImageAlloc    {
        vk::Image image{nullptr};
        VmaAllocation allocation{};
        VmaAllocationInfo allocationInfo;
    };

    enum class QueueType : uint8_t {
        graphics = 0,
        present = 1,
        transfer = 2
    };

    VkUtils() = delete;

    static BufferAlloc createBufferVMA(vk::DeviceSize bufferSize, vk::BufferUsageFlags bufferUsage, VmaAllocationCreateFlags allocationFlags = {});
    static void destroyBufferVMA(BufferAlloc&& buffer);
    static void mapMemory(const BufferAlloc& buffer, void*& ptr);
    static void unmapMemory(const BufferAlloc& buffer);


    static ImageAlloc createImageVMA(const vk::ImageCreateInfo& imageInfo, VmaAllocationCreateFlags allocationFlags = {});
    static void destroyImageVMA(ImageAlloc&& image);


    static void copyBuffer(const BufferAlloc& srcBuffer, const BufferAlloc& dstBuffer, vk::DeviceSize size);
    static void copyBuffer(const BufferAlloc& srcBuffer, const BufferAlloc& dstBuffer, const vk::BufferCopy& region);


    static void copyBufferToImage(const BufferAlloc& buffer, const ImageAlloc& image, uint32_t width, uint32_t height, vk::raii::CommandBuffer& cmdBuf);
    static void copyImageToBuffer(const ImageAlloc& image, const BufferAlloc& buffer, int32_t offsetX, uint32_t width, int32_t offsetY, uint32_t height, vk::raii::CommandBuffer& cmdBuf);

    /**
     * @brief allocates and returns a new command buffer
     * @return newly created command buffer
     */
    static vk::raii::CommandBuffer beginSingleTimeCommand();

    /**
     * @brief submits the provided command buffer and waits on its execution
     * @param cmdBuf command buffer to submit
     * @param queueType where to submit the command buffer
     */
    static void endSingleTimeCommand(const vk::raii::CommandBuffer& cmdBuf, QueueType queueType);


    /**
     * 
     * @param image image to transition
     * @param oldLayout old layout of the image
     * @param newLayout desired layout of the image
     * @param srcStageMask in what stage the last write took place before the transition
     * @param srcAccessMask how the resource was accessed before the transition
     * @param dstStageMask what stage will work with this image after the transition
     * @param dstAccessMask how the resource will be accessed after the transition
     * @param imageAspectFlags
     * @param cmdBuf command buffer where the transition is recorded
     */
    static void transitionImageLayout(const vk::Image &image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                                      vk::PipelineStageFlags2 srcStageMask, vk::AccessFlags2 srcAccessMask,
                                      vk::PipelineStageFlags2 dstStageMask, vk::AccessFlags2 dstAccessMask,
                                      vk::ImageAspectFlags imageAspectFlags, vk::raii::CommandBuffer &cmdBuf);

    static constexpr VmaAllocationCreateFlags stagingAllocFlagsVMA{VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT};

    static const vk::raii::Device& getDevice() {return *device_;}

private:
    friend class Engine;

    static void init(const vk::raii::Device* device, const vk::raii::PhysicalDevice* physicalDevice, const vk::raii::Instance* instance, const std::vector<const vk::raii::Queue*>&& queueHandles, const vk::
                     raii::CommandPool* commandPool);

    static void destroy();

    inline static const vk::raii::Device* device_{};
    inline static const vk::raii::PhysicalDevice* physicalDevice_{};
    inline static const vk::raii::Instance* instance_{};
    inline static vk::PhysicalDeviceMemoryProperties memoryProperties_{};
    inline static std::vector<const vk::raii::Queue*> queueHandles_{};
    inline static const vk::raii::CommandPool* commandPool_{};

    inline static VmaAllocator allocator_{};
};






