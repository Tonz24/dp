//
// Created by Tonz on 23.07.2025.
//

#include "texture.h"
#include <iostream>

#include "engine.h"
#include "../engine/utils.h"
#include "vkUtils.h"

void Texture::initDummy() {

    dummy_ = new Texture(1,1,4,vk::Format::eB8G8R8A8Unorm, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal);

    dummy_->data_[0] = 255;
    dummy_->data_[1] = 0;
    dummy_->data_[2] = 255;
    dummy_->data_[3] = 255;

    dummy_->uploadToDevice();
}


Texture::Texture(uint32_t width, uint32_t height, uint32_t channels, vk::Format format, vk::ImageUsageFlags imageUsage, vk::MemoryPropertyFlags memoryProperties):
    ManagedResource(), width_(width), height_(height), channels_(channels), vkFormat_(format), imageUsageFlags_(imageUsage), memoryPropertyFlags_(memoryProperties)
{
    data_.reserve(width_ * height_ * channels_);
    data_.resize(width_ * height_ * channels_,0);
    initVkImage();

}

void Texture::initVkImage() {

    pixelSize_ = channels_;

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

    const auto& device = Engine::getInstance().getDevice();


    vkImage_ = vk::raii::Image(device, imageInfo);
    auto idMapMemReqs = vkImage_.getMemoryRequirements();

    vk::MemoryAllocateInfo memAllocInfo{
        .allocationSize = idMapMemReqs.size,
        .memoryTypeIndex = VkUtils::findMemoryType(idMapMemReqs.memoryTypeBits,memoryPropertyFlags_),
    };
    vkImageMemory_ = vk::raii::DeviceMemory(device,memAllocInfo);
    vkImage_.bindMemory(vkImageMemory_,0);

    vk::ImageAspectFlags aspectFlags{vk::ImageAspectFlagBits::eColor};
    if (imageUsageFlags_ & vk::ImageUsageFlagBits::eDepthStencilAttachment)
        aspectFlags = vk::ImageAspectFlagBits::eDepth;

    vk::ImageViewCreateInfo imageViewCreateInfo{
        .flags = vk::ImageViewCreateFlags(),
        .image = vkImage_,
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

    setResourceName(correctFileName);

    FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(correctFileName.c_str(), 0);
    if (fif == FIF_UNKNOWN)
        fif = FreeImage_GetFIFFromFilename(correctFileName.c_str());

    if ( fif != FIF_UNKNOWN){
        FIBITMAP* tempBitmap = FreeImage_Load(fif,correctFileName.c_str());
        FIBITMAP* bitmap = FreeImage_ConvertTo32Bits(tempBitmap);
        FreeImage_Unload(tempBitmap);

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
                channels_ = pixelSize_;

                data_.reserve(scanWidth_ * height_);
                data_.resize(scanWidth_ * height_,0);
                
                FreeImage_ConvertToRawBits(data_.data(),bitmap,scanWidth_,pixelSize_ * 8, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, FALSE);
                isFromDisk_ = true;

                memoryPropertyFlags_ = vk::MemoryPropertyFlagBits::eDeviceLocal;
                imageUsageFlags_ = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;

                assignVkFormat();
                initVkImage();
                uploadToDevice();
            }
        }
        FreeImage_Unload(bitmap);

    }
    else{
        std::cerr << "ERROR! Failed to load texture " << fileName << std::endl;
        exit(EXIT_FAILURE);
    }

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

void Texture::uploadToDevice() {

    size_t imageSize = width_ * height_ * pixelSize_;

    auto stagingBuffer = VkUtils::createBuffer(imageSize,vk::BufferUsageFlagBits::eTransferSrc,VkUtils::stagingMemoryFlags);

    void* bufferData = stagingBuffer.memory.mapMemory(0,imageSize);
    memcpy(bufferData,data_.data(),imageSize);
    stagingBuffer.memory.unmapMemory(); //  put image pixel data into the mapped staging buffer

    auto cmdBuf = VkUtils::beginSingleTimeCommand();
    VkUtils::transitionImageLayout(
        vkImage_,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        vk::PipelineStageFlagBits2::eTopOfPipe,
        vk::AccessFlagBits2::eNone,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferWrite,
        vk::ImageAspectFlagBits::eColor,
        cmdBuf
    );

    VkUtils::copyBufferToImage(stagingBuffer.buffer,vkImage_,width_,height_, cmdBuf);

    VkUtils::transitionImageLayout(
       vkImage_,
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


void Texture::assignVkFormat() {
    switch (freeImageType_) {
        case FIT_BITMAP:
            switch (channels_) {
                case 1:
                    vkFormat_ = vk::Format::eR8Srgb;
                    return;
                case 2:
                    vkFormat_ = vk::Format::eR8G8Srgb;
                    return;
                case 3:
                    vkFormat_ = vk::Format::eB8G8R8Srgb;
                    return;
                case 4:
                    vkFormat_ = vk::Format::eB8G8R8A8Srgb;
                    return;
                default:
                    std::cout << "ERROR! Unsupported format type" << std::endl;
                    exit(EXIT_FAILURE);
            }
        default:
            std::cout << "ERROR! Unsupported format type" << std::endl;
            exit(EXIT_FAILURE);
    }
}

void Texture::initImageViewAndSampler() {
}

