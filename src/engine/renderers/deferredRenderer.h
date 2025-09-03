//
// Created by Tonz on 03.09.2025.
//

#pragma once
#include "renderer.h"
#include "../vk/graphicsPipeline.h"


class DeferredRenderer : public Renderer {
public:
    bool drawGUI() override;

    void render(const Scene& scene) override;

    DeferredRenderer(std::shared_ptr<GBuffer> gBuffer);
    DeferredRenderer(std::string_view gBufferName);

protected:
    void initGraphicsPipelines();
    void initDescriptorSetLayouts();

    void recordCommandBuffer() override;

    GraphicsPipeline skyboxPipeline_;
    GraphicsPipeline gBufferFillPipeline_;
    GraphicsPipeline gBufferShadePipeline_;

    std::shared_ptr<GBuffer> gBuffer_{nullptr};
};

