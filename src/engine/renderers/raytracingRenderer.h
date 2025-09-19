//
// Created by Tonz on 19.09.2025.
//

#pragma once
#include "deferredRenderer.h"
#include "../vk/raytracingPipeline.h"


class RaytracingRenderer : public DeferredRenderer {
public:
    bool drawGUI() override;

    void render(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const vk::Image& swapchainImage,
        const vk::ImageView& swapchainImageView, const vk::Extent2D& swapchainExtent) override;

    explicit RaytracingRenderer(const std::shared_ptr<GBuffer>& gBuffer)
        : DeferredRenderer(gBuffer) {}

    explicit RaytracingRenderer(const std::string_view& gBufferName)
        : DeferredRenderer(gBufferName) {}

protected:
    void initGraphicsPipelines();

    void recordCommandBuffer(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const vk::Image& swapchainImage,
                             const vk::ImageView& swapchainImageView, const vk::Extent2D& swapchainExtent) override;


    RaytracingPipeline rtPipeline_{};
};
