//
// Created by Tonz on 24.10.2025.
//

#include "raytracedRendererNee.h"

#include "imgui/imgui.h"

bool RaytracedRendererNEE::drawGUI() {
    if (ImGui::CollapsingHeader("NEE path tracer")) {
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

RaytracedRendererNEE::RaytracedRendererNEE(const std::shared_ptr<GBuffer>& gBuffer): RaytracedRenderer(gBuffer) {
    initGraphicsPipelines();
}

RaytracedRendererNEE::RaytracedRendererNEE(const std::string_view& gBufferName): RaytracedRenderer(gBufferName) {
    initGraphicsPipelines();
}

void RaytracedRendererNEE::initGraphicsPipelines() {
    std::vector descSetFillLayouts = {*Renderer::getDescSetLayoutFrame()};

    std::array raygenRange{PcsRaygen::getRange()};

    auto rtStages = std::vector<RasterPipeline::ShaderStageInfo>{
                {"shaders/raygen_rgen.spv",vk::ShaderStageFlagBits::eRaygenKHR},
                {"shaders/miss_rmiss.spv",vk::ShaderStageFlagBits::eMissKHR},
                {"shaders/miss_brdf_sample_rmiss.spv",vk::ShaderStageFlagBits::eMissKHR},
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

    auto tonemapStages = std::vector<RasterPipeline::ShaderStageInfo>{
            {"shaders/skypass_vert.spv",vk::ShaderStageFlagBits::eVertex},
            {"shaders/tonemap_frag.spv",vk::ShaderStageFlagBits::eFragment}
    };

    tonemapPipeline_ = RasterPipeline{
        tonemapStages,
        descSetFillLayouts,
        std::array{PcsRtTonemap::getRange()},
        std::array{gBuffer_->getTarget().getVkFormat()},
        false
    };
}
