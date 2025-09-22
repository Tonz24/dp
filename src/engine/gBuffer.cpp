//
// Created by Tonz on 29.08.2025.
//

#include "gBuffer.h"

#include "engine.h"
#include "managers/resourceManager.h"



GBuffer::GBuffer(std::string_view resourceName, uint32_t width, uint32_t height) {
    std::string textureNamesPrefix{resourceName};

    createTextures(textureNamesPrefix,width,height);
}

void GBuffer::recordDescriptorSet() const {
    for (const auto & texture : textures_) {
        Renderer::registerTextureBindless(*texture);
    }
}

void GBuffer::createTextures(const std::string& prefix, uint32_t width, uint32_t height) {

    albedoMap_.reset();
    normalMap_.reset();
    materialIdMap_.reset();
    target_.reset();
    depthMap_.reset();
    objectIdMap_.reset();

    textures_.clear();

    albedoMap_ = TextureManager::getInstance()->registerResource(prefix + "_albedo",
                                                                 width,
                                                                 height,
                                                                 albedoMapVkFormat,
                                                                 albedoMapUsageFlags);

    normalMap_ = TextureManager::getInstance()->registerResource(prefix + "_normal",
                                                                 width,
                                                                 height,
                                                                 normalMapVkFormat,
                                                                 normalMapUsageFlags);

    materialIdMap_ = TextureManager::getInstance()->registerResource(prefix + "_mat_id",
                                                                 width,
                                                                 height,
                                                                 materialMapVkFormat,
                                                                 materialMapUsageFlags);

    target_ = TextureManager::getInstance()->registerResource(prefix + "_shading_target",
                                                                 width,
                                                                 height,
                                                                 getTargetVkFormat(),
                                                                 targetUsageFlags);

    depthMap_ = TextureManager::getInstance()->registerResource(prefix + "_depth",
                                                                width,
                                                                height,
                                                                depthMapVkFormat,
                                                                depthMapUsageFlags);

    objectIdMap_ = TextureManager::getInstance()->registerResource(prefix + "_obj_id",
                                                                 width,
                                                                 height,
                                                                 idMapVkFormat,
                                                                 idMapUsageFlags);

    textures_ = {albedoMap_,normalMap_,depthMap_,materialIdMap_, objectIdMap_, target_};
    recordDescriptorSet();
}

vk::Format GBuffer::getTargetVkFormat() {
    const auto& device = VkUtils::getPhysicalDevice();
    for (const auto & format : targetAcceptableFormats) {
        vk::FormatProperties props = device.getFormatProperties(format);

        //  all images are hardcoded to be tiled so use tiled features
        auto features = props.optimalTilingFeatures;

        //  return first found suitable format
        if ((features & targetFormatFlags) == targetFormatFlags)
            return format;
    }
    throw std::runtime_error("ERROR: no suitable format for G buffer target texture!");
}

void GBuffer::resizeContents(uint32_t width, uint32_t height) {
    createTextures(getResourceName(),width,height);
}

void GBuffer::transitionToFill(vk::raii::CommandBuffer& cmdBuf) const {
    target_->transitionLayout(vk::ImageLayout::eColorAttachmentOptimal,vk::PipelineStageFlagBits2::eColorAttachmentOutput,vk::AccessFlagBits2::eColorAttachmentWrite,cmdBuf);
    albedoMap_->transitionLayout(vk::ImageLayout::eColorAttachmentOptimal,vk::PipelineStageFlagBits2::eColorAttachmentOutput,vk::AccessFlagBits2::eColorAttachmentWrite,cmdBuf);
    normalMap_->transitionLayout(vk::ImageLayout::eColorAttachmentOptimal,vk::PipelineStageFlagBits2::eColorAttachmentOutput,vk::AccessFlagBits2::eColorAttachmentWrite,cmdBuf);
    objectIdMap_->transitionLayout(vk::ImageLayout::eColorAttachmentOptimal,vk::PipelineStageFlagBits2::eColorAttachmentOutput,vk::AccessFlagBits2::eColorAttachmentWrite,cmdBuf);
    materialIdMap_->transitionLayout(vk::ImageLayout::eColorAttachmentOptimal,vk::PipelineStageFlagBits2::eColorAttachmentOutput,vk::AccessFlagBits2::eColorAttachmentWrite,cmdBuf);
    depthMap_->transitionLayout(vk::ImageLayout::eDepthAttachmentOptimal,vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,vk::AccessFlagBits2::eDepthStencilAttachmentWrite | vk::AccessFlagBits2::eDepthStencilAttachmentRead,cmdBuf);
}

void GBuffer::transitionToShade(vk::raii::CommandBuffer& cmdBuf) const {
    target_->transitionLayout(vk::ImageLayout::eColorAttachmentOptimal,vk::PipelineStageFlagBits2::eColorAttachmentOutput,vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead,cmdBuf);
    albedoMap_->transitionLayout(vk::ImageLayout::eShaderReadOnlyOptimal,vk::PipelineStageFlagBits2::eFragmentShader,vk::AccessFlagBits2::eShaderSampledRead,cmdBuf);
    normalMap_->transitionLayout(vk::ImageLayout::eShaderReadOnlyOptimal,vk::PipelineStageFlagBits2::eFragmentShader,vk::AccessFlagBits2::eShaderSampledRead,cmdBuf);
    materialIdMap_->transitionLayout(vk::ImageLayout::eShaderReadOnlyOptimal,vk::PipelineStageFlagBits2::eFragmentShader,vk::AccessFlagBits2::eShaderSampledRead,cmdBuf);
    depthMap_->transitionLayout(vk::ImageLayout::eDepthReadOnlyOptimal,vk::PipelineStageFlagBits2::eFragmentShader,vk::AccessFlagBits2::eShaderSampledRead | vk::AccessFlagBits2::eDepthStencilAttachmentRead,cmdBuf);
}

void GBuffer::transitionToBlit(vk::raii::CommandBuffer& cmdBuf) const {
    target_->transitionLayout(vk::ImageLayout::eTransferSrcOptimal,vk::PipelineStageFlagBits2::eTransfer,vk::AccessFlagBits2::eTransferRead,cmdBuf);
}

void GBuffer::transitionToTrace(vk::raii::CommandBuffer& cmdBuf) const {
    target_->transitionLayout(vk::ImageLayout::eShaderReadOnlyOptimal,vk::PipelineStageFlagBits2::eRayTracingShaderKHR,vk::AccessFlagBits2::eShaderSampledRead,cmdBuf);
    albedoMap_->transitionLayout(vk::ImageLayout::eShaderReadOnlyOptimal,vk::PipelineStageFlagBits2::eRayTracingShaderKHR,vk::AccessFlagBits2::eShaderSampledRead,cmdBuf);
    normalMap_->transitionLayout(vk::ImageLayout::eShaderReadOnlyOptimal,vk::PipelineStageFlagBits2::eRayTracingShaderKHR,vk::AccessFlagBits2::eShaderSampledRead,cmdBuf);
    materialIdMap_->transitionLayout(vk::ImageLayout::eShaderReadOnlyOptimal,vk::PipelineStageFlagBits2::eRayTracingShaderKHR,vk::AccessFlagBits2::eShaderSampledRead,cmdBuf);
    depthMap_->transitionLayout(vk::ImageLayout::eDepthReadOnlyOptimal,vk::PipelineStageFlagBits2::eRayTracingShaderKHR,vk::AccessFlagBits2::eShaderSampledRead,cmdBuf);
}