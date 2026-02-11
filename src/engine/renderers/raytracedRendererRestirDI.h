//
// Created by Tonz on 24.10.2025.
//

#pragma once
#include "raytracedRenderer.h"


class RaytracedRendererRestirDI : public RaytracedRenderer {
public:

    bool drawGUI() override;

    explicit RaytracedRendererRestirDI(const std::shared_ptr<GBuffer>& gBuffer) : RaytracedRenderer(gBuffer, rtStages) {initReservoirBuffers();}
    explicit RaytracedRendererRestirDI(const std::string_view& gBufferName): RaytracedRenderer(gBufferName, rtStages) {initReservoirBuffers();}

protected:

    void recordTraceCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex) override;
    void recordInitialPassCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const glm::vec<2,uint32_t>& renderDims);
    void recordSpatialPassCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const glm::vec<2,uint32_t>& renderDims);
    void recordTemporalPassCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const glm::vec<2,uint32_t>& renderDims);
    void recordFinalShadePassCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const glm::vec<2,uint32_t>& renderDims);

    const std::vector<RasterPipeline::ShaderStageInfo>& getShaderStages() override {
        return rtStages;
    }

    void initReservoirBuffers();

private:

    std::array<VkUtils::BufferAlloc,3> reservoirSSBOs_{};
    bool doSpatialReuse_{false};
    bool doTemporalReuse_{false};

    static inline const std::vector<RasterPipeline::ShaderStageInfo> rtStages = {
            // raygen region
            {"shaders/candidate_pass_rgen.spv",vk::ShaderStageFlagBits::eRaygenKHR},
            {"shaders/spatial_pass_rgen.spv",vk::ShaderStageFlagBits::eRaygenKHR},
            {"shaders/temporal_pass_rgen.spv",vk::ShaderStageFlagBits::eRaygenKHR},
            {"shaders/shade_pass_rgen.spv",vk::ShaderStageFlagBits::eRaygenKHR},
            // miss region
            {"shaders/miss_rmiss.spv",vk::ShaderStageFlagBits::eMissKHR},
            {"shaders/miss_brdf_sample_rmiss.spv",vk::ShaderStageFlagBits::eMissKHR},
            // per material closest hit shaders
            {"shaders/closesthit_diffuse_rchit.spv",vk::ShaderStageFlagBits::eClosestHitKHR},
            {"shaders/closesthit_mirror_rchit.spv",vk::ShaderStageFlagBits::eClosestHitKHR},
            {"shaders/closesthit_pbr_rchit.spv",vk::ShaderStageFlagBits::eClosestHitKHR},
            // brdf ray helper closest hit shaders
            {"shaders/closesthit_brdf_sample_rchit.spv",vk::ShaderStageFlagBits::eClosestHitKHR},
            {"shaders/closesthit_brdf_sample_rchit.spv",vk::ShaderStageFlagBits::eClosestHitKHR},
            {"shaders/closesthit_brdf_sample_rchit.spv",vk::ShaderStageFlagBits::eClosestHitKHR},
            {"shaders/closesthit_brdf_sample_rchit.spv",vk::ShaderStageFlagBits::eClosestHitKHR},
    };
};
