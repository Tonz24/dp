//
// Created by Tonz on 19.09.2025.
//

#pragma once
#include "graphicsPipeline.h"

// https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/#raytracingpipeline
class RaytracingPipeline : public GraphicsPipeline{
public:
    RaytracingPipeline(const std::vector<ShaderStageInfo>& shaderInfos, const std::span<const vk::DescriptorSetLayout>& descriptorSetLayouts,
        const std::span<const vk::PushConstantRange>& pcsRange);

    RaytracingPipeline() = default;

private:
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups_{};

    void initShaderGroups(const std::vector<ShaderStageInfo>& shaderInfos);
};
