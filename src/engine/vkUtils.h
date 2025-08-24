//
// Created by Tonz on 14.08.2025.
//

#pragma once

#include <vulkan/vulkan_raii.hpp>
#include "../entry/vmaUsage.h"

class VkUtils {
public:

    struct BufferWithMemory {
        vk::raii::Buffer buffer{nullptr};
        vk::raii::DeviceMemory memory{nullptr};
    };

    enum class QueueType : uint8_t {
        graphics = 0,
        present = 1,
        transfer = 2
    };

    VkUtils() = delete;

    /**
     * @brief configures and creates a buffer for given vk::Buffer and vk::DeviceMemory objects
     * @param bufferSize size of the buffer (in bytes)
     * @param buffer buffer object
     * @param bufferUsage what kind of buffer to create
     * @param bufferMemory memory object for the buffer
     * @param properties desired memory properties of the buffer
     */
    static void createBuffer(vk::DeviceSize bufferSize, vk::raii::Buffer& buffer, vk::BufferUsageFlags bufferUsage, vk::raii::DeviceMemory& bufferMemory, vk::MemoryPropertyFlags properties);

    /**
     * @brief configures and creates a buffer, returns it alongside its memory
     * @param bufferSize size of the buffer (in bytes)
     * @param bufferUsage what kind of buffer to create
     * @param properties desired memory properties of the buffer
     * @return BufferWithMemory struct containing vk::Buffer and vk::DeviceMemory objects of the newly created buffer
     */
    static BufferWithMemory createBuffer(vk::DeviceSize bufferSize, vk::BufferUsageFlags bufferUsage, vk::MemoryPropertyFlags properties);

    /**
     * @brief copies source buffer to dst buffer
     * @param srcBuffer source buffer (from)
     * @param dstBuffer destination buffer (to)
     * @param size size of data to copy
     */
    static void copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size);

    /**
     * @brief copies source buffer to dst buffer
     * @param srcBuffer source buffer (from)
     * @param dstBuffer destination buffer (to)
     * @param region offsets and size of data
     */
    static void copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::BufferCopy region);

    /**
     * @brief Copies a buffer to image
     * @param buffer buffer to copy from
     * @param image image to copy to
     * @param width width of the image
     * @param height height of the image
     * @param cmdBuf
     */
    static void copyBufferToImage(const vk::raii::Buffer &buffer, vk::raii::Image &image, uint32_t width, uint32_t height, vk::raii::CommandBuffer& cmdBuf);

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


    static uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

    static constexpr vk::MemoryPropertyFlags stagingMemoryFlags{vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent};

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




