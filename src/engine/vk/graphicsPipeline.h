//
// Created by Tonz on 25.08.2025.
//

#pragma once
#include <vulkan/vulkan_raii.hpp>

#include "vkUtils.h"
#include "../utils.h"
#include "../uboFormat.h"
#include "../concepts.h"

template <VertexType V>
class GraphicsPipeline {
public:
    GraphicsPipeline(std::string_view vShaderPath,std::string_view fShaderPath, std::span<vk::DescriptorSetLayout> descriptorSetLayouts, std::span<vk::Format> colorAttachmentFormats, vk::Format depthFormat = vk::Format::eUndefined) {
        initShaders(vShaderPath,fShaderPath);

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
            .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
            .pSetLayouts =  descriptorSetLayouts.data(),
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pcsTest
        };

        pipelineLayout_ = vk::raii::PipelineLayout( VkUtils::getDevice(), pipelineLayoutInfo );

        vk::Bool32 depthTestEnable = depthFormat == vk::Format::eUndefined ? vk::False : vk::True;

        vk::PipelineDepthStencilStateCreateInfo depthStencilCreateInfo{
            .depthTestEnable = depthTestEnable,
            .depthWriteEnable = depthTestEnable,
            .depthCompareOp = vk::CompareOp::eLess,
            .depthBoundsTestEnable = vk::False,
            .stencilTestEnable = vk::False,
        };

        pipelineRenderingCreateInfo_ = vk::PipelineRenderingCreateInfo{
            .colorAttachmentCount = static_cast<uint32_t>(colorAttachmentFormats.size()),
            .pColorAttachmentFormats = colorAttachmentFormats.data(),
            .depthAttachmentFormat = depthFormat,
        };


        vk::PipelineColorBlendAttachmentState colorBlendAttachment{
            .blendEnable = vk::False,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
        };

        std::vector blendAttachments{colorAttachmentFormats.size(),colorBlendAttachment};

        vk::PipelineColorBlendStateCreateInfo colorBlending{
            .logicOpEnable = vk::False,
            .logicOp =  vk::LogicOp::eCopy,
            .attachmentCount = static_cast<uint32_t>(blendAttachments.size()),
            .pAttachments =  blendAttachments.data()
        };


        //  put all the info together and create the pipeline
        vk::GraphicsPipelineCreateInfo pipelineInfo{
            .pNext = &pipelineRenderingCreateInfo_,
            .stageCount = static_cast<uint32_t>(shaderStages_.size()),
            .pStages = shaderStages_.data(),
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pDepthStencilState = &depthStencilCreateInfo,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState,
            .layout = pipelineLayout_,
            .renderPass = nullptr,
            .basePipelineHandle = nullptr,
            .basePipelineIndex = -1,
        };
        graphicsPipeline_ = vk::raii::Pipeline(VkUtils::getDevice(), nullptr, pipelineInfo);
    }
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

    static constexpr auto bindingDescription = V::getBindingDescription();
    static constexpr auto attributeDescriptions = V::getAttributeDescriptions();

    static constexpr vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = attributeDescriptions.size(),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };
};