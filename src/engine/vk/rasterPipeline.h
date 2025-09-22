//
// Created by Tonz on 25.08.2025.
//

#pragma once
#include "graphicsPipeline.h"


class RasterPipeline : public GraphicsPipeline {
public:

    RasterPipeline(const std::vector<ShaderStageInfo>& shaderInfos, std::span<const vk::DescriptorSetLayout> descriptorSetLayouts,
                     std::span<const vk::PushConstantRange> pcsRange, std::span<const vk::Format> colorAttachmentFormats, bool hasVertexLayout,
                     vk::Format depthFormat = vk::Format::eUndefined);

    RasterPipeline() = default;

protected:

    static constexpr std::array dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    //  create the dynamic state info structure
    static constexpr vk::PipelineDynamicStateCreateInfo dynamicState{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    //  specify how to assembly input vertex data into primitive shapes (TRIANGLE LIST)
    static constexpr vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        .topology = vk::PrimitiveTopology::eTriangleList
    };

    //  set viewports and scissor regions as nullptr - will be submitted dynamically when recording the command buffer
    static constexpr vk::PipelineViewportStateCreateInfo viewportState{
        .viewportCount = 1,
        .pViewports = nullptr,
        .scissorCount = 1,
        .pScissors = nullptr
    };

    //  set rasterizer settings (leave be for now)
    static constexpr vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eNone,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = vk::False,
        .depthBiasSlopeFactor = 1.0f,
        .lineWidth = 1.0f
    };

    //  set multisampling settings (leave be for now)
    static constexpr vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False
    };

};