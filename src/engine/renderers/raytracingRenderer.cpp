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

RaytracingRenderer::RaytracingRenderer(const std::shared_ptr<GBuffer>& gBuffer): DeferredRenderer(gBuffer) {
    initGraphicsPipelines();
}

RaytracingRenderer::RaytracingRenderer(const std::string_view& gBufferName): DeferredRenderer(gBufferName) {
    initGraphicsPipelines();
}

void RaytracingRenderer::initGraphicsPipelines() {
    std::vector descSetFillLayouts = {*Renderer::getDescSetLayoutFrame()};
    std::array fillAttachmentFormats{GBuffer::attachmentFormats[0], GBuffer::attachmentFormats[1],GBuffer::attachmentFormats[2], GBuffer::attachmentFormats[3]};

    std::array pcsFillRange{GBuffer::pcsFillRange};

    auto rtStages = std::vector<RasterPipeline::ShaderStageInfo>{
            {"shaders/raygen_rgen.spv",vk::ShaderStageFlagBits::eRaygenKHR},
            {"shaders/closesthit_rchit.spv",vk::ShaderStageFlagBits::eClosestHitKHR},
            {"shaders/miss_rmiss.spv",vk::ShaderStageFlagBits::eMissKHR}
    };
    rtPipeline_ = RaytracingPipeline{rtStages,descSetFillLayouts,pcsFillRange};
}

void RaytracingRenderer::recordCommandBuffer(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex,
    const vk::Image& swapchainImage, const vk::ImageView& swapchainImageView, const vk::Extent2D& swapchainExtent) {
    DeferredRenderer::recordCommandBuffer(scene, cmdBuf, frameInFlightIndex, swapchainImage, swapchainImageView, swapchainExtent);

    cmdBuf.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR,rtPipeline_.getGraphicsPipeline());
    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, rtPipeline_.getPipelineLayout(), 0, *getDescSetFrame(frameInFlightIndex), nullptr);

    auto renderDims = getRenderDimensions();

    cmdBuf.traceRaysKHR(rtPipeline_.getRaygenRegion(),rtPipeline_.getMissRegion(),rtPipeline_.getHitRegion(),rtPipeline_.getHitRegion(),renderDims.x,renderDims.y,1);

}
