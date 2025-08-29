//
// Created by Tonz on 25.08.2025.
//

#include "graphicsPipeline.h"

#include "../../scene/Vertex.h"


GraphicsPipeline::GraphicsPipeline(std::string_view vShaderPath, std::string_view fShaderPath,
                                   std::span<const vk::DescriptorSetLayout> descriptorSetLayouts, std::span<const vk::Format> colorAttachmentFormats, bool hasVertexLayout, vk::Format depthFormat) {
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


    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};

    auto bindingDescription = Vertex3D::getBindingDescription();
    auto attributeDescriptions = Vertex3D::getAttributeDescriptions();

    if (hasVertexLayout) {
        vertexInputInfo = {
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &bindingDescription,
            .vertexAttributeDescriptionCount = attributeDescriptions.size(),
            .pVertexAttributeDescriptions = attributeDescriptions.data()
        };
    }


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
