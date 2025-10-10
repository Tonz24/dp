//
// Created by Tonz on 19.09.2025.
//

#pragma once
#include <random>

#include "deferredRenderer.h"
#include "../vk/raytracingPipeline.h"


class RaytracingRenderer : public DeferredRenderer {
public:
    bool drawGUI() override;

    void render(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const vk::Image& swapchainImage,
        const vk::ImageView& swapchainImageView, const vk::Extent2D& swapchainExtent) override;

    explicit RaytracingRenderer(const std::shared_ptr<GBuffer>& gBuffer);

    explicit RaytracingRenderer(const std::string_view& gBufferName);

protected:
    void initGraphicsPipelines();

    void recordCommandBuffer(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const vk::Image& swapchainImage,
                             const vk::ImageView& swapchainImageView, const vk::Extent2D& swapchainExtent) override;

    void recordPresentBuffer(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const vk::Image& swapchainImage,
        const vk::ImageView& swapchainImageView, const vk::Extent2D& swapchainExtent) override;

    void recordTraceCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex);

    RaytracingPipeline rtPipeline_;

    std::random_device rngDevice_;
    std::mt19937 generator_;
    std::uniform_int_distribution<uint32_t> distr_;

    PcsRaygen pcs_{};

    std::shared_ptr<Texture> accumulator_{nullptr};

    static constexpr vk::ShaderStageFlags pcsRaygenStageFlags{vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eMissKHR | vk::ShaderStageFlagBits::eClosestHitKHR};
    static constexpr vk::PushConstantRange pcsRaygenRange{
        .stageFlags = pcsRaygenStageFlags,
        .offset = 0,
        .size = static_cast<uint32_t>(sizeof(PcsRaygen))
    };
    static constexpr vk::ImageUsageFlags accumulatorUsage{vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage};
};
