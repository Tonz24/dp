//
// Created by Tonz on 24.10.2025.
//

#pragma once
#include "raytracedRenderer.h"


class RaytracedRendererNEE : public RaytracedRenderer {
public:

    bool drawGUI() override;

    explicit RaytracedRendererNEE(const std::shared_ptr<GBuffer>& gBuffer) : RaytracedRenderer(gBuffer, rtStages) {}
    explicit RaytracedRendererNEE(const std::string_view& gBufferName): RaytracedRenderer(gBufferName, rtStages) {}


protected:
    const std::vector<RasterPipeline::ShaderStageInfo>& getShaderStages() override {
        return rtStages;
    }
private:

    static inline const std::vector<RasterPipeline::ShaderStageInfo> rtStages = {
            {"shaders/raygen_rgen.spv",vk::ShaderStageFlagBits::eRaygenKHR},
            {"shaders/miss_rmiss.spv",vk::ShaderStageFlagBits::eMissKHR},
            {"shaders/miss_brdf_sample_rmiss.spv",vk::ShaderStageFlagBits::eMissKHR},
            {"shaders/closesthit_rchit.spv",vk::ShaderStageFlagBits::eClosestHitKHR},
            {"shaders/closesthit_mirror_rchit.spv",vk::ShaderStageFlagBits::eClosestHitKHR},
            {"shaders/closesthit_brdf_sample_rchit.spv",vk::ShaderStageFlagBits::eClosestHitKHR},
    };
};
