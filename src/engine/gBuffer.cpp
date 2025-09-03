//
// Created by Tonz on 29.08.2025.
//

#include "gBuffer.h"

#include "engine.h"
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

    materialIdMap_ = TextureManager::getInstance()->registerResource(textureNamesPrefix + "_mat_id",
                                                                 width,
                                                                 height,
                                                                 materialMapChannelCount,
                                                                 materialMapVkFormat,
                                                                 materialMapUsageFlags);

    target_ = TextureManager::getInstance()->registerResource(textureNamesPrefix + "_shading_target",
                                                                 width,
                                                                 height,
                                                                 targetChannelCount,
                                                                 targetVkFormat,
                                                                 targetUsageFlags);




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

    textures_ = {albedoMap_,normalMap_,depthMap_,materialIdMap_};
    allocateDescriptorSet();
    recordDescriptorSet();

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

    //  transition normals
    VkUtils::transitionImageLayout(normalMap_->getVkImage().image,
                                  vk::ImageLayout::eUndefined,
                                  vk::ImageLayout::eColorAttachmentOptimal,
                                  vk::PipelineStageFlagBits2::eTopOfPipe,
                                  vk::AccessFlagBits2::eNone,
                                  vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                  vk::AccessFlagBits2::eColorAttachmentWrite,
                                  vk::ImageAspectFlagBits::eColor,
                                  cmdBuf);

    //  transition material map
    VkUtils::transitionImageLayout(materialIdMap_->getVkImage().image,
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
                                  vk::ImageLayout::eDepthStencilAttachmentOptimal,
                                  vk::PipelineStageFlagBits2::eTopOfPipe,
                                  vk::AccessFlagBits2::eNone,
                                  vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
                                  vk::AccessFlagBits2::eDepthStencilAttachmentWrite | vk::AccessFlagBits2::eDepthStencilAttachmentRead,
                                  vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil,
                                  cmdBuf);

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



    VkUtils::endSingleTimeCommand(cmdBuf,VkUtils::QueueType::graphics);
}

void GBuffer::allocateDescriptorSet() {
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = Engine::getInstance().getDescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &*Engine::getInstance().getDescriptorSetLayoutMaterial(),
    };

    auto h = VkUtils::getDevice().allocateDescriptorSets(allocInfo);
    descriptorSet_ = std::move(h.front());
}

void GBuffer::recordDescriptorSet() {
    std::vector<vk::WriteDescriptorSet> descriptorWrites{};
    std::vector<vk::DescriptorImageInfo> imageInfos;

    imageInfos.reserve(textures_.size());
    descriptorWrites.reserve(textures_.size());


    for (uint32_t i = 0; i < textures_.size(); ++i) {

        vk::ImageLayout imageLayout = textures_[i] == depthMap_ ?  vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eShaderReadOnlyOptimal;

        imageInfos.emplace_back(vk::DescriptorImageInfo{
            .sampler = textures_[i]->getVkSampler(),
            .imageView =  textures_[i]->getVkImageView(),
            .imageLayout = imageLayout
        });

        vk::WriteDescriptorSet writeDescriptorSet{
            .dstSet = descriptorSet_,
            .dstBinding = i,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &imageInfos.back()
        };

        descriptorWrites.emplace_back(writeDescriptorSet);
    }

    VkUtils::getDevice().updateDescriptorSets(descriptorWrites,{});
}


void GBuffer::transitionToFill(vk::raii::CommandBuffer& cmdBuf) const {

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

  //  transition to g buffer shade
    VkUtils::transitionImageLayout(target_->getVkImage().image,
                                    vk::ImageLayout::eColorAttachmentOptimal,
                                   vk::ImageLayout::eColorAttachmentOptimal,
                                   vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                   vk::AccessFlagBits2::eColorAttachmentWrite,
                                   vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                   vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead,
                                   vk::ImageAspectFlagBits::eColor,
                                   cmdBuf);


    VkUtils::transitionImageLayout(albedoMap_->getVkImage().image,
                                   vk::ImageLayout::eColorAttachmentOptimal,
                                  vk::ImageLayout::eShaderReadOnlyOptimal,
                                  vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                  vk::AccessFlagBits2::eColorAttachmentWrite,
                                  vk::PipelineStageFlagBits2::eFragmentShader,
                                  vk::AccessFlagBits2::eShaderSampledRead,
                                  vk::ImageAspectFlagBits::eColor,
                                  cmdBuf);

    VkUtils::transitionImageLayout(normalMap_->getVkImage().image,
                                  vk::ImageLayout::eColorAttachmentOptimal,
                                 vk::ImageLayout::eShaderReadOnlyOptimal,
                                 vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                 vk::AccessFlagBits2::eColorAttachmentWrite,
                                 vk::PipelineStageFlagBits2::eFragmentShader,
                                 vk::AccessFlagBits2::eShaderSampledRead,
                                 vk::ImageAspectFlagBits::eColor,
                                 cmdBuf);

    VkUtils::transitionImageLayout(materialIdMap_->getVkImage().image,
                                 vk::ImageLayout::eColorAttachmentOptimal,
                                vk::ImageLayout::eShaderReadOnlyOptimal,
                                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                vk::AccessFlagBits2::eColorAttachmentWrite,
                                vk::PipelineStageFlagBits2::eFragmentShader,
                                vk::AccessFlagBits2::eShaderSampledRead,
                                vk::ImageAspectFlagBits::eColor,
                                cmdBuf);

    VkUtils::transitionImageLayout(depthMap_->getVkImage().image,
                                 vk::ImageLayout::eDepthStencilAttachmentOptimal,
                                vk::ImageLayout::eDepthStencilReadOnlyOptimal,
                                vk::PipelineStageFlagBits2::eAllCommands,
                                  vk::AccessFlagBits2::eDepthStencilAttachmentWrite | vk::AccessFlagBits2::eDepthStencilAttachmentRead,
                                vk::PipelineStageFlagBits2::eFragmentShader,
                                vk::AccessFlagBits2::eDepthStencilAttachmentRead,
                                vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil,
                                cmdBuf);


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
