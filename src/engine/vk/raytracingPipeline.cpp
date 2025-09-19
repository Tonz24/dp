//
// Created by Tonz on 19.09.2025.
//

#include "raytracingPipeline.h"

RaytracingPipeline::RaytracingPipeline(const std::vector<ShaderStageInfo>& shaderInfos,
    const std::span<const vk::DescriptorSetLayout>& descriptorSetLayouts, const std::span<const vk::PushConstantRange>& pcsRange,
    const std::span<const vk::Format>& colorAttachmentFormats, bool hasVertexLayout, vk::Format depthFormat) : GraphicsPipeline()
{
    initShaderStages(shaderInfos);



}
