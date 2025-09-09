//
// Created by Tonz on 29.08.2025.
//

#include "gBuffer.h"

#include "engine.h"
#include "managers/resourceManager.h"



GBuffer::GBuffer(std::string_view resourceName, uint32_t width, uint32_t height) {
    std::string textureNamesPrefix{resourceName};

    allocateDescriptorSet();
    createTextures(textureNamesPrefix,width,height);
}

void GBuffer::allocateDescriptorSet() {
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = Engine::getInstance().getDescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &*Renderer::getDescSetLayoutMaterial(),
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
                                                                 targetVkFormat,
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

    textures_ = {albedoMap_,normalMap_,depthMap_,materialIdMap_};
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
                                  vk::PipelineStageFlagBits2::eFragmentShader,
                                vk::AccessFlagBits2::eShaderSampledRead | vk::AccessFlagBits2::eDepthStencilAttachmentRead,
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

void GBuffer::resizeContents(uint32_t width, uint32_t height) {
    createTextures(getResourceName(),width,height);
}


void GBuffer::transitionToFill(vk::raii::CommandBuffer& cmdBuf) const {

   VkUtils::transitionImageLayout(albedoMap_->getVkImage().image,
                                    vk::ImageLayout::eShaderReadOnlyOptimal,
                                  vk::ImageLayout::eColorAttachmentOptimal,
                                  vk::PipelineStageFlagBits2::eFragmentShader,
                                                vk::AccessFlagBits2::eShaderSampledRead,
                                                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                                vk::AccessFlagBits2::eColorAttachmentWrite,
                                  vk::ImageAspectFlagBits::eColor,
                                  cmdBuf);

    VkUtils::transitionImageLayout(normalMap_->getVkImage().image,
                                    vk::ImageLayout::eShaderReadOnlyOptimal,
                                  vk::ImageLayout::eColorAttachmentOptimal,
                                  vk::PipelineStageFlagBits2::eFragmentShader,
                                                vk::AccessFlagBits2::eShaderSampledRead,
                                                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                                vk::AccessFlagBits2::eColorAttachmentWrite,
                                  vk::ImageAspectFlagBits::eColor,
                                  cmdBuf);

    VkUtils::transitionImageLayout(materialIdMap_->getVkImage().image,
                                    vk::ImageLayout::eShaderReadOnlyOptimal,
                                  vk::ImageLayout::eColorAttachmentOptimal,
                                  vk::PipelineStageFlagBits2::eFragmentShader,
                                                vk::AccessFlagBits2::eShaderSampledRead,
                                                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                                vk::AccessFlagBits2::eColorAttachmentWrite,
                                  vk::ImageAspectFlagBits::eColor,
                                  cmdBuf);

    VkUtils::transitionImageLayout(depthMap_->getVkImage().image,
                                 vk::ImageLayout::eDepthStencilReadOnlyOptimal,
                                vk::ImageLayout::eDepthStencilAttachmentOptimal,
                                vk::PipelineStageFlagBits2::eFragmentShader,
                                vk::AccessFlagBits2::eShaderSampledRead | vk::AccessFlagBits2::eDepthStencilAttachmentRead,
                                vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
                                  vk::AccessFlagBits2::eDepthStencilAttachmentWrite | vk::AccessFlagBits2::eDepthStencilAttachmentRead,
                                vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil,
                                cmdBuf);

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
                                vk::AccessFlagBits2::eShaderSampledRead | vk::AccessFlagBits2::eDepthStencilAttachmentRead,
                                vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil,
                                cmdBuf);


}

void GBuffer::transitionToBlit(vk::raii::CommandBuffer& cmdBuf) const {

    VkUtils::transitionImageLayout(target_->getVkImage().image,
                                     vk::ImageLayout::eColorAttachmentOptimal,
                                    vk::ImageLayout::eTransferSrcOptimal,
                                    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                    vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead,
                                    vk::PipelineStageFlagBits2::eTransfer,
                                    vk::AccessFlagBits2::eTransferRead,
                                    vk::ImageAspectFlagBits::eColor,
                                    cmdBuf);


}

void GBuffer::transitionResetTarget(vk::raii::CommandBuffer& cmdBuf) const {
    VkUtils::transitionImageLayout(target_->getVkImage().image,
                                  vk::ImageLayout::eTransferSrcOptimal,
                                  vk::ImageLayout::eColorAttachmentOptimal,
                                vk::PipelineStageFlagBits2::eTransfer,
                                vk::AccessFlagBits2::eTransferRead,
                                vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                vk::AccessFlagBits2::eColorAttachmentWrite,
                                vk::ImageAspectFlagBits::eColor,
                                cmdBuf);
}
