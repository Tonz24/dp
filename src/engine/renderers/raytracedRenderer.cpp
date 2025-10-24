//
// Created by Tonz on 19.09.2025.
//

#include "raytracedRenderer.h"

#include "../managers/resourceManager.h"
#include "imgui/imgui.h"

bool RaytracedRenderer::drawGUI() {
    if (ImGui::CollapsingHeader("Basic path tracer")) {
        ImGui::Indent();

        ImGui::Checkbox("Tonemap",reinterpret_cast<bool*>(&tonemap_));
        ImGui::Checkbox("Accumulate",reinterpret_cast<bool*>(&pcs_.accumulate));
        ImGui::DragInt("Max bounce count",reinterpret_cast<int*>(&pcs_.maxRecursionDepth),0.33,1,16);
        ImGui::Checkbox("Next event estimation",reinterpret_cast<bool*>(&pcs_.NEE));
        ImGui::Checkbox("Sample sky",reinterpret_cast<bool*>(&pcs_.sampleSky));

        ImGui::Unindent();
    }
    return false;
}

void RaytracedRenderer::render(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const vk::Image& swapchainImage,
                                const vk::ImageView& swapchainImageView, const vk::Extent2D& swapchainExtent) {
    DeferredRenderer::render(scene, cmdBuf, frameInFlightIndex, swapchainImage, swapchainImageView, swapchainExtent);
}

RaytracedRenderer::RaytracedRenderer(const std::shared_ptr<GBuffer>& gBuffer): DeferredRenderer(gBuffer) {
    initGraphicsPipelines();
}

RaytracedRenderer::RaytracedRenderer(const std::string_view& gBufferName): DeferredRenderer(gBufferName) {
    initGraphicsPipelines();
}

void RaytracedRenderer::resizeScreen(uint32_t newWidth, uint32_t newHeight) {
    DeferredRenderer::resizeScreen(newWidth, newHeight);

    if (newWidth != accumulator_->getWidth() || newHeight != accumulator_->getHeight())
        initAccumulator(newWidth, newHeight);

    pcs_.albedoMapHandle = gBuffer_->getAlbedoMap().getCID();
    pcs_.normalMapHandle = gBuffer_->getNormalMap().getCID();
    pcs_.depthMapHandle = gBuffer_->getDepthMap().getCID();
    pcs_.materialMapHandle = gBuffer_->getMaterialMap().getCID();
    pcs_.targetHandle = gBuffer_->getTarget().getCID();
    pcs_.maxRecursionDepth = rtPipeline_.getMaxRecursionDepth();
}

void RaytracedRenderer::initGraphicsPipelines() {
    std::vector descSetFillLayouts = {*Renderer::getDescSetLayoutFrame()};

    std::array raygenRange{pcsRaygenRange};

    auto rtStages = std::vector<RasterPipeline::ShaderStageInfo>{
            {"shaders/raygen_rgen.spv",vk::ShaderStageFlagBits::eRaygenKHR},
            {"shaders/miss_rmiss.spv",vk::ShaderStageFlagBits::eMissKHR},
            {"shaders/closesthit_rchit.spv",vk::ShaderStageFlagBits::eClosestHitKHR},
            {"shaders/closesthit_mirror_rchit.spv",vk::ShaderStageFlagBits::eClosestHitKHR},
            {"shaders/closesthit_brdf_sample_rchit.spv",vk::ShaderStageFlagBits::eClosestHitKHR},
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

   initAccumulator(gBuffer_->getTarget().getWidth(), gBuffer_->getTarget().getHeight());

    auto tonemapStages = std::vector<RasterPipeline::ShaderStageInfo>{
        {"shaders/skypass_vert.spv",vk::ShaderStageFlagBits::eVertex},
        {"shaders/tonemap_frag.spv",vk::ShaderStageFlagBits::eFragment}
    };

    tonemapPipeline_ = RasterPipeline{
        tonemapStages,
        descSetFillLayouts,
        std::array{pcsTonemapRange},
        std::array{gBuffer_->getTarget().getVkFormat()},
        false
    };
}

void RaytracedRenderer::initAccumulator(uint32_t width, uint32_t height) {
    accumulator_.reset();
    accumulator_ = TextureManager::getInstance()->registerResource(gBuffer_->getResourceName() + "_accumulator",
                                                                width,
                                                                height,
                                                                vk::Format::eR32G32B32A32Sfloat,
                                                                accumulatorUsage);
}

void RaytracedRenderer::recordCommandBuffer(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex,
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

    // trace rays
    recordTraceCommands(scene,cmdBuf,frameInFlightIndex);

    //  transition accumulator to readonly optimal for sampling
    accumulator_->transitionLayout(vk::ImageLayout::eShaderReadOnlyOptimal,vk::PipelineStageFlagBits2::eFragmentShader,vk::AccessFlagBits2::eShaderSampledRead,cmdBuf);
    // transition target to color attachment optimal (will store the result of tonemapping)
    gBuffer_->getTarget().transitionLayout(vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,cmdBuf);

    recordTonemapCommands(scene,cmdBuf,frameInFlightIndex);
}

void RaytracedRenderer::recordTraceCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex) {
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR,rtPipeline_.getGraphicsPipeline());
    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, rtPipeline_.getPipelineLayout(), 0, *getDescSetFrame(frameInFlightIndex), nullptr);

    pcs_.seed = distr_(generator_);

    cmdBuf.pushConstants(rtPipeline_.getPipelineLayout(), pcsRaygenStageFlags,0, vk::ArrayProxy<const PcsRaygenNEE>{pcs_});

    auto renderDims = getRenderDimensions();

    cmdBuf.traceRaysKHR(rtPipeline_.getRaygenRegion(),rtPipeline_.getMissRegion(),rtPipeline_.getHitRegion(),rtPipeline_.getHitRegion(),renderDims.x,renderDims.y,1);

    pcs_.frameCtr += 1;

    if (!pcs_.accumulate) pcs_.frameCtr = 0;
}

void RaytracedRenderer::recordTonemapCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex) {

    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);
    vk::RenderingAttachmentInfo colorAttachmentInfo = {
        .imageView = gBuffer_->getTarget().getVkImageView(),
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor
    };

    vk::Extent2D extent{
        .width = gBuffer_->getTarget().getWidth(),
        .height = gBuffer_->getTarget().getHeight()
    };

    vk::RenderingInfo renderingInfo{
        .renderArea = {
            .offset = {
                .x = 0,
                .y = 0
            },
            .extent = extent
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentInfo,
        .pDepthAttachment =  nullptr
    };

    //  set dynamic rendering state values
    const vk::Viewport viewport{
        .x = 0,
        .y = static_cast<float>(gBuffer_->getTarget().getHeight()),
        .width = static_cast<float>(gBuffer_->getTarget().getWidth()),
        .height = -static_cast<float>(gBuffer_->getTarget().getHeight()),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    const vk::Rect2D scissor{
        .offset = vk::Offset2D{
            .x = 0,
            .y = 0
        },
        .extent =  extent
    };

    //begin rendering with the specified info
    cmdBuf.beginRendering(renderingInfo);

    cmdBuf.setViewport(0, viewport);
    cmdBuf.setScissor(0, scissor);

    //  bind graphics pipeline and global descriptor set
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, tonemapPipeline_.getGraphicsPipeline());
    //  bind global descriptor set
    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, tonemapPipeline_.getPipelineLayout(), 0, *getDescSetFrame(frameInFlightIndex), nullptr);


    PcsRtTonemap tonemapPcs{
        .accumulatorHandle = accumulator_->getCID(),
        .normalTexIndex = gBuffer_->getNormalMap().getCID(),
        .doTonemap = tonemap_
    };

    cmdBuf.pushConstants(tonemapPipeline_.getPipelineLayout(), vk::ShaderStageFlagBits::eFragment,0, vk::ArrayProxy<const PcsRtTonemap>{ tonemapPcs});

    // draw six vertices making up the screen quad
    cmdBuf.draw(6, 1, 0, 0);
    cmdBuf.endRendering();
}