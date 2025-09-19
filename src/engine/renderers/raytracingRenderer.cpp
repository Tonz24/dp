//
// Created by Tonz on 19.09.2025.
//

#include "raytracingRenderer.h"

bool RaytracingRenderer::drawGUI() {
    return DeferredRenderer::drawGUI();
}

void RaytracingRenderer::render(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const vk::Image& swapchainImage,
                                const vk::ImageView& swapchainImageView, const vk::Extent2D& swapchainExtent) {
    DeferredRenderer::render(scene, cmdBuf, frameInFlightIndex, swapchainImage, swapchainImageView, swapchainExtent);
}

void RaytracingRenderer::initGraphicsPipelines() {

}

void RaytracingRenderer::recordCommandBuffer(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex,
    const vk::Image& swapchainImage, const vk::ImageView& swapchainImageView, const vk::Extent2D& swapchainExtent) {
    DeferredRenderer::recordCommandBuffer(scene, cmdBuf, frameInFlightIndex, swapchainImage, swapchainImageView, swapchainExtent);
}
