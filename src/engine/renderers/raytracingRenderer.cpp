//
// Created by Tonz on 19.09.2025.
//

#include "raytracingRenderer.h"

#include "imgui/imgui.h"

bool RaytracingRenderer::drawGUI() {
    if (ImGui::CollapsingHeader("Path tracer")) {
        ImGui::Indent();

        ImGui::Checkbox("Accumulate",reinterpret_cast<bool*>(&pcs_.accumulate));
        ImGui::DragInt("Max bounce count",reinterpret_cast<int*>(&pcs_.maxRecursionDepth),0.33,1,16);


        ImGui::Unindent();
    }
    return false;
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

    std::array raygenRange{pcsRaygenRange};

    auto rtStages = std::vector<RasterPipeline::ShaderStageInfo>{
            {"shaders/raygen_rgen.spv",vk::ShaderStageFlagBits::eRaygenKHR},
            {"shaders/miss_rmiss.spv",vk::ShaderStageFlagBits::eMissKHR},
            {"shaders/closesthit_rchit.spv",vk::ShaderStageFlagBits::eClosestHitKHR},
    };
    rtPipeline_ = RaytracingPipeline{rtStages,descSetFillLayouts,raygenRange};

    pcs_.albedoMapHandle = gBuffer_->getAlbedoMap().getCID();
    pcs_.normalMapHandle = gBuffer_->getNormalMap().getCID();
    pcs_.depthMapHandle = gBuffer_->getDepthMap().getCID();
    pcs_.materialMapHandle = gBuffer_->getMaterialMap().getCID();
    pcs_.targetHandle = gBuffer_->getTarget().getCID();
    pcs_.maxRecursionDepth = rtPipeline_.getMaxRecursionDepth();

    generator_ = std::mt19937(rngDevice_());
    distr_ = std::uniform_int_distribution(std::numeric_limits<uint32_t>::min(),std::numeric_limits<uint32_t>::max());

    accumulator_ = TextureManager::getInstance()->registerResource(gBuffer_->getResourceName() + "_accumulator",
                                                                 gBuffer_->getTarget().getWidth(),
                                                                 gBuffer_->getTarget().getHeight(),
                                                                 vk::Format::eR32G32B32A32Sfloat,
                                                                 accumulatorUsage);
    registerTextureStorage(*accumulator_);
}

void RaytracingRenderer::recordCommandBuffer(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex,
    const vk::Image& swapchainImage, const vk::ImageView& swapchainImageView, const vk::Extent2D& swapchainExtent) {

    pcs_.skyHandle = scene.getSky()->getCID();

    cmdBuf.reset();
    cmdBuf.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    gBuffer_->transitionToFill(cmdBuf);

    //  g buffer fill and sky pass (same as in the deferred renderer)
    recordSkyCommands(scene,cmdBuf,frameInFlightIndex);
    recordSceneCommands(scene,cmdBuf,frameInFlightIndex);

    gBuffer_->transitionToTrace(cmdBuf);
    accumulator_->transitionLayout(vk::ImageLayout::eGeneral,vk::PipelineStageFlagBits2::eRayTracingShaderKHR,vk::AccessFlagBits2::eShaderStorageWrite | vk::AccessFlagBits2::eShaderStorageRead,cmdBuf);

    recordTraceCommands(scene,cmdBuf,frameInFlightIndex);
}

void RaytracingRenderer::recordPresentBuffer(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex,
    const vk::Image& swapchainImage, const vk::ImageView& swapchainImageView, const vk::Extent2D& swapchainExtent)
{
   //  transition accumulator to blit
    accumulator_->transitionLayout(vk::ImageLayout::eTransferSrcOptimal,vk::PipelineStageFlagBits2::eTransfer,vk::AccessFlagBits2::eTransferRead,cmdBuf);

    VkUtils::transitionImageLayout(swapchainImage,
                                   vk::ImageLayout::eUndefined,
                                   vk::ImageLayout::eTransferDstOptimal,
                                   vk::PipelineStageFlagBits2::eBottomOfPipe,
                                   vk::AccessFlagBits2::eNone,
                                   vk::PipelineStageFlagBits2::eTransfer,
                                   vk::AccessFlagBits2::eTransferWrite,
                                   vk::ImageAspectFlagBits::eColor,
                                   cmdBuf);

    VkUtils::blit(cmdBuf,accumulator_->getVkImage().image,
                    {accumulator_->getWidth(),accumulator_->getHeight()},
                    vk::ImageAspectFlagBits::eColor,
                    swapchainImage,
                    {swapchainExtent.width,swapchainExtent.height},
                    vk::ImageAspectFlagBits::eColor,
                    vk::Filter::eNearest);


    //  transition swapchain image into color attachment optimal for gui write
    VkUtils::transitionImageLayout(swapchainImage,
                                   vk::ImageLayout::eTransferDstOptimal,
                                   vk::ImageLayout::eColorAttachmentOptimal,
                                   vk::PipelineStageFlagBits2::eTransfer,
                                   vk::AccessFlagBits2::eTransferWrite,
                                   vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                   vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite,
                                   vk::ImageAspectFlagBits::eColor,
                                   cmdBuf);

    // render gui last, into the swapchain frame buffer
    recordGUICommands(scene,cmdBuf,frameInFlightIndex, swapchainImageView, swapchainExtent);

    //  transition swapchain image to present
    VkUtils::transitionImageLayout(swapchainImage,
                                   vk::ImageLayout::eColorAttachmentOptimal,
                                   vk::ImageLayout::ePresentSrcKHR,
                                   vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                   vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead,
                                   vk::PipelineStageFlagBits2::eBottomOfPipe,
                                   vk::AccessFlagBits2::eNone,
                                   vk::ImageAspectFlagBits::eColor,
                                   cmdBuf);

    cmdBuf.end();
}

void RaytracingRenderer::recordTraceCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex) {

    cmdBuf.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR,rtPipeline_.getGraphicsPipeline());
    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, rtPipeline_.getPipelineLayout(), 0, *getDescSetFrame(frameInFlightIndex), nullptr);

    pcs_.seed = distr_(generator_);

    cmdBuf.pushConstants(rtPipeline_.getPipelineLayout(), pcsRaygenStageFlags,0, vk::ArrayProxy<const PcsRaygen>{pcs_});

    auto renderDims = getRenderDimensions();

    cmdBuf.traceRaysKHR(rtPipeline_.getRaygenRegion(),rtPipeline_.getMissRegion(),rtPipeline_.getHitRegion(),rtPipeline_.getHitRegion(),renderDims.x,renderDims.y,1);

    pcs_.frameCtr += 1;

    if (!pcs_.accumulate)
        pcs_.frameCtr = 0;

}
