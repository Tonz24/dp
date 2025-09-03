//
// Created by Tonz on 03.09.2025.
//

#include "deferredRenderer.h"
bool DeferredRenderer::drawGUI() { return true; }
void DeferredRenderer::render(const Scene& scene) {}

DeferredRenderer::DeferredRenderer(std::shared_ptr<GBuffer> gBuffer) : Renderer(), gBuffer_(std::move(gBuffer_)) {
    initGraphicsPipelines();
}

DeferredRenderer::DeferredRenderer(std::string_view gBufferName) {
    gBuffer_ = GBufferManager::getInstance()->getResource(gBufferName);

    if (gBuffer_ == nullptr)
        throw std::runtime_error("ERROR: nonexistent G-buffer provided!");

    initGraphicsPipelines();
}

void DeferredRenderer::initGraphicsPipelines() {
    /*gBufferFillPipeline_ = GraphicsPipeline{"shaders/shader_vert.spv","shaders/gbuffer_fill_frag.spv",descriptorSetLayouts,pcsFillRange,huhAttachments,true, GBuffer::depthMapVkFormat};
skyboxPipeline_ = GraphicsPipeline{"shaders/skypass_vert.spv","shaders/skypass_frag.spv",descriptorSetLayoutsSky,{},colorAttachmentFormatsSky, false};
gBufferShadePipeline_ = GraphicsPipeline{"shaders/skypass_vert.spv","shaders/gbuffer_shade_frag.spv",descriptorSetLayouts,pcsShadeRange,gBufferShadeFormat, false};*/
}


void DeferredRenderer::recordCommandBuffer() {}
