//
// Created by Tonz on 25.08.2025.
//

#pragma once
#include <vulkan/vulkan_raii.hpp>

#include "../uboFormat.h"
#include "../../scene/vertex.h"

template<typename T>
concept VertexType = requires {
    { T::getBindingDescription() } -> std::same_as<vk::VertexInputBindingDescription>;
    { T::getAttributeDescriptions()};
};


class GraphicsPipeline {
public:
    GraphicsPipeline(std::string_view vShaderPath,std::string_view fShaderPath, const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts, const std::vector<vk::Format>& colorAttachmentFormats, vk::Format depthFormat = vk::Format::eUndefined);
    GraphicsPipeline() = default;

    [[nodiscard]] const vk::raii::Pipeline& getGraphicsPipeline() const { return graphicsPipeline_; }
    [[nodiscard]] const vk::raii::PipelineLayout& getPipelineLayout() const { return pipelineLayout_; }

private:
    vk::raii::ShaderModule vShaderModule_{nullptr};
    vk::raii::ShaderModule fShaderModule_{nullptr};

    vk::raii::Pipeline graphicsPipeline_{nullptr};
    vk::raii::PipelineLayout pipelineLayout_{nullptr};
    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo_{};
    std::array<vk::PipelineShaderStageCreateInfo,2> shaderStages_{};

    void initShaders(std::string_view vShaderPath, std::string_view fShaderPath);

    static vk::raii::ShaderModule createShaderModule(const std::vector<char> &code);

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
        .cullMode = vk::CullModeFlagBits::eBack,
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

    static constexpr vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };

    static constexpr std::array blendAttachments = {colorBlendAttachment, colorBlendAttachment};
    static constexpr vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = vk::False,
        .logicOp =  vk::LogicOp::eCopy,
        .attachmentCount = static_cast<uint32_t>(blendAttachments.size()),
        .pAttachments =  blendAttachments.data()
    };

    //  try to set one push constant for model matrix
    static constexpr vk::PushConstantRange pcsTest{
        .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        .offset = 0,
        .size = static_cast<uint32_t>(sizeof(PushConstants))
    };

    static constexpr auto bindingDescription = Vertex::getBindingDescription();
    static constexpr auto attributeDescription = Vertex::getAttributeDescriptions();

    static constexpr vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = attributeDescription.size(),
        .pVertexAttributeDescriptions = attributeDescription.data()
    };

    static constexpr vk::PipelineDepthStencilStateCreateInfo depthStencilCreateInfo{
        .depthTestEnable = vk::True,
        .depthWriteEnable = vk::True,
        .depthCompareOp = vk::CompareOp::eLess,
        .depthBoundsTestEnable = vk::False,
        .stencilTestEnable = vk::False,
    };



};


