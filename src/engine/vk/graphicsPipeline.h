//
// Created by Tonz on 25.08.2025.
//

#pragma once
#include <vulkan/vulkan_raii.hpp>

#include "vkUtils.h"
#include "../utils.h"
#include "../uboFormat.h"

class GraphicsPipeline {
public:
    struct ShaderStageInfo {
        std::string_view shaderPath{};
        vk::ShaderStageFlagBits stage{};
    };

    GraphicsPipeline(const std::vector<ShaderStageInfo>& shaderInfos, std::span<const vk::DescriptorSetLayout> descriptorSetLayouts,
                     std::span<const vk::PushConstantRange> pcsRange, std::span<const vk::Format> colorAttachmentFormats, bool hasVertexLayout,
                     vk::Format depthFormat = vk::Format::eUndefined);

    GraphicsPipeline() = default;

    [[nodiscard]] const vk::raii::Pipeline& getGraphicsPipeline() const { return graphicsPipeline_; }
    [[nodiscard]] const vk::raii::PipelineLayout& getPipelineLayout() const { return pipelineLayout_; }

    static vk::raii::ShaderModule createShaderModule(const std::vector<char> &code) {
        vk::ShaderModuleCreateInfo createInfo{
            .codeSize = code.size() * sizeof(char),
            .pCode = reinterpret_cast<const uint32_t*>(code.data())
        };
        return vk::raii::ShaderModule{VkUtils::getDevice(), createInfo};
    }

protected:

    std::vector<vk::raii::ShaderModule> shaderModules_{};

    vk::raii::Pipeline graphicsPipeline_{nullptr};
    vk::raii::PipelineLayout pipelineLayout_{nullptr};
    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo_{};
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages_{};

    void initShaderStages(const std::vector<ShaderStageInfo>& shaderInfos) {

        for (const auto & shaderInfo : shaderInfos) {
            auto code = Utils::readFile(shaderInfo.shaderPath);

            shaderModules_.emplace_back(createShaderModule(code));

            vk::PipelineShaderStageCreateInfo createInfo{
                .stage = shaderInfo.stage,
                .module = shaderModules_.back(),
                .pName = "main"
            };
            shaderStages_.emplace_back(createInfo);
        }
    }

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

};