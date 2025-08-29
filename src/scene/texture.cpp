//
// Created by Tonz on 23.07.2025.
//

#include "texture.h"
#include <iostream>

#include "Vertex.h"
#include "../engine/engine.h"
#include "../engine/utils.h"
#include "../engine/managers/resourceManager.h"

std::shared_ptr<Texture> Texture::createDummy(std::string_view name,  const glm::vec<4, uint8_t>& color) {

    vk::ImageUsageFlags usageFlags = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    auto dummy = TextureManager::getInstance()->registerResource(name,1,1,4,vk::Format::eB8G8R8A8Unorm, usageFlags, vk::MemoryPropertyFlagBits::eDeviceLocal);

    memcpy(dummy->data_.data(),&color[0],sizeof(color));

    VkUtils::BufferAlloc stagingBuffer = VkUtils::createBufferVMA(dummy->getTotalSize(),vk::BufferUsageFlagBits::eTransferSrc,VkUtils::stagingAllocFlagsVMA);
    dummy->stage(stagingBuffer);
    VkUtils::destroyBufferVMA(std::move(stagingBuffer));

    return dummy;
}


Texture::Texture(uint32_t width, uint32_t height, uint32_t channels, vk::Format format, vk::ImageUsageFlags imageUsage, vk::MemoryPropertyFlags memoryProperties):
    ManagedResource(), width_(width), height_(height), channelCount_(channels), vkFormat_(format), imageUsageFlags_(imageUsage), memoryPropertyFlags_(memoryProperties)
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
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = imageUsageFlags_,
        .sharingMode = vk::SharingMode::eExclusive
    };


    imageAlloc_ = VkUtils::createImageVMA(imageInfo);

    vk::ImageAspectFlags aspectFlags{vk::ImageAspectFlagBits::eColor};
    if (imageUsageFlags_ & vk::ImageUsageFlagBits::eDepthStencilAttachment)
        aspectFlags = vk::ImageAspectFlagBits::eDepth;

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
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
    };

    const auto& device = Engine::getInstance().getDevice();

    vkImageView_ = vk::raii::ImageView(device, imageViewCreateInfo);

    vk::SamplerCreateInfo samplerInfo = {
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode  = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .mipLodBias = 0.0f,
        .anisotropyEnable = vk::True,
        .maxAnisotropy = Engine::getInstance().getDeviceLimits().maxSamplerAnisotropy,
        .compareEnable = vk::False,
        .compareOp =   vk::CompareOp::eAlways,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
        .unnormalizedCoordinates = vk::False
    };

    vkSampler_ = vk::raii::Sampler(device,samplerInfo);
}


Texture::Texture(std::string_view fileName) : ManagedResource() {
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

                memoryPropertyFlags_ = vk::MemoryPropertyFlagBits::eDeviceLocal;
                imageUsageFlags_ = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;

                vkFormat_ = chooseVkFormat();
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

void Texture::expand() {
    #pragma omp parallel for collapse(2)
    for (int x = 0; x < static_cast<int>(width_) ; ++x) {
        for (int y = 0; y < static_cast<int>(height_) ; ++y) {

            const uint32_t offset_pixel = y * scanWidth_ + x * pixelSize_;

            for (uint32_t c = 0; c < pixelSize_; ++c){
                const uint32_t total_offset = offset_pixel + c;

                const uint8_t pixel_b = data_[total_offset];
                float pixel_f = static_cast<float>(pixel_b) / 255.0f;

                pixel_f = Utils::expand(pixel_f);

                data_[total_offset] = static_cast<uint8_t>(pixel_f * 255.0f);
            }
        }
    }
}

void Texture::stage(const VkUtils::BufferAlloc& stagingBuffer) const {

    if (stagingBuffer.allocationInfo.pMappedData == nullptr)
        throw std::runtime_error("ERROR: Mapped pointer points to NULL!");

    size_t imageSize = width_ * height_ * pixelSize_;

    memcpy(stagingBuffer.allocationInfo.pMappedData,data_.data(),imageSize);

    auto cmdBuf = VkUtils::beginSingleTimeCommand();
    VkUtils::transitionImageLayout(
        imageAlloc_.image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        vk::PipelineStageFlagBits2::eTopOfPipe,
        vk::AccessFlagBits2::eNone,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferWrite,
        vk::ImageAspectFlagBits::eColor,
        cmdBuf
    );

    VkUtils::copyBufferToImage(stagingBuffer,imageAlloc_,width_,height_, cmdBuf);

    VkUtils::transitionImageLayout(
       imageAlloc_.image,
       vk::ImageLayout::eTransferDstOptimal,
       vk::ImageLayout::eShaderReadOnlyOptimal,
       vk::PipelineStageFlagBits2::eTransfer,
       vk::AccessFlagBits2::eTransferWrite,
       vk::PipelineStageFlagBits2::eFragmentShader,
       vk::AccessFlagBits2::eShaderRead,
       vk::ImageAspectFlagBits::eColor,
       cmdBuf
   );

    VkUtils::endSingleTimeCommand(cmdBuf,VkUtils::QueueType::graphics);
}


vk::Format Texture::chooseVkFormat() const {
    switch (freeImageType_) {
        case FIT_BITMAP:
            switch (channelCount_) {
                case 1:
                    return vk::Format::eR8Srgb;
                case 2:
                    return vk::Format::eR8G8Srgb;
                case 3:
                    return vk::Format::eB8G8R8Srgb;
                case 4:
                    return vk::Format::eB8G8R8A8Srgb;
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


Texture::~Texture() {
    VkUtils::destroyImageVMA(std::move(imageAlloc_));
}
