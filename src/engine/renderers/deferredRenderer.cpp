//
// Created by Tonz on 03.09.2025.
//

#include "deferredRenderer.h"

#include <imgui/imgui_impl_vulkan.h>
bool DeferredRenderer::drawGUI() {
    if (ImGui::CollapsingHeader("Deferred renderer")) {
        ImGui::Indent();

        static constexpr std::array items{"Debug Phong","Albedo map","Normal map","Depth map","World space position"};
        if (ImGui::Combo("Show target", &pcs_.overlayIndex, items.data(), items.size())) {

        }
        ImGui::Checkbox("Draw skybox",reinterpret_cast<bool*>(&pcs_.drawSkybox));

        if (pcs_.overlayIndex == 0) {
            ImGui::DragFloat3("Debug light position",&pcs_.lightPosWS[0],0.1);
            ImGui::ColorEdit3("Debug light emission",&pcs_.lightEmission[0],ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
        }

        if (pcs_.overlayIndex == 2) {
            ImGui::Checkbox("Remap to [0,1] range",reinterpret_cast<bool*>(&pcs_.remapNormals));
        }

        ImGui::Unindent();
    }
    return false;
}

void DeferredRenderer::render(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const vk::Image& swapchainImage,
                              const vk::ImageView& swapchainImageView, const vk::Extent2D& swapchainExtent) {
    recordCommandBuffer(scene,cmdBuf,frameInFlightIndex,swapchainImage,swapchainImageView,swapchainExtent);
}

DeferredRenderer::DeferredRenderer(std::shared_ptr<GBuffer> gBuffer) : gBuffer_(std::move(gBuffer)) {
    initGraphicsPipelines();
}

DeferredRenderer::DeferredRenderer(std::string_view gBufferName) {
    gBuffer_ = GBufferManager::getInstance()->getResource(gBufferName);

    if (gBuffer_ == nullptr)
        throw std::runtime_error("ERROR: nonexistent G-buffer provided!");

    initGraphicsPipelines();
}

void DeferredRenderer::resizeScreen(uint32_t newWidth, uint32_t newHeight) {
    gBuffer_->resizeContents(newWidth,newHeight);
}

void DeferredRenderer::initGraphicsPipelines() {

    std::array pcsFillRange{GBuffer::pcsFillRange};
    std::vector descSetFillLayouts = {*Renderer::getDescSetLayoutFrame(), *Renderer::getDescSetLayoutMaterial()};
    std::array fillAttachmentFormats{GBuffer::targetVkFormat, GBuffer::attachmentFormats[1],GBuffer::attachmentFormats[2], GBuffer::attachmentFormats[3]};


    std::array pcsShadeRange{GBuffer::pcsShadeRange};
    std::array shadeAttachmentFormat{GBuffer::targetVkFormat};


    std::vector descSetLayoutSky = {*Renderer::getDescSetLayoutFrame(),*Renderer::getDescSetLayoutSky()};
    gBufferFillPipeline_ = GraphicsPipeline{"shaders/shader_vert.spv","shaders/gbuffer_fill_frag.spv",descSetFillLayouts,pcsFillRange,fillAttachmentFormats,true, GBuffer::depthMapVkFormat};
    skyboxPipeline_ = GraphicsPipeline{"shaders/skypass_vert.spv","shaders/skypass_frag.spv",descSetLayoutSky,{},{fillAttachmentFormats.begin(),1}, false};
    gBufferShadePipeline_ = GraphicsPipeline{"shaders/skypass_vert.spv","shaders/gbuffer_shade_frag.spv",descSetFillLayouts,pcsShadeRange,shadeAttachmentFormat, false};
}




void DeferredRenderer::recordCommandBuffer(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex,
                                           const vk::Image& swapchainImage, const vk::ImageView& swapchainImageView, const vk::Extent2D& swapchainExtent)
{
    cmdBuf.reset();
    cmdBuf.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    //  g buffer fill and sky pass
    recordSkyCommands(scene,cmdBuf,frameInFlightIndex);
    recordSceneCommands(scene,cmdBuf,frameInFlightIndex);

    //  transition to g buffer shade
    gBuffer_->transitionToShade(cmdBuf);

    recordGBufferShadeCommands(scene,cmdBuf,frameInFlightIndex);

    gBuffer_->transitionToFill(cmdBuf);

    //  transition g buffer target to blit
    gBuffer_->transitionToBlit(cmdBuf);

    VkUtils::transitionImageLayout(swapchainImage,
                                   vk::ImageLayout::eUndefined,
                                   vk::ImageLayout::eTransferDstOptimal,
                                   vk::PipelineStageFlagBits2::eBottomOfPipe,
                                   vk::AccessFlagBits2::eNone,
                                   vk::PipelineStageFlagBits2::eTransfer,
                                   vk::AccessFlagBits2::eTransferWrite,
                                   vk::ImageAspectFlagBits::eColor,
                                   cmdBuf);

    VkUtils::blit(cmdBuf,gBuffer_->getTarget().getVkImage().image,
                    {gBuffer_->getTarget().getWidth(),gBuffer_->getTarget().getHeight()},
                    vk::ImageAspectFlagBits::eColor,
                    swapchainImage,
                    {swapchainExtent.width,swapchainExtent.height},
                    vk::ImageAspectFlagBits::eColor,
                    vk::Filter::eNearest);

    // transition g buffer target to color attachment (preparation for the next frame)
    gBuffer_->transitionResetTarget(cmdBuf);


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
void DeferredRenderer::recordSkyCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex) {
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
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, skyboxPipeline_.getGraphicsPipeline());
    //  bind global descriptor set
    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, skyboxPipeline_.getPipelineLayout(), 0, *getDescSetFrame(frameInFlightIndex), nullptr);
    //  bind per mesh descriptor set
    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, skyboxPipeline_.getPipelineLayout(), 1, *scene.getSkyDescriptorSet(), nullptr);

    // draw six vertices making up the screen quad
    cmdBuf.draw(6, 1, 0, 0);
    cmdBuf.endRendering();
}


void DeferredRenderer::recordSceneCommands(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex) {
     //set up the color attachment
    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);

    std::array colorAttachmentInfos = {
        vk::RenderingAttachmentInfo { // albedo image
            .imageView = gBuffer_->getAlbedoMap().getVkImageView(),
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clearColor
        },
        vk::RenderingAttachmentInfo { // normals
            .imageView = gBuffer_->getNormalMap().getVkImageView(),
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clearColor
        },
        vk::RenderingAttachmentInfo { // id map
            .imageView = gBuffer_->getObjectIdMap().getVkImageView(),
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clearColor
        },
        vk::RenderingAttachmentInfo { // material map
            .imageView = gBuffer_->getMaterialMap().getVkImageView(),
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clearColor
        }
    };

    vk::ClearValue depthClearColor = vk::ClearDepthStencilValue(1.0f,0);
    vk::RenderingAttachmentInfo depthAttachmentInfo = {
        .imageView = gBuffer_->getDepthMap().getVkImageView(),
        .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = depthClearColor
    };

    vk::RenderingInfo renderingInfo{
        .renderArea = {
                .offset = {
                    .x = 0,
                    .y = 0
                },
                .extent = {gBuffer_->getAlbedoMap().getWidth(),gBuffer_->getAlbedoMap().getHeight()}
        },
        .layerCount = 1,
        .colorAttachmentCount = colorAttachmentInfos.size(),
        .pColorAttachments = colorAttachmentInfos.data(),
        .pDepthAttachment = &depthAttachmentInfo,
    };
    //  set dynamic rendering state values
    const vk::Viewport viewport{
        .x = 0,
        .y = static_cast<float>(gBuffer_->getAlbedoMap().getHeight()),
        .width = static_cast<float>(gBuffer_->getAlbedoMap().getWidth()),
        .height = -static_cast<float>(gBuffer_->getAlbedoMap().getHeight()),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    const vk::Rect2D scissor{
        .offset = vk::Offset2D{
            .x = 0,
            .y = 0
        },
        .extent =  {gBuffer_->getAlbedoMap().getWidth(),gBuffer_->getAlbedoMap().getHeight()}
    };

    //begin rendering with the specified info
    cmdBuf.beginRendering(renderingInfo);

    cmdBuf.setViewport(0, viewport);
    cmdBuf.setScissor(0, scissor);

    //  bind graphics pipeline and global descriptor set
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, gBufferFillPipeline_.getGraphicsPipeline());
    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, gBufferFillPipeline_.getPipelineLayout(), 0, *getDescSetFrame(frameInFlightIndex), nullptr);

    for (const auto &mesh : scene.getMeshes()) {
        mesh->recordDrawCommands(cmdBuf, gBufferFillPipeline_.getPipelineLayout());
    }
    cmdBuf.endRendering();
}

void DeferredRenderer::recordGBufferShadeCommands(const Scene& scene, const vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex) {
     //set up the color attachment
    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);

    std::array colorAttachmentInfos = {
        vk::RenderingAttachmentInfo { // swapchain image
            .imageView = gBuffer_->getTarget().getVkImageView(),
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eLoad,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clearColor
        }
    };

    vk::RenderingInfo renderingInfo{
        .renderArea = {
                .offset = {
                    .x = 0,
                    .y = 0
                },
                .extent = {gBuffer_->getTarget().getWidth(),gBuffer_->getTarget().getHeight()}
        },
        .layerCount = 1,
        .colorAttachmentCount = colorAttachmentInfos.size(),
        .pColorAttachments = colorAttachmentInfos.data(),
        .pDepthAttachment = nullptr,
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
        .extent =  {gBuffer_->getTarget().getWidth(),gBuffer_->getTarget().getHeight()}
    };

    //begin rendering with the specified info
    cmdBuf.beginRendering(renderingInfo);

    cmdBuf.setViewport(0, viewport);
    cmdBuf.setScissor(0, scissor);

    //  bind graphics pipeline and global descriptor set
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, gBufferShadePipeline_.getGraphicsPipeline());
    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, gBufferShadePipeline_.getPipelineLayout(), 0, *getDescSetFrame(frameInFlightIndex), nullptr);
    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, gBufferShadePipeline_.getPipelineLayout(), 1, *gBuffer_->getDescriptorSet(), nullptr);


    cmdBuf.pushConstants(gBufferShadePipeline_.getPipelineLayout(), vk::ShaderStageFlagBits::eFragment,0, vk::ArrayProxy<const PcsGBufferShade>{pcs_});
    cmdBuf.draw(6, 1, 0, 0);
    cmdBuf.endRendering();
}

void DeferredRenderer::recordGUICommands(const Scene& scene, const vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const vk::ImageView& swapchainImageView, const vk::Extent2D&
                                         swapchainExtent) {
    // prepare GUI render pass
    vk::RenderingAttachmentInfo guiAttachmentInfo = {
        .imageView = swapchainImageView,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eStore,
    };

    vk::RenderingInfo guiRenderingInfo{
        .renderArea = {
            .offset = { .x = 0, .y = 0 },
            .extent = swapchainExtent
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &guiAttachmentInfo,
        .pDepthAttachment = nullptr,
    };

    // render GUI separately
    cmdBuf.beginRendering(guiRenderingInfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),*cmdBuf);
    cmdBuf.endRendering();
}
