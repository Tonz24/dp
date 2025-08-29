//
// Created by Tonz on 29.08.2025.
//

#include "gBuffer.h"
#include "managers/resourceManager.h"



GBuffer::GBuffer(std::string_view resourceName, uint32_t width, uint32_t height) {
    std::string textureNamesPrefix{resourceName};

    albedoMap_ = TextureManager::getInstance()->registerResource(textureNamesPrefix + "_albedo",
                                                                 width,
                                                                 height,
                                                                 albedoMapChannelCount,
                                                                 albedoMapVkFormat,
                                                                 albedoMapUsageFlags);

    normalMap_ = TextureManager::getInstance()->registerResource(textureNamesPrefix + "_normal",
                                                                 width,
                                                                 height,
                                                                 normalMapChannelCount,
                                                                 normalMapVkFormat,
                                                                 normalMapUsageFlags);


    depthMap_ = TextureManager::getInstance()->registerResource(textureNamesPrefix + "_depth",
                                                                 width,
                                                                 height,
                                                                 depthMapChannelCount,
                                                                 depthMapVkFormat,
                                                                 depthMapUsageFlags);

    objectIdMap_ = TextureManager::getInstance()->registerResource(textureNamesPrefix + "_obj_id",
                                                                 width,
                                                                 height,
                                                                 idMapChannelCount,
                                                                 idMapVkFormat,
                                                                 idMapUsageFlags);


    target_ = TextureManager::getInstance()->registerResource(textureNamesPrefix + "_shading_target",
                                                                 width,
                                                                 height,
                                                                 targetChannelCount,
                                                                 targetVkFormat,
                                                                 targetUsageFlags);

    auto cmdBuf = VkUtils::beginSingleTimeCommand();

    //  transition albedo map
    VkUtils::transitionImageLayout(albedoMap_->getVkImage().image,
                                  vk::ImageLayout::eUndefined,
                                  vk::ImageLayout::eColorAttachmentOptimal,
                                  vk::PipelineStageFlagBits2::eTopOfPipe,
                                  vk::AccessFlagBits2::eNone,
                                  vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                  vk::AccessFlagBits2::eColorAttachmentWrite,
                                  vk::ImageAspectFlagBits::eColor,
                                  cmdBuf);

    /*//  transition normals
    VkUtils::transitionImageLayout(normalMap_->getVkImage().image,
                                  vk::ImageLayout::eUndefined,
                                  vk::ImageLayout::eColorAttachmentOptimal,
                                  vk::PipelineStageFlagBits2::eTopOfPipe,
                                  vk::AccessFlagBits2::eNone,
                                  vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                  vk::AccessFlagBits2::eColorAttachmentWrite,
                                  vk::ImageAspectFlagBits::eColor,
                                  cmdBuf);



    //  transition depth
    VkUtils::transitionImageLayout(depthMap_->getVkImage().image,
                                  vk::ImageLayout::eUndefined,
                                  vk::ImageLayout::eDepthAttachmentOptimal,
                                  vk::PipelineStageFlagBits2::eTopOfPipe,
                                  vk::AccessFlagBits2::eNone,
                                  vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
                                  vk::AccessFlagBits2::eDepthStencilAttachmentWrite | vk::AccessFlagBits2::eDepthStencilAttachmentRead,
                                  vk::ImageAspectFlagBits::eDepth,
                                  cmdBuf);*/

    //  transition id map
    VkUtils::transitionImageLayout(objectIdMap_->getVkImage().image,
                                   vk::ImageLayout::eUndefined,
                                   vk::ImageLayout::eColorAttachmentOptimal,
                                   vk::PipelineStageFlagBits2::eTopOfPipe,
                                   vk::AccessFlagBits2::eNone,
                                   vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                   vk::AccessFlagBits2::eColorAttachmentWrite,
                                   vk::ImageAspectFlagBits::eColor,
                                   cmdBuf);

/*
    //  transition target
    VkUtils::transitionImageLayout(target_->getVkImage().image,
                                   vk::ImageLayout::eUndefined,
                                   vk::ImageLayout::eColorAttachmentOptimal,
                                   vk::PipelineStageFlagBits2::eTopOfPipe,
                                   vk::AccessFlagBits2::eNone,
                                   vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                   vk::AccessFlagBits2::eColorAttachmentWrite,
                                   vk::ImageAspectFlagBits::eColor,
                                   cmdBuf);*/

    VkUtils::endSingleTimeCommand(cmdBuf,VkUtils::QueueType::graphics);
}


void GBuffer::transitionToGather(vk::raii::CommandBuffer& cmdBuf) const {

    //  transition albedo map
    VkUtils::transitionImageLayout(albedoMap_->getVkImage().image,
                                  vk::ImageLayout::eShaderReadOnlyOptimal,
                                  vk::ImageLayout::eColorAttachmentOptimal,
                                  vk::PipelineStageFlagBits2::eFragmentShader,
                                  vk::AccessFlagBits2::eShaderRead,
                                  vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                  vk::AccessFlagBits2::eColorAttachmentWrite,
                                  vk::ImageAspectFlagBits::eColor,
                                  cmdBuf);

    /*//  transition normals
    VkUtils::transitionImageLayout(normalMap_->getVkImage().image,
                                  vk::ImageLayout::eShaderReadOnlyOptimal,
                                  vk::ImageLayout::eColorAttachmentOptimal,
                                  vk::PipelineStageFlagBits2::eFragmentShader,
                                  vk::AccessFlagBits2::eShaderRead,
                                  vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                  vk::AccessFlagBits2::eColorAttachmentWrite,
                                  vk::ImageAspectFlagBits::eColor,
                                  cmdBuf);


    //  transition depth
    VkUtils::transitionImageLayout(depthMap_->getVkImage().image,
                                    vk::ImageLayout::eShaderReadOnlyOptimal,
                                  vk::ImageLayout::eDepthAttachmentOptimal,
                                  vk::PipelineStageFlagBits2::eFragmentShader,
                                        vk::AccessFlagBits2::eShaderRead,
                                  vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
                                  vk::AccessFlagBits2::eDepthStencilAttachmentWrite | vk::AccessFlagBits2::eDepthStencilAttachmentRead,
                                  vk::ImageAspectFlagBits::eDepth,
                                  cmdBuf);*/
}

void GBuffer::transitionToShade(vk::raii::CommandBuffer& cmdBuf) const {

    //  transition albedo map
    VkUtils::transitionImageLayout(albedoMap_->getVkImage().image,
                                  vk::ImageLayout::eColorAttachmentOptimal,
                                  vk::ImageLayout::eShaderReadOnlyOptimal,
                                  vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                  vk::AccessFlagBits2::eColorAttachmentWrite,
                                  vk::PipelineStageFlagBits2::eFragmentShader,
                                  vk::AccessFlagBits2::eShaderRead,
                                  vk::ImageAspectFlagBits::eColor,
                                  cmdBuf);

    /*//  transition normals
    VkUtils::transitionImageLayout(normalMap_->getVkImage().image,
                                  vk::ImageLayout::eColorAttachmentOptimal,
                                  vk::ImageLayout::eShaderReadOnlyOptimal,
                                  vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                  vk::AccessFlagBits2::eColorAttachmentWrite,
                                  vk::PipelineStageFlagBits2::eFragmentShader,
                                  vk::AccessFlagBits2::eShaderRead,
                                  vk::ImageAspectFlagBits::eColor,
                                  cmdBuf);

    //  transition depth
    VkUtils::transitionImageLayout(depthMap_->getVkImage().image,
                                  vk::ImageLayout::eDepthAttachmentOptimal,
                                  vk::ImageLayout::eShaderReadOnlyOptimal,
                                  vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
                                  vk::AccessFlagBits2::eDepthStencilAttachmentWrite | vk::AccessFlagBits2::eDepthStencilAttachmentRead,
                                  vk::PipelineStageFlagBits2::eFragmentShader,
                                  vk::AccessFlagBits2::eShaderRead,
                                  vk::ImageAspectFlagBits::eDepth,
                                  cmdBuf);


    //  transition target
    VkUtils::transitionImageLayout(target_->getVkImage().image,
                                   vk::ImageLayout::eUndefined,
                                   vk::ImageLayout::eColorAttachmentOptimal,
                                   vk::PipelineStageFlagBits2::eTopOfPipe,
                                   vk::AccessFlagBits2::eNone,
                                   vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                   vk::AccessFlagBits2::eColorAttachmentWrite,
                                   vk::ImageAspectFlagBits::eColor,
                                   cmdBuf);

    //  transition target
    VkUtils::transitionImageLayout(target_->getVkImage().image,
                                    vk::ImageLayout::eTransferSrcOptimal,
                                   vk::ImageLayout::eColorAttachmentOptimal,
                                   vk::PipelineStageFlagBits2::eBlit,
                                   vk::AccessFlagBits2::eTransferRead,
                                   vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                   vk::AccessFlagBits2::eColorAttachmentWrite,
                                   vk::ImageAspectFlagBits::eColor,
                                   cmdBuf);*/


}

void GBuffer::transitionToBlit(vk::raii::CommandBuffer& cmdBuf) const {

    //  transition target
    VkUtils::transitionImageLayout(target_->getVkImage().image,
                                   vk::ImageLayout::eColorAttachmentOptimal,
                                   vk::ImageLayout::eTransferSrcOptimal,
                                   vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                   vk::AccessFlagBits2::eColorAttachmentWrite,
                                   vk::PipelineStageFlagBits2::eBlit,
                                   vk::AccessFlagBits2::eTransferRead,
                                   vk::ImageAspectFlagBits::eColor,
                                   cmdBuf);


}
