//
// Created by Tonz on 23.07.2025.
//

#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <FreeImage.h>

#include <string>
#include <glm/detail/type_vec4.hpp>

#include "../engine/vk/vkUtils.h"
#include "../engine/managers/managedResource.h"

class Texture : public ManagedResource{
public:


    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&&) = delete;
    Texture& operator=(Texture&&) = delete;

    explicit Texture(std::string_view fileName, bool isSrgb, bool generateMipmaps);
    Texture(uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags imageUsage, vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1, bool populateData = false);

    ~Texture() override;

    [[nodiscard]] std::string getResourceType() const override;
    [[nodiscard]] const vk::raii::ImageView & getVkImageView() const { return vkImageView_; }
    [[nodiscard]] const vk::raii::Sampler & getVkSampler() const { return vkSampler_; }
    [[nodiscard]] const VkUtils::ImageAlloc& getVkImage() const { return imageAlloc_; }
    [[nodiscard]] vk::Format getVkFormat() const { return vkFormat_; }
    [[nodiscard]] uint32_t getWidth() const { return width_; }
    [[nodiscard]] uint32_t getHeight() const { return height_; }
    [[nodiscard]] vk::ImageLayout getSamplerLayout() const { return samplerLayout_; }
    [[nodiscard]] uint32_t getTotalSize() const {return data_.size() * sizeof(data_[0]);}
    [[nodiscard]] std::shared_ptr<Texture> getCdf();
    [[nodiscard]] const std::vector<uint8_t>& getData() const { return data_; }
    [[nodiscard]] vk::ImageUsageFlags getImageUsageFlags() const { return imageUsageFlags_; }
    [[nodiscard]] uint32_t getScanWidth() const { return scanWidth_; }
    [[nodiscard]] uint32_t getPixelSize() const { return pixelSize_; }

    friend class TextureManager;


    /**
     * @brief copies texture data from RAM to VRAM using the provided staging buffer. (data flow is host memory -> staging buffer -> device local memory)
     * @param stagingBuffer buffer that facilitates transfer from host to device local memory. Must be large enough to hold all texture data at once
     */
    void stage(const VkUtils::BufferAlloc& stagingBuffer);
    void generateMipmaps();

    void transitionLayout(vk::ImageLayout newLayout, vk::PipelineStageFlags2 stage, vk::AccessFlags2 accessFlags, vk::raii::CommandBuffer& cmdBuf, const VkUtils::TransitionMipInfo&
                          mipInfo = {0,1});

    /**
     * @brief fetches texture data from device memory into the data_ member on host
     */
    void copyToHost();

    template <typename T>
    T getTexel(uint32_t x, uint32_t y);;

    [[nodiscard]] std::string getCdfName() const {return getResourceName() + "_cdf";}

private:

    // initialize member fields from a FreeImage HDR texture
    void initializeEnvMap(FIBITMAP* bitmap);

    // initialize member fields from a FreeImage non HDR texture
    void initializeTexture(FIBITMAP* bitmap, bool isSrgb);

    void initVkImage();

    vk::Format chooseVkFormat(bool isSrgb) const;

    static int getChannelCount(FREE_IMAGE_TYPE type, uint32_t bpp);
    int getChannelCount(vk::Format format);

    static uint32_t getFormatPixelSize(vk::Format format);

    // width in texels
    uint32_t width_{};
    // height in texels
    uint32_t height_{};
    // channel count
    uint32_t channelCount_{};
    // pixel size in (channel count * element size) in bytes
    uint32_t pixelSize_{};
    // size of one element in bytes
    uint32_t elementSize_{};
    // size of one row (width * channel count * element size) in bytes
    uint32_t scanWidth_{};

    // raw texture data in bytes
    std::vector<uint8_t> data_;

    FREE_IMAGE_FORMAT freeImageFormat_{};
    FREE_IMAGE_TYPE freeImageType_{};
    vk::Format vkFormat_;

    VkUtils::ImageAlloc imageAlloc_;

    vk::raii::ImageView vkImageView_{nullptr};
    vk::raii::Sampler  vkSampler_{nullptr};
    uint32_t mipLevelCount_{1};

    vk::ImageUsageFlags imageUsageFlags_{};
    vk::ImageAspectFlags aspectFlags_{vk::ImageAspectFlagBits::eNone};

    vk::ImageLayout samplerLayout_{vk::ImageLayout::eReadOnlyOptimal};

    std::vector<vk::ImageLayout> mipLayouts_{};
    std::vector<vk::PipelineStageFlags2> mipStageMasks_{};
    std::vector<vk::AccessFlags2> mipAccessMasks_{};
    vk::SampleCountFlagBits sampleCount_{vk::SampleCountFlagBits::e1};
};

template<typename T>
T Texture::getTexel(uint32_t x, uint32_t y) {
    if (x >= width_ || y >= height_)
        throw std::runtime_error("ERROR: trying to access texel at invalid coordinates!");

    uint32_t row = y * width_;
    uint32_t col = x;

    T* data = reinterpret_cast<T*>(data_.data());

    T value = *(data + row + col);
    return value;
}
