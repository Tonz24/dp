//
// Created by Tonz on 14.08.2025.
//

#pragma once

#include <vulkan/vulkan_raii.hpp>

class VkUtils {
public:

    static void createBuffer(vk::DeviceSize size, vk::raii::Buffer& buffer, vk::BufferUsageFlags bufferUsage, vk::raii::DeviceMemory& bufferMemory, vk::MemoryPropertyFlags properties);
    static void copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size);

    static vk::raii::CommandBuffer beginSingleTimeCommand(const vk::raii::CommandPool& commandPool);
    static void endSingleTimeCommand(const vk::raii::CommandBuffer& cmdBuf, const vk::Queue& queue);


    /**
     * 
     * @param image image to transition
     * @param oldLayout old layout of the image
     * @param newLayout desired layout of the image
     * @param srcStageMask in what stage the last write took place before the transition
     * @param srcAccessMask how the resource was accessed before the transition
     * @param dstStageMask what stage will work with this image after the transition
     * @param dstAccessMask how the resource will be accessed after the transition
     * @param cmdBuf command buffer where the transition will be recorded
     */
    static void transitionImageLayout(const vk::Image &image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                                      vk::PipelineStageFlags2 srcStageMask, vk::AccessFlags2 srcAccessMask,
                                      vk::PipelineStageFlags2 dstStageMask, vk::AccessFlags2 dstAccessMask,
                                      vk::raii::CommandBuffer &cmdBuf);


    static uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

private:
    friend class Engine;

    static void init(const vk::raii::Device* device, const vk::raii::PhysicalDevice* physicalDevice);

    inline static const vk::raii::Device* device_{};
    inline static const vk::raii::PhysicalDevice* physicalDevice_{};
    inline static vk::PhysicalDeviceMemoryProperties memoryProperties_{};
};




