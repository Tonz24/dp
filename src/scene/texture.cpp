//
// Created by Tonz on 23.07.2025.
//

#include "texture.h"
#include <iostream>
#include <numeric>
#include <glm/ext/scalar_constants.hpp>
#include <glm/detail/type_half.hpp>

#include "vertex.h"
#include "../engine/engine.h"
#include "../engine/managers/resourceManager.h"


// std::shared_ptr<Texture> Texture::createDummy(std::string_view name,  const glm::vec<4, uint8_t>& color) {
//
//     vk::ImageUsageFlags usageFlags = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
//     auto dummy = TextureManager::getInstance()->registerResource(name,1,1,vk::Format::eB8G8R8A8Unorm, usageFlags);
//
//     memcpy(dummy->data_.data(),&color[0],sizeof(color));
//
//     VkUtils::BufferAlloc stagingBuffer = VkUtils::createBufferVMA(dummy->getTotalSize(),vk::BufferUsageFlagBits::eTransferSrc,VkUtils::stagingAllocFlagsVMA);
//     dummy->stage(stagingBuffer);
//     VkUtils::destroyBufferVMA(std::move(stagingBuffer));
//
//     return dummy;
// }

Texture::Texture(uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags imageUsage,vk::SampleCountFlagBits sampleCount, bool populateData):
    ManagedResource(), width_(width), height_(height), channelCount_(getChannelCount(format)), vkFormat_(format), sampleCount_(sampleCount), imageUsageFlags_(imageUsage), pixelSize_(getFormatPixelSize(format)), scanWidth_(width_ * pixelSize_)
{

    if (populateData) {
        data_.reserve(width_ * height_ * pixelSize_);
        data_.resize(width_ * height_ * pixelSize_,0);
    }

    initVkImage();
}

std::shared_ptr<Texture> Texture::getCdf() {
    std::vector imgScalar(width_ * height_,0.0f);

    // if the CDF already exists, return it
    auto cdfTexture = TextureManager::getInstance()->getResource(getCdfName());
    if (cdfTexture != nullptr)
        return cdfTexture;

    cdfTexture = TextureManager::getInstance()->registerResource(getCdfName(),width_ + 1, height_,vk::Format::eR32Sfloat, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst, vk::SampleCountFlagBits::e1, true);

    std::vector marginalSum(height_,0.0f);

    double sumTotal{0.0};

    # pragma omp parallel for reduction(+:sumTotal)
    //  compute luminance values for every pixel
    for (int y = 0; y < height_; y++) {
        //  get vertical texture coordinate in [0,1] (offset by 0.5 to sample pixel centers)
        //  sinTheta corrects for the fact that sphere poles would otherwise get oversampled
        float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(height_);
        float sinTheta = glm::sin(glm::pi<float>() * v);

        for (int x = 0; x < width_; ++x) {

            glm::vec3 rgb{0.0f};
            if (freeImageFormat_ == FIF_JPEG || freeImageFormat_ == FIF_PNG || freeImageFormat_ == FIF_BMP)
                rgb = static_cast<glm::vec<3,float>>(getTexel<glm::vec<4,uint8_t>>(x, y)) / 255.0f;

            if (freeImageFormat_ == FIF_HDR || freeImageFormat_ == FIF_EXR ) {
                uint32_t dataOffset = y * scanWidth_ + x * pixelSize_;

                // convert float16 pixel to float32 rgb data
                uint16_t rgbData[3] {};
                memcpy(&rgbData, data_.data() + dataOffset , sizeof(rgbData));

                rgb = {
                    glm::detail::toFloat32(rgbData[0]),
                    glm::detail::toFloat32(rgbData[1]),
                    glm::detail::toFloat32(rgbData[2])
                };
            }

            // opencv formula
            rgb *= glm::vec3( 0.299,0.587, 0.114);
            float luminance = rgb.x + rgb.y + rgb.z;

            // set a minimum value for black pixels, otherwise they can't be sampled at all
            // that would result in incorrect estimation where bilinear interpolation should return nonzero radiance when sampling such pixels
            float weightedLuminance = glm::max(luminance * sinTheta, 1e-5f * sinTheta);

            imgScalar[y * width_ + x] = weightedLuminance; // insert weighted luminance into scalar image
            sumTotal += weightedLuminance; // accumulate total sum of scalar values

            marginalSum[y] += weightedLuminance; // accumulate sum for every row
        }
    }

    auto marginalSumNorm = marginalSum;
    // normalize the marginal sum
    #pragma omp parallel for
    for (int i = 0; i < height_; ++i) {
        marginalSumNorm[i] = static_cast<float>(marginalSum[i] / sumTotal);
    }
    std::vector marginalCdf(height_,0.0f);

    // build the marginal CDF -- accumulate sums into marginalCdf
    std::partial_sum(marginalSumNorm.begin(),marginalSumNorm.end(),marginalCdf.begin(),std::plus());

    // build conditional cdfs
    std::vector conditionalCdf(width_ * height_,0.0f);

    // normalize every pixel by corresponding row sum
    #pragma omp parallel for
    for (int y = 0; y < height_; y++) {
        for (int x = 0; x < width_; ++x) {
            conditionalCdf[y * width_ + x] = imgScalar[y * width_ + x] / marginalSum[y];
        }
    }

    // build conditional cdf for each row
    #pragma omp parallel for
    for (int y = 0; y < height_; y++) {
        for (int x = 1; x < width_; ++x) {
            conditionalCdf[y * width_ + x] += conditionalCdf[y * width_ + (x - 1)];
        }
    }

    float* dataFloat = reinterpret_cast<float*>(cdfTexture->data_.data());
    #pragma omp parallel for
    for (int y = 0; y < cdfTexture->getHeight(); y++) {
        for (int x = 0; x < cdfTexture->getWidth(); ++x) {
            float val = x == 0 ? marginalCdf[y] : conditionalCdf[y * width_ + (x - 1)];
            dataFloat[y * cdfTexture->getWidth() + x] = val;
        }
    }

    // put cdf image data to device local memory
    VkUtils::BufferAlloc stagingBuffer = VkUtils::createBufferVMA(cdfTexture->getTotalSize(),vk::BufferUsageFlagBits::eTransferSrc,VkUtils::stagingAllocFlagsVMA);
    cdfTexture->stage(stagingBuffer);
    VkUtils::destroyBufferVMA(std::move(stagingBuffer));

    return cdfTexture;
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
        .samples = sampleCount_,
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

    // get image format
    if (fif == FIF_UNKNOWN)
        fif = FreeImage_GetFIFFromFilename(correctFileName.c_str());
    if (fif == FIF_UNKNOWN)
            throw std::runtime_error("ERROR! Failed to load texture " + std::string{fileName} + " due to unknown format");

    // get image data
    FIBITMAP* bitmap = FreeImage_Load(fif,correctFileName.c_str());
    if (bitmap == nullptr)
        throw std::runtime_error("ERROR! could not open texture " + std::string{fileName});

    freeImageFormat_ = fif;
    freeImageType_ = FreeImage_GetImageType(bitmap);
    width_ = FreeImage_GetWidth(bitmap);
    height_ = FreeImage_GetHeight(bitmap);

    if (width_ == 0 || height_ == 0)
        throw std::runtime_error("ERROR! Texture " + std::string{fileName} + " has zero width or height!");

    if (fif == FIF_EXR || fif == FIF_HDR)
        initializeEnvMap(bitmap);
    else
        initializeTexture(bitmap, isSrgb);

    isFromDisk_ = true;
    //  transfer src for generating mipmaps
    imageUsageFlags_ = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc;

    if (generateMipmaps)
        mipLevelCount_ = static_cast<uint32_t>(std::floor(std::log2(std::max(width_, height_)))) + 1;

    initVkImage();

    FreeImage_Unload(bitmap);
    FreeImage_DeInitialise();
}


void Texture::initializeEnvMap(FIBITMAP* bitmap) {

    channelCount_ = 4;
    elementSize_ = 2;  // two bytes per one half float
    pixelSize_ = channelCount_ * elementSize_;
    scanWidth_ = width_ * pixelSize_;
    vkFormat_ = vk::Format::eR16G16B16A16Sfloat;

    data_.reserve(scanWidth_ * height_);
    data_.resize(scanWidth_ * height_,0);

    // set bitmap values manually
    #pragma omp parallel for collapse(2)
    for (int y = 0; y < height_; ++y) {
        auto srcScan = reinterpret_cast<FIRGBF*>(FreeImage_GetScanLine(bitmap, y));

        for (int x = 0; x < width_; ++x) {
            uint32_t pixelOffset = y * scanWidth_ + x * pixelSize_;

            // convert pixel data to float16 manually
            // IMPORTANT: clamp them to a safe max so that they don't turn into inf or nan
            short pixelData[4] = {
                glm::detail::toFloat16(glm::clamp(srcScan[x].red,0.0f,65500.0f)),
                glm::detail::toFloat16(glm::clamp(srcScan[x].green,0.0f,65500.0f)),
                glm::detail::toFloat16(glm::clamp(srcScan[x].blue,0.0f,65500.0f)),
                glm::detail::toFloat16(1.0f)
            };

            memcpy(data_.data() + pixelOffset, pixelData, sizeof(pixelData));
        }
    }
}

void Texture::initializeTexture(FIBITMAP* bitmap, bool isSrgb) {
    auto bitmapConverted = FreeImage_ConvertTo32Bits(bitmap);

    pixelSize_ = FreeImage_GetBPP(bitmapConverted) / 8;
    scanWidth_ = FreeImage_GetPitch(bitmapConverted);
    channelCount_ = getChannelCount(freeImageType_,FreeImage_GetBPP(bitmapConverted));
    vkFormat_ = chooseVkFormat(isSrgb);

    data_.reserve(scanWidth_ * height_);
    data_.resize(scanWidth_ * height_,0);

    FreeImage_ConvertToRawBits(data_.data(),bitmapConverted,scanWidth_,pixelSize_ * 8, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, FALSE);
    FreeImage_Unload(bitmapConverted);
}


std::string Texture::getResourceType() const {
    return "Texture";
}

void Texture::stage(const VkUtils::BufferAlloc& stagingBuffer) {

    if (stagingBuffer.allocationInfo.pMappedData == nullptr)
        throw std::runtime_error("ERROR: Mapped pointer points to NULL!");

    size_t imageSize = width_ * height_ * pixelSize_;

    if (stagingBuffer.allocationInfo.size < imageSize)
        throw std::runtime_error("ERROR: staging buffer is too small for texture data!");

    memcpy(stagingBuffer.allocationInfo.pMappedData,data_.data(),imageSize);

    auto cmdBuf = VkUtils::beginSingleTimeCommand();

    transitionLayout(vk::ImageLayout::eTransferDstOptimal,vk::PipelineStageFlagBits2::eTransfer,vk::AccessFlagBits2::eTransferWrite,cmdBuf, {0,mipLevelCount_});

    VkUtils::copyBufferToImage(stagingBuffer,imageAlloc_,width_,height_, cmdBuf);

    // storage images should not return to shader read only, but general
    vk::ImageLayout returnLayout = imageUsageFlags_ & vk::ImageUsageFlagBits::eStorage ? vk::ImageLayout::eGeneral : vk::ImageLayout::eShaderReadOnlyOptimal;

    transitionLayout(returnLayout,vk::PipelineStageFlagBits2::eFragmentShader,vk::AccessFlagBits2::eShaderRead,cmdBuf, {0,mipLevelCount_});

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
        throw std::runtime_error("ERROR: trying to transition nonexistent mip level!");

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

void Texture::copyToHost() {
    vk::DeviceSize bufferSize = width_ * height_ * pixelSize_;
    auto stagingBuffer = VkUtils::createBufferVMA(bufferSize,vk::BufferUsageFlagBits::eTransferDst,VkUtils::stagingAllocFlagsVMA);


    auto cmdBuf =  VkUtils::beginSingleTimeCommand();
    transitionLayout(vk::ImageLayout::eTransferSrcOptimal,vk::PipelineStageFlagBits2::eTransfer,vk::AccessFlagBits2::eTransferRead,cmdBuf,{0,mipLevelCount_});
    VkUtils::copyImageToBuffer(imageAlloc_, stagingBuffer,0,width_,0,height_,cmdBuf);
    VkUtils::endSingleTimeCommand(cmdBuf,VkUtils::QueueType::graphics);

    data_.clear();
    data_.resize(bufferSize);

    memcpy(data_.data(),stagingBuffer.allocationInfo.pMappedData,bufferSize);
    VkUtils::destroyBufferVMA(std::move(stagingBuffer));
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

int Texture::getChannelCount(vk::Format format) {
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

uint32_t Texture::getFormatPixelSize(vk::Format format) {
    switch (format) {
        // 8-bit per channel formats
        case vk::Format::eR8Unorm:
        case vk::Format::eR8Snorm:
        case vk::Format::eR8Uint:
        case vk::Format::eR8Sint:
        case vk::Format::eR8Srgb:
            return 1;

        case vk::Format::eR8G8Unorm:
        case vk::Format::eR8G8Snorm:
        case vk::Format::eR8G8Uint:
        case vk::Format::eR8G8Sint:
        case vk::Format::eR8G8Srgb:
            return 2;

        case vk::Format::eR8G8B8Unorm:
        case vk::Format::eR8G8B8Snorm:
        case vk::Format::eR8G8B8Uint:
        case vk::Format::eR8G8B8Sint:
        case vk::Format::eR8G8B8Srgb:
        case vk::Format::eB8G8R8Unorm:
        case vk::Format::eB8G8R8Snorm:
        case vk::Format::eB8G8R8Uint:
        case vk::Format::eB8G8R8Sint:
        case vk::Format::eB8G8R8Srgb:
            return 3;

        case vk::Format::eR8G8B8A8Unorm:
        case vk::Format::eR8G8B8A8Snorm:
        case vk::Format::eR8G8B8A8Uint:
        case vk::Format::eR8G8B8A8Sint:
        case vk::Format::eR8G8B8A8Srgb:
        case vk::Format::eB8G8R8A8Unorm:
        case vk::Format::eB8G8R8A8Snorm:
        case vk::Format::eB8G8R8A8Uint:
        case vk::Format::eB8G8R8A8Sint:
        case vk::Format::eB8G8R8A8Srgb:
            return 4;

        // 16-bit per channel formats
        case vk::Format::eR16Unorm:
        case vk::Format::eR16Snorm:
        case vk::Format::eR16Uint:
        case vk::Format::eR16Sint:
        case vk::Format::eR16Sfloat:
            return 2;

        case vk::Format::eR16G16Unorm:
        case vk::Format::eR16G16Snorm:
        case vk::Format::eR16G16Uint:
        case vk::Format::eR16G16Sint:
        case vk::Format::eR16G16Sfloat:
        case vk::Format::eB10G11R11UfloatPack32:
            return 4;

        case vk::Format::eR16G16B16Unorm:
        case vk::Format::eR16G16B16Snorm:
        case vk::Format::eR16G16B16Uint:
        case vk::Format::eR16G16B16Sint:
        case vk::Format::eR16G16B16Sfloat:
            return 6;

        case vk::Format::eR16G16B16A16Unorm:
        case vk::Format::eR16G16B16A16Snorm:
        case vk::Format::eR16G16B16A16Uint:
        case vk::Format::eR16G16B16A16Sint:
        case vk::Format::eR16G16B16A16Sfloat:
            return 8;

        // 32-bit per channel formats
        case vk::Format::eR32Uint:
        case vk::Format::eR32Sint:
        case vk::Format::eR32Sfloat:
            return 4;

        case vk::Format::eR32G32Uint:
        case vk::Format::eR32G32Sint:
        case vk::Format::eR32G32Sfloat:
            return 8;

        case vk::Format::eR32G32B32Uint:
        case vk::Format::eR32G32B32Sint:
        case vk::Format::eR32G32B32Sfloat:
            return 12;

        case vk::Format::eR32G32B32A32Uint:
        case vk::Format::eR32G32B32A32Sint:
        case vk::Format::eR32G32B32A32Sfloat:
            return 16;

        // Depth / stencil formats
        case vk::Format::eD16Unorm:
            return 2;
        case vk::Format::eX8D24UnormPack32:
        case vk::Format::eD32Sfloat:
            return 4;
        case vk::Format::eD24UnormS8Uint:
        case vk::Format::eD32SfloatS8Uint:
            return 5;

        default:
            throw std::runtime_error("Unsupported or compressed vk::Format in GetFormatPixelSize");
    }
}

Texture::~Texture() {
    data_.clear();
    VkUtils::destroyImageVMA(std::move(imageAlloc_));
}
