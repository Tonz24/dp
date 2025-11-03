//
// Created by Tonz on 19.09.2025.
//

#include "graphicsPipeline.h"
GraphicsPipeline::GraphicsPipeline(std::span<const vk::DescriptorSetLayout> descriptorSetLayouts,std::span<const vk::PushConstantRange> pcsRange) {
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
        .pSetLayouts =  descriptorSetLayouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(pcsRange.size()),
        .pPushConstantRanges = pcsRange.data()
    };

    pipelineLayout_ = vk::raii::PipelineLayout( VkUtils::getDevice(), pipelineLayoutInfo );
}

void GraphicsPipeline::initShaderStages(const std::vector<ShaderStageInfo>& shaderInfos) {

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

vk::raii::ShaderModule GraphicsPipeline::createShaderModule(const std::vector<char>& code) {
    vk::ShaderModuleCreateInfo createInfo{
        .codeSize = code.size() * sizeof(char),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };
    return vk::raii::ShaderModule{VkUtils::getDevice(), createInfo};
}
