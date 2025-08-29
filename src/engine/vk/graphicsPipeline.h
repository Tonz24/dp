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
    GraphicsPipeline(std::string_view vShaderPath, std::string_view fShaderPath, std::span<const vk::DescriptorSetLayout> descriptorSetLayouts,
                     std::span<const vk::Format> colorAttachmentFormats, bool hasVertexLayout, vk::Format depthFormat = vk::Format::eUndefined);

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

    void initShaders(std::string_view vShaderPath, std::string_view fShaderPath) {
        auto vertexShaderCode = Utils::readFile(vShaderPath);
        auto fragmentShaderCode = Utils::readFile(fShaderPath);

        vShaderModule_ = createShaderModule(vertexShaderCode);
        fShaderModule_ = createShaderModule(fragmentShaderCode);

        shaderStages_ = {
            vk::PipelineShaderStageCreateInfo{// vertex shader comes first
                .stage = vk::ShaderStageFlagBits::eVertex,
                .module = vShaderModule_,
                .pName = "main"
            },
            vk::PipelineShaderStageCreateInfo{// followed by the fragment shader
                .stage = vk::ShaderStageFlagBits::eFragment,
                .module = fShaderModule_,
                .pName = "main"
            }
        };
    }

    static vk::raii::ShaderModule createShaderModule(const std::vector<char> &code) {
        vk::ShaderModuleCreateInfo createInfo{
            .codeSize = code.size() * sizeof(char),
            .pCode = reinterpret_cast<const uint32_t*>(code.data())
        };
        return vk::raii::ShaderModule{VkUtils::getDevice(), createInfo};
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


    static constexpr vk::PushConstantRange pcsTest{
        .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        .offset = 0,
        .size = static_cast<uint32_t>(sizeof(PushConstants))
    };
};