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

    uint32_t getAccumulatedFrameCount() override {return pcs_.frameCtr;}

protected:

    explicit RaytracedRenderer(const std::string_view& gBufferName, const std::vector<RasterPipeline::ShaderStageInfo>& rtStages);
    explicit RaytracedRenderer(const std::shared_ptr<GBuffer>& gBuffer, const std::vector<RasterPipeline::ShaderStageInfo>& rtStages);



    void recordCommandBuffer(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const vk::Image& swapchainImage,
                             const vk::ImageView& swapchainImageView, const vk::Extent2D& swapchainExtent) override;


    void recordTraceCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex);
    void recordTonemapCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex);

    void setPcsData() override;

    RaytracingPipeline rtPipeline_;
    RasterPipeline tonemapPipeline_;

    std::random_device rngDevice_;
    std::mt19937 generator_;
    std::uniform_int_distribution<uint32_t> distr_;

    PcsRaygen::Data pcs_{};


    uint32_t tonemap_{1};

    void initGraphicsPipelines(const std::vector<RasterPipeline::ShaderStageInfo>& rtStages);
    virtual const std::vector<RasterPipeline::ShaderStageInfo>& getShaderStages() {
        return rtStages;
    }


private:

    const static inline std::vector<RasterPipeline::ShaderStageInfo>  rtStages = {
        {"shaders/raygen_naive_rgen.spv",vk::ShaderStageFlagBits::eRaygenKHR},
        {"shaders/miss_naive_rmiss.spv",vk::ShaderStageFlagBits::eMissKHR},
        {"shaders/closesthit_naive_rchit.spv",vk::ShaderStageFlagBits::eClosestHitKHR},
        {"shaders/closesthit_mirror_naive_rchit.spv",vk::ShaderStageFlagBits::eClosestHitKHR},
        {"shaders/closesthit_pbr_naive_rchit.spv",vk::ShaderStageFlagBits::eClosestHitKHR},
    };

};
