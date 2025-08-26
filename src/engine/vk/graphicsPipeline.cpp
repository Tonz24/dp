//
// Created by Tonz on 25.08.2025.
//

#include "graphicsPipeline.h"

#include "../engine.h"
#include "../utils.h"

GraphicsPipeline::GraphicsPipeline(std::string_view vShaderPath, std::string_view fShaderPath, const std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts,  const std::vector<vk::Format>& colorAttachmentFormats, vk::Format depthFormat) {
    initShaders(vShaderPath,fShaderPath);

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
        .pSetLayouts =  descriptorSetLayouts.data(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pcsTest
    };

    pipelineLayout_ = vk::raii::PipelineLayout( Engine::getInstance().getDevice(), pipelineLayoutInfo );


    pipelineRenderingCreateInfo_ = vk::PipelineRenderingCreateInfo{
        .colorAttachmentCount = static_cast<uint32_t>(colorAttachmentFormats.size()),
        .pColorAttachmentFormats = colorAttachmentFormats.data(),
        .depthAttachmentFormat = depthFormat,
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
    graphicsPipeline_ = vk::raii::Pipeline(Engine::getInstance().getDevice(), nullptr, pipelineInfo);
}

void GraphicsPipeline::initShaders(std::string_view vShaderPath, std::string_view fShaderPath) {
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

vk::raii::ShaderModule GraphicsPipeline::createShaderModule(const std::vector<char>& code) {
    vk::ShaderModuleCreateInfo createInfo{
        .codeSize = code.size() * sizeof(char),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };
    return vk::raii::ShaderModule{Engine::getInstance().getDevice(), createInfo};
}
