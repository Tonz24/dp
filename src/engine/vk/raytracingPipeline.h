//
// Created by Tonz on 19.09.2025.
//

#pragma once
#include "graphicsPipeline.h"
#include "graphicsPipeline.h"

// https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/#raytracingpipeline
// https://github.com/yknishidate/single-file-vulkan-pathtracing/blob/master/main.cpp
class RaytracingPipeline : public GraphicsPipeline{
public:
    RaytracingPipeline(const std::vector<ShaderStageInfo>& shaderInfos, const std::span<const vk::DescriptorSetLayout>& descriptorSetLayouts,
        const std::span<const vk::PushConstantRange>& pcsRange);

    RaytracingPipeline() = default;

    [[nodiscard]] const VkUtils::BufferAlloc& getSBTBuffer() const { return sbtBuffer_; }
    [[nodiscard]] const vk::StridedDeviceAddressRegionKHR& getRaygenRegion() const { return raygenRegion_; }
    [[nodiscard]] const vk::StridedDeviceAddressRegionKHR& getMissRegion() const { return missRegion_; }
    [[nodiscard]] const vk::StridedDeviceAddressRegionKHR& getHitRegion() const { return hitRegion_; }

private:
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups_{};

    uint32_t hitCount_{0}, missCount_{0};

    VkUtils::BufferAlloc sbtBuffer_{};

    vk::StridedDeviceAddressRegionKHR raygenRegion_{};
    vk::StridedDeviceAddressRegionKHR missRegion_{};
    vk::StridedDeviceAddressRegionKHR hitRegion_{};

    void initShaderGroups(const std::vector<ShaderStageInfo>& shaderInfos);
    void initSBT();
    void extractGroupCounts(const ShaderStageInfo& stageInfo);

    static uint32_t alignUp(uint32_t size, uint32_t alignment);
};
