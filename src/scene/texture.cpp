//
// Created by Tonz on 23.07.2025.
//

#include "texture.h"
#include <iostream>

#include "Vertex.h"
#include "../engine/engine.h"
#include "../engine/managers/resourceManager.h"

std::shared_ptr<Texture> Texture::createDummy(std::string_view name,  const glm::vec<4, uint8_t>& color) {

    vk::ImageUsageFlags usageFlags = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    auto dummy = TextureManager::getInstance()->registerResource(name,1,1,vk::Format::eB8G8R8A8Unorm, usageFlags);

    memcpy(dummy->data_.data(),&color[0],sizeof(color));

    VkUtils::BufferAlloc stagingBuffer = VkUtils::createBufferVMA(dummy->getTotalSize(),vk::BufferUsageFlagBits::eTransferSrc,VkUtils::stagingAllocFlagsVMA);
    dummy->stage(stagingBuffer);
    VkUtils::destroyBufferVMA(std::move(stagingBuffer));

    return dummy;
}


Texture::Texture(uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags imageUsage):
    ManagedResource(), width_(width), height_(height), channelCount_(chooseChannelCount(format)), vkFormat_(format), imageUsageFlags_(imageUsage)
{
    pixelSize_ = channelCount_;
    data_.reserve(width_ * height_ * channelCount_);
    data_.resize(width_ * height_ * channelCount_,0);
    initVkImage();

}

void Texture::initVkImage() {
    vk::ImageCreateInfo imageInfo{
        .imageType = vk::ImageType::e2D,
        .format = vkFormat_,
        .extent = vk::Extent3D{
            .width = width_,
            .height = height_,
            .depth = 1
        },
        .mipLevels = mipLevelCount_,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = imageUsageFlags_,
        .sharingMode = vk::SharingMode::eExclusive
    };


    imageAlloc_ = VkUtils::createImageVMA(imageInfo);

    aspectFlags_ = vk::ImageAspectFlagBits::eColor;
    if (imageUsageFlags_ & vk::ImageUsageFlagBits::eDepthStencilAttachment) {
        aspectFlags_ = vk::ImageAspectFlagBits::eDepth;
        samplerLayout_ = vk::ImageLayout::eDepthReadOnlyOptimal;
    }

    vk::ImageViewCreateInfo imageViewCreateInfo{
        .flags = vk::ImageViewCreateFlags(),
        .image = imageAlloc_.image,
        .viewType = vk::ImageViewType::e2D,
        .format = vkFormat_,
        .components = vk::ComponentMapping{
            .r = vk::ComponentSwizzle::eIdentity,
            .g = vk::ComponentSwizzle::eIdentity,
            .b = vk::ComponentSwizzle::eIdentity,
            .a = vk::ComponentSwizzle::eIdentity,
        },
        .subresourceRange = vk::ImageSubresourceRange{
            .aspectMask = aspectFlags_,
            .baseMipLevel = 0,
            .levelCount = mipLevelCount_,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
    };

    const auto& device = Engine::getInstance().getDevice();

    vkImageView_ = vk::raii::ImageView(device, imageViewCreateInfo);

    auto formatProps = Engine::getInstance().getPhysicalDevice().getFormatProperties2(vkFormat_);
    vk::Filter filter = formatProps.formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear ? vk::Filter::eLinear : vk::Filter::eNearest;
    vk::SamplerMipmapMode mipmapMode = formatProps.formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear ? vk::SamplerMipmapMode::eLinear : vk::SamplerMipmapMode::eNearest;

    vk::SamplerCreateInfo samplerInfo = {
        .magFilter = filter,
        .minFilter = filter,
        .mipmapMode  = mipmapMode,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .mipLodBias = 0.0f,
        .anisotropyEnable = vk::True,
        .maxAnisotropy = Engine::getInstance().getDeviceLimits().maxSamplerAnisotropy,
        .compareEnable = vk::False,
        .compareOp = vk::CompareOp::eAlways,
        .minLod = 0.0f,
        .maxLod = static_cast<float>(mipLevelCount_),
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
        .unnormalizedCoordinates = vk::False
    };
    vkSampler_ = vk::raii::Sampler(device,samplerInfo);


    mipLayouts_ = std::vector(mipLevelCount_,vk::ImageLayout::eUndefined);
    mipStageMasks_ = std::vector(mipLevelCount_,vk::PipelineStageFlags2{vk::PipelineStageFlagBits2::eTopOfPipe});
    mipAccessMasks_ = std::vector(mipLevelCount_,vk::AccessFlags2{vk::AccessFlagBits2::eNone});
}


Texture::Texture(std::string_view fileName, bool isSrgb, bool generateMipmaps) : ManagedResource() {
    //TODO: initialize only once
    FreeImage_Initialise();

    std::string correctFileName{fileName};

    auto extensionSeparator = correctFileName.find_last_of('.');
    fileName_ = correctFileName.substr(0,extensionSeparator);
    extension_ = correctFileName.substr(extensionSeparator, correctFileName.size() - extensionSeparator);

    FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(correctFileName.c_str(), 0);
    if (fif == FIF_UNKNOWN)
        fif = FreeImage_GetFIFFromFilename(correctFileName.c_str());

    if ( fif != FIF_UNKNOWN){

        FIBITMAP* bitmap{nullptr};

        if (fif == FIF_JPEG) {
            FIBITMAP* tempBitmap = FreeImage_Load(fif,correctFileName.c_str());
            bitmap = FreeImage_ConvertTo32Bits(tempBitmap);
            FreeImage_Unload(tempBitmap);
        }
        if (fif == FIF_EXR || fif == FIF_HDR) {
            FIBITMAP* tempBitmap = FreeImage_Load(fif,correctFileName.c_str());
            bitmap = FreeImage_ConvertToRGBAF(tempBitmap);
            FreeImage_Unload(tempBitmap);
        }

        if (bitmap){
            uint8_t * bits = nullptr;

            bits = FreeImage_GetBits( bitmap );

            width_ = FreeImage_GetWidth(bitmap);
            height_ = FreeImage_GetHeight(bitmap);

            freeImageFormat_ = FreeImage_GetFIFFromFilename(correctFileName.c_str());
            freeImageType_ = FreeImage_GetImageType(bitmap);

            if (bits != nullptr && width_ != 0 && height_ != 0) {
                pixelSize_ = FreeImage_GetBPP(bitmap) / 8;
                scanWidth_ = FreeImage_GetPitch(bitmap);
                channelCount_ = getChannelCount(freeImageType_,FreeImage_GetBPP(bitmap));

                data_.reserve(scanWidth_ * height_);
                data_.resize(scanWidth_ * height_,0);
                
                FreeImage_ConvertToRawBits(data_.data(),bitmap,scanWidth_,pixelSize_ * 8, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, FALSE);
                isFromDisk_ = true;

                //  transfer src for generating mipmaps!
                imageUsageFlags_ = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc;

                vkFormat_ = chooseVkFormat(isSrgb);
                if (generateMipmaps)
                    mipLevelCount_ = static_cast<uint32_t>(std::floor(std::log2(std::max(width_, height_)))) + 1;
                initVkImage();
            }
        }
        FreeImage_Unload(bitmap);
    }
    else
        throw std::runtime_error("ERROR! Failed to load texture " + std::string{fileName});

    FreeImage_DeInitialise();
}

std::string Texture::getResourceType() const {
    return "Texture";
}

void Texture::stage(const VkUtils::BufferAlloc& stagingBuffer) {

    if (stagingBuffer.allocationInfo.pMappedData == nullptr)
        throw std::runtime_error("ERROR: Mapped pointer points to NULL!");

    size_t imageSize = width_ * height_ * pixelSize_;

    memcpy(stagingBuffer.allocationInfo.pMappedData,data_.data(),imageSize);

    auto cmdBuf = VkUtils::beginSingleTimeCommand();

    transitionLayout(vk::ImageLayout::eTransferDstOptimal,vk::PipelineStageFlagBits2::eTransfer,vk::AccessFlagBits2::eTransferWrite,cmdBuf, {0,mipLevelCount_});

    VkUtils::copyBufferToImage(stagingBuffer,imageAlloc_,width_,height_, cmdBuf);

    transitionLayout(vk::ImageLayout::eShaderReadOnlyOptimal,vk::PipelineStageFlagBits2::eFragmentShader,vk::AccessFlagBits2::eShaderRead,cmdBuf, {0,mipLevelCount_});

    VkUtils::endSingleTimeCommand(cmdBuf,VkUtils::QueueType::graphics);
    generateMipmaps();
}

void Texture::generateMipmaps() {

    if (mipLevelCount_ == 1)
        return;

    auto cmdBuf = VkUtils::beginSingleTimeCommand();

    //  transition all mip levels to transfer dst optimal, then in the loop, transition appropriate ones to src optimal
    transitionLayout(vk::ImageLayout::eTransferDstOptimal,vk::PipelineStageFlagBits2::eTransfer,vk::AccessFlagBits2::eTransferWrite,cmdBuf,{0,mipLevelCount_});

    //  cast to int here (VkOffset3D expects ints)
    int mipWidth = static_cast<int>(width_);
    int mipHeight = static_cast<int>(height_);


    for (uint32_t dstMipLevel = 1; dstMipLevel < mipLevelCount_; ++dstMipLevel) {
        uint32_t srcMipLevel = dstMipLevel-1;

        // transition src mip level to transfer src optimal
        transitionLayout(vk::ImageLayout::eTransferSrcOptimal,vk::PipelineStageFlagBits2::eTransfer,vk::AccessFlagBits2::eTransferRead,cmdBuf,{srcMipLevel,1});

        std::array srcOffsets{
            vk::Offset3D{.x = 0,.y = 0,.z = 0},
            vk::Offset3D{.x = mipWidth,.y = mipHeight,.z = 1}
        };

        std::array dstOffsets{
            vk::Offset3D{.x = 0,.y = 0,.z = 0},
            vk::Offset3D{.x = mipWidth > 1 ? mipWidth / 2 : 1, .y = mipHeight  > 1 ? mipHeight / 2 : 1, .z = 1}
        };


        vk::ImageBlit region{
            .srcSubresource = {
                .aspectMask = aspectFlags_,
                .mipLevel = srcMipLevel,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .srcOffsets = srcOffsets,
            .dstSubresource = {
                .aspectMask = aspectFlags_,
                .mipLevel = dstMipLevel,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .dstOffsets = dstOffsets
        };

        VkUtils::blit(cmdBuf,imageAlloc_.image,imageAlloc_.image,region,vk::Filter::eLinear);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;

        // transition src mip level back to shader read optimal
        transitionLayout(vk::ImageLayout::eShaderReadOnlyOptimal,vk::PipelineStageFlagBits2::eFragmentShader,vk::AccessFlagBits2::eShaderRead,cmdBuf,{srcMipLevel,1});
    }

    // transition the last mip level back to shader read optimal, since it is not included in the loop
    transitionLayout(vk::ImageLayout::eShaderReadOnlyOptimal,vk::PipelineStageFlagBits2::eFragmentShader,vk::AccessFlagBits2::eShaderRead,cmdBuf,{mipLevelCount_-1,1});

    VkUtils::endSingleTimeCommand(cmdBuf,VkUtils::QueueType::graphics);
}

void Texture::transitionLayout(vk::ImageLayout newLayout, vk::PipelineStageFlags2 stage, vk::AccessFlags2 accessFlags,
                               vk::raii::CommandBuffer& cmdBuf, const VkUtils::TransitionMipInfo& mipInfo) {

    if (mipInfo.baseLevel + mipInfo.levelCount > mipLevelCount_)
        throw std::exception("ERROR: trying to transition nonexistent mip level!");


    for (uint32_t i = mipInfo.baseLevel; i < mipInfo.baseLevel + mipInfo.levelCount; ++i) {
        VkUtils::transitionImageLayout(imageAlloc_.image,
                                   mipLayouts_[i],
                                   newLayout,
                                   mipStageMasks_[i],
                                   mipAccessMasks_[i],
                                   stage,
                                   accessFlags,
                                   aspectFlags_,
                                   cmdBuf,
                                   {i,1});
        mipLayouts_[i] = newLayout;
        mipStageMasks_[i] = stage;
        mipAccessMasks_[i] = accessFlags;
    }
}


vk::Format Texture::chooseVkFormat(bool isSrgb) const {
    switch (freeImageType_) {
        case FIT_BITMAP:
            switch (channelCount_) {
                case 1:
                    return isSrgb ? vk::Format::eR8Srgb : vk::Format::eR8Unorm;
                case 2:
                    return isSrgb ? vk::Format::eR8G8Srgb : vk::Format::eR8G8Unorm;
                case 3:
                    return isSrgb ? vk::Format::eB8G8R8Srgb : vk::Format::eB8G8R8Unorm;
                case 4:
                    return isSrgb ? vk::Format::eB8G8R8A8Srgb: vk::Format::eB8G8R8A8Unorm;
                default:
                    throw std::runtime_error("ERROR: Unsupported format type!");
            }
        case FIT_RGB16:
            return vk::Format::eR16G16B16Sfloat;
        case FIT_RGBA16:
            return  vk::Format::eR16G16B16A16Sfloat;
        case FIT_RGBF:
            return vk::Format::eR32G32B32Sfloat;
        case FIT_RGBAF:
            return vk::Format::eR32G32B32A32Sfloat;
        default:
            throw std::runtime_error("ERROR: Unsupported format type!");
    }
}

int Texture::getChannelCount(FREE_IMAGE_TYPE type, uint32_t bpp) {
    switch(type) {
        case FIT_BITMAP: {
            if (bpp == 8)  return 1;
            if (bpp == 24) return 3;
            if (bpp == 32) return 4;
            return 0; // unsupported format
        }
        case FIT_RGB16:
        case FIT_RGBF:
            return 3;
        case FIT_RGBA16:
        case FIT_RGBAF:
            return 4;
        default:
            return 0;
    }
}

int Texture::chooseChannelCount(vk::Format format) {
    using F = vk::Format;
    switch (format) {
        case F::eR8Unorm:
        case F::eR8Snorm:
        case F::eR8Uscaled:
        case F::eR8Sscaled:
        case F::eR8Uint:
        case F::eR8Sint:
        case F::eR8Srgb:
        case F::eR16Unorm:
        case F::eR16Snorm:
        case F::eR16Uint:
        case F::eR16Sint:
        case F::eR16Sfloat:
        case F::eR32Uint:
        case F::eR32Sint:
        case F::eR32Sfloat:
        case F::eR64Uint:
        case F::eR64Sint:
        case F::eR64Sfloat:
        case F::eD16Unorm:
        case F::eX8D24UnormPack32:
        case F::eD32Sfloat:
        case F::eS8Uint:
            return 1;

        case F::eR8G8Unorm:
        case F::eR8G8Snorm:
        case F::eR8G8Uscaled:
        case F::eR8G8Sscaled:
        case F::eR8G8Uint:
        case F::eR8G8Sint:
        case F::eR8G8Srgb:
        case F::eR16G16Unorm:
        case F::eR16G16Snorm:
        case F::eR16G16Uint:
        case F::eR16G16Sint:
        case F::eR16G16Sfloat:
        case F::eR32G32Uint:
        case F::eR32G32Sint:
        case F::eR32G32Sfloat:
        case F::eR64G64Uint:
        case F::eR64G64Sint:
        case F::eR64G64Sfloat:
        case F::eD16UnormS8Uint:
        case F::eD24UnormS8Uint:
        case F::eD32SfloatS8Uint:
            return 2;

        case F::eR8G8B8Unorm:
        case F::eR8G8B8Snorm:
        case F::eR8G8B8Uscaled:
        case F::eR8G8B8Sscaled:
        case F::eR8G8B8Uint:
        case F::eR8G8B8Sint:
        case F::eR8G8B8Srgb:
        case F::eB8G8R8Unorm:
        case F::eB8G8R8Snorm:
        case F::eB8G8R8Uscaled:
        case F::eB8G8R8Sscaled:
        case F::eB8G8R8Uint:
        case F::eB8G8R8Sint:
        case F::eB8G8R8Srgb:
        case F::eR16G16B16Unorm:
        case F::eR16G16B16Snorm:
        case F::eR16G16B16Uint:
        case F::eR16G16B16Sint:
        case F::eR16G16B16Sfloat:
        case F::eR32G32B32Uint:
        case F::eR32G32B32Sint:
        case F::eR32G32B32Sfloat:
        case F::eB10G11R11UfloatPack32:
            return 3;

        case F::eR8G8B8A8Unorm:
        case F::eR8G8B8A8Snorm:
        case F::eR8G8B8A8Uscaled:
        case F::eR8G8B8A8Sscaled:
        case F::eR8G8B8A8Uint:
        case F::eR8G8B8A8Sint:
        case F::eR8G8B8A8Srgb:
        case F::eB8G8R8A8Unorm:
        case F::eB8G8R8A8Snorm:
        case F::eB8G8R8A8Uscaled:
        case F::eB8G8R8A8Sscaled:
        case F::eB8G8R8A8Uint:
        case F::eB8G8R8A8Sint:
        case F::eB8G8R8A8Srgb:
        case F::eA8B8G8R8UnormPack32:
        case F::eA8B8G8R8SnormPack32:
        case F::eA8B8G8R8UintPack32:
        case F::eA8B8G8R8SintPack32:
        case F::eA8B8G8R8SrgbPack32:
        case F::eA2R10G10B10UnormPack32:
        case F::eA2R10G10B10UintPack32:
        case F::eA2B10G10R10UnormPack32:
        case F::eA2B10G10R10UintPack32:
        case F::eR16G16B16A16Unorm:
        case F::eR16G16B16A16Snorm:
        case F::eR16G16B16A16Uint:
        case F::eR16G16B16A16Sint:
        case F::eR16G16B16A16Sfloat:
        case F::eR32G32B32A32Uint:
        case F::eR32G32B32A32Sint:
        case F::eR32G32B32A32Sfloat:
            return 4;

        default:
            throw std::runtime_error("ERROR: Unsupported vk::Format in chooseChannelCount!");
    }
}

Texture::~Texture() {
    VkUtils::destroyImageVMA(std::move(imageAlloc_));
}
