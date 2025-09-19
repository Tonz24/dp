//
// Created by Tonz on 19.09.2025.
//

#pragma once
#include "vkUtils.h"
#include "../utils.h"

class GraphicsPipeline {
public:
    struct ShaderStageInfo {
        std::string_view shaderPath{};
        vk::ShaderStageFlagBits stage{};
    };

    GraphicsPipeline() = default;
    GraphicsPipeline(std::span<const vk::DescriptorSetLayout> descriptorSetLayouts, std::span<const vk::PushConstantRange> pcsRange);

    [[nodiscard]] const vk::raii::Pipeline& getGraphicsPipeline() const { return pipeline_; }
    [[nodiscard]] const vk::raii::PipelineLayout& getPipelineLayout() const { return pipelineLayout_; }

protected:
    std::vector<vk::raii::ShaderModule> shaderModules_{};

    vk::raii::Pipeline pipeline_{nullptr};
    vk::raii::PipelineLayout pipelineLayout_{nullptr};
    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo_{};
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages_{};

    void initShaderStages(const std::vector<ShaderStageInfo>& shaderInfos);

    static vk::raii::ShaderModule createShaderModule(const std::vector<char> &code);
};