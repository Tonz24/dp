//
// Created by Tonz on 23.07.2025.
//

#include "texture.h"
#include <iostream>
#include <omp.h>

#include "../engine.h"

Texture::~Texture() {
    delete[] data_;
}

Texture::Texture(uint32_t width, uint32_t height, uint32_t channels, vk::Format format): ManagedResource() {
    fileName_ = std::string{"texture"}.append(std::to_string(categoryId_));

    //TODO: initialize blank texture
    renameOnGenerate = true;
    std::cerr << "NOT IMPLEMENTED!" << std::endl;
    exit(EXIT_FAILURE);
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
                data_ = new uint8_t[scanWidth_ * height_];
                FreeImage_ConvertToRawBits(data_,bitmap,scanWidth_,pixelSize_ * 8, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, FALSE);
                isFromDisk_ = true;
                assignVkFormat();
            }
        }
        FreeImage_Unload(bitmap);

        uploadToDevice();
        initImageViewAndSampler();
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

                const BYTE pixel_b = data_[total_offset];
                float pixel_f = static_cast<float>(pixel_b) / 255.0f;

                pixel_f = Utils::expand(pixel_f);

                data_[total_offset] = static_cast<BYTE>(pixel_f * 255.0f);
            }
        }
    }
}

void Texture::uploadToDevice() {

    size_t imageSize = width_ * height_ * pixelSize_;

    //  initialize local staging buffer
    vk::raii::Buffer stagingBuffer{nullptr};
    vk::raii::DeviceMemory stagingMemory{nullptr};

    Engine::getInstance().createBuffer(
        imageSize,stagingBuffer,
        vk::BufferUsageFlagBits::eTransferSrc,stagingMemory,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* bufferData = stagingMemory.mapMemory(0,imageSize);
    memcpy(bufferData,data_,imageSize);
    stagingMemory.unmapMemory(); //  put image pixel data into the mapped staging buffer

    //  create the image object
    vk::ImageCreateInfo imageCreateInfo{
        .imageType = vk::ImageType::e2D,
        .format = vkFormat_,
        .extent = {width_, height_, 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        .sharingMode = vk::SharingMode::eExclusive
    };
    vkImage_ = vk::raii::Image(Engine::getInstance().getDevice(),imageCreateInfo);

    auto memRequirements = vkImage_.getMemoryRequirements();
    auto allocInfo = vk::MemoryAllocateInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = Engine::getInstance().findMemoryType(memRequirements.memoryTypeBits,vk::MemoryPropertyFlagBits::eDeviceLocal)
    };
    vkImageMemory_ = vk::raii::DeviceMemory(Engine::getInstance().getDevice(),allocInfo);
    vkImage_.bindMemory(vkImageMemory_,0);

    //  transition to dst optimal because the image will be written to from the staging buffer
    transitionImageLayout(vk::ImageLayout::eUndefined,vk::ImageLayout::eTransferDstOptimal);

    Engine::getInstance().copyBufferToImage(stagingBuffer,vkImage_,width_,height_);

    //  transition to shader read only optimal layout from transfer dst optimal
    transitionImageLayout(vk::ImageLayout::eTransferDstOptimal,vk::ImageLayout::eShaderReadOnlyOptimal);
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

void Texture::transitionImageLayout(vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
    auto cmdBuf = Engine::getInstance().beginSingleTimeCommand();

    vk::ImageMemoryBarrier barrier{
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = vkImage_,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else {
        std::cerr << "ERROR: Unsupported image layout!" << std::endl;
        exit(EXIT_FAILURE);
    }

    cmdBuf.pipelineBarrier(sourceStage,destinationStage,{},nullptr,nullptr,barrier);

    Engine::getInstance().endSingleTimeCommand(cmdBuf);
}

void Texture::initImageViewAndSampler() {
    vk::ImageViewCreateInfo viewInfo = {
        .image = vkImage_,
        .viewType = vk::ImageViewType::e2D,
        .format = vkFormat_,
        .components = vk::ComponentSwizzle::eIdentity,
        .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
    };

    vkImageView_ = vk::raii::ImageView(Engine::getInstance().getDevice(),viewInfo);

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

    vkSampler_ = vk::raii::Sampler(Engine::getInstance().getDevice(),samplerInfo);
}

