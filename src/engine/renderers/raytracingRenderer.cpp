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

    std::array pcsFillRange{GBuffer::pcsShadeRange};

    auto rtStages = std::vector<RasterPipeline::ShaderStageInfo>{
            {"shaders/raygen_rgen.spv",vk::ShaderStageFlagBits::eRaygenKHR},
            {"shaders/closesthit_rchit.spv",vk::ShaderStageFlagBits::eClosestHitKHR},
            {"shaders/miss_rmiss.spv",vk::ShaderStageFlagBits::eMissKHR}
    };
    rtPipeline_ = RaytracingPipeline{rtStages,descSetFillLayouts,pcsFillRange};

    pcs_.albedoMapHandle = gBuffer_->getAlbedoMap().getCID();
    pcs_.normalMapHandle = gBuffer_->getNormalMap().getCID();
    pcs_.depthMapHandle = gBuffer_->getDepthMap().getCID();
    pcs_.materialMapHandle = gBuffer_->getMaterialMap().getCID();
}

void RaytracingRenderer::recordCommandBuffer(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex,
    const vk::Image& swapchainImage, const vk::ImageView& swapchainImageView, const vk::Extent2D& swapchainExtent) {
    //DeferredRenderer::recordCommandBuffer(scene, cmdBuf, frameInFlightIndex, swapchainImage, swapchainImageView, swapchainExtent);

    cmdBuf.reset();
    cmdBuf.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    gBuffer_->transitionToFill(cmdBuf);

    //  g buffer fill and sky pass (same as in the deferred renderer)
    recordSkyCommands(scene,cmdBuf,frameInFlightIndex);
    recordSceneCommands(scene,cmdBuf,frameInFlightIndex);

    gBuffer_->transitionToTrace(cmdBuf);

    recordTraceCommands(scene,cmdBuf,frameInFlightIndex);
}

void RaytracingRenderer::recordTraceCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex) {

    cmdBuf.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR,rtPipeline_.getGraphicsPipeline());
    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, rtPipeline_.getPipelineLayout(), 0, *getDescSetFrame(frameInFlightIndex), nullptr);
    cmdBuf.pushConstants(gBufferShadePipeline_.getPipelineLayout(), vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eFragment,0, vk::ArrayProxy<const PcsGBufferShade>{pcs_});

    auto renderDims = getRenderDimensions();

    cmdBuf.traceRaysKHR(rtPipeline_.getRaygenRegion(),rtPipeline_.getMissRegion(),rtPipeline_.getHitRegion(),rtPipeline_.getHitRegion(),renderDims.x,renderDims.y,1);
}
