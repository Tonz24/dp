//
// Created by Tonz on 19.09.2025.
//

#pragma once
#include <random>

#include "deferredRenderer.h"
#include "../vk/raytracingPipeline.h"


class RaytracedRenderer : public DeferredRenderer {
public:
    bool drawGUI() override;

    void render(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const vk::Image& swapchainImage,
        const vk::ImageView& swapchainImageView, const vk::Extent2D& swapchainExtent) override;

    explicit RaytracedRenderer(const std::shared_ptr<GBuffer>& gBuffer);
    explicit RaytracedRenderer(const std::string_view& gBufferName);

    void resizeScreen(uint32_t newWidth, uint32_t newHeight) override;

protected:

    void recordCommandBuffer(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const vk::Image& swapchainImage,
                             const vk::ImageView& swapchainImageView, const vk::Extent2D& swapchainExtent) override;


    void recordTraceCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex);
    void recordTonemapCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex);

    RaytracingPipeline rtPipeline_;
    RasterPipeline tonemapPipeline_;

    std::random_device rngDevice_;
    std::mt19937 generator_;
    std::uniform_int_distribution<uint32_t> distr_;

    PcsRaygenNEE pcs_{};

    uint32_t tonemap_{1};

    static constexpr vk::ImageUsageFlags accumulatorUsage{vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled};

    static constexpr vk::ShaderStageFlags pcsRaygenStageFlags{vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eMissKHR | vk::ShaderStageFlagBits::eClosestHitKHR};
    static constexpr vk::PushConstantRange pcsRaygenRange{
        .stageFlags = pcsRaygenStageFlags,
        .offset = 0,
        .size = static_cast<uint32_t>(sizeof(PcsRaygenNEE))
    };
    static constexpr vk::PushConstantRange pcsTonemapRange{
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .offset = 0,
        .size = static_cast<uint32_t>(sizeof(PcsRtTonemap))
    };

private:
    void initGraphicsPipelines();


};
