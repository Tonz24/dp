//
// Created by Tonz on 03.09.2025.
//

#pragma once
#include <random>

#include "renderer.h"
#include "../gBuffer.h"
#include "../vk/RasterPipeline.h"
#include "../pushConstants.h"


class DeferredRenderer : public Renderer {
public:
    bool drawGUI() override;

    void render(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const vk::Image& swapchainImage, const vk::ImageView&
                swapchainImageView, const vk::Extent2D&
                swapchainExtent) override;

    explicit DeferredRenderer(std::shared_ptr<GBuffer> gBuffer);
    explicit DeferredRenderer(std::string_view gBufferName);

    void resizeScreen(uint32_t newWidth, uint32_t newHeight) override;
    [[nodiscard]] glm::vec<2, uint32_t> getRenderDimensions() const override {return {gBuffer_->getTarget().getWidth(),gBuffer_->getTarget().getHeight()};}

protected:

    void recordMultisampledSceneCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex);
    void recordSinglesampledSceneCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex);

    void recordGBufferShadeCommands(const Scene& scene, const vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex);

    void recordGUICommands(const Scene& scene, const vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const vk::ImageView& swapchainImageView, const vk::Extent2D&
                           swapchainExtent);

    void recordCommandBuffer(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const vk::Image& swapchainImage, const vk::ImageView&
                             swapchainImageView, const vk::Extent2D&
                             swapchainExtent) override;

    void recordSkyCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex);

    void recordPresentBuffer(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex,
                                           const vk::Image& swapchainImage, const vk::ImageView& swapchainImageView, const vk::Extent2D& swapchainExtent) override;

    RasterPipeline skyboxPipeline_;
    RasterPipeline gBufferFillPipeline_;
    RasterPipeline idFillPipeline_;
    RasterPipeline gBufferShadePipeline_;

    std::shared_ptr<GBuffer> gBuffer_{nullptr};

    std::random_device rngDevice_;
    std::mt19937 generator_;
    std::uniform_int_distribution<uint32_t> distr_;

     void setPcsData() override;

private:
    void initGraphicsPipelines();
    PcsGBufferShade::Data pcs_{};

};

