//
// Created by Tonz on 19.09.2025.
//

#include "raytracingPipeline.h"

#include <ranges>

RaytracingPipeline::RaytracingPipeline(const std::vector<ShaderStageInfo>& shaderInfos, const std::span<const vk::DescriptorSetLayout>& descriptorSetLayouts,
                                       const std::span<const vk::PushConstantRange>& pcsRange)
: GraphicsPipeline(descriptorSetLayouts, pcsRange)
{
    initShaderStages(shaderInfos);
    initShaderGroups(shaderInfos);

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
        .pSetLayouts =  descriptorSetLayouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(pcsRange.size()),
        .pPushConstantRanges = pcsRange.data()
    };
    pipelineLayout_ = vk::raii::PipelineLayout( VkUtils::getDevice(), pipelineLayoutInfo);

    vk::RayTracingPipelineCreateInfoKHR rtPipelineInfo{
        .stageCount = static_cast<uint32_t>(shaderStages_.size()),
        .pStages = shaderStages_.data(),
        .groupCount  = static_cast<uint32_t>(shaderGroups_.size()),
        .pGroups =   shaderGroups_.data(),
        .maxPipelineRayRecursionDepth = 10,
        .layout = pipelineLayout_,
        .basePipelineHandle = nullptr,
        .basePipelineIndex = -1,
    };

    pipeline_ = VkUtils::getDevice().createRayTracingPipelineKHR(nullptr,nullptr,rtPipelineInfo);
}

void RaytracingPipeline::initShaderGroups(const std::vector<ShaderStageInfo>& shaderInfos) {

    //for (const auto & shaderInfo : shaderInfos) {
    for (auto  [i, shaderInfo]: std::views::enumerate(shaderInfos) | std::views::as_const) {
        vk::RayTracingShaderGroupCreateInfoKHR groupInfo{};

        // Miss and raygen shaders have type eGeneral
        if (shaderInfo.stage == vk::ShaderStageFlagBits::eRaygenKHR || shaderInfo.stage == vk::ShaderStageFlagBits::eMissKHR) {
            groupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
            groupInfo.generalShader = i;
        }
        else { // closest hit shader has type eTrianglesHitGroup (no procedural geometry is used, so eProceduralHitGroup is never used)
            groupInfo.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
            groupInfo.closestHitShader = i;
        }
        shaderGroups_.emplace_back(groupInfo);
    }
}