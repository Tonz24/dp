//
// Created by Tonz on 23.07.2025.
//

#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <FreeImage.h>

#include <string>
#include <glm/detail/type_vec4.hpp>

#include "camera.h"
#include "../engine/vk/vkUtils.h"
#include "../engine/managers/managedResource.h"

class Texture : public ManagedResource{
public:


    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&&) = delete;
    Texture& operator=(Texture&&) = delete;

    explicit Texture(std::string_view fileName, bool isSrgb);
    Texture(uint32_t width, uint32_t height, uint32_t channels, vk::Format format, vk::ImageUsageFlags imageUsage);


    ~Texture() override;

    void expand();

    [[nodiscard]] std::string getResourceType() const override;

    [[nodiscard]] const vk::raii::ImageView & getVkImageView() const { return vkImageView_; }
    [[nodiscard]] const vk::raii::Sampler & getVkSampler() const { return vkSampler_; }
    [[nodiscard]] const VkUtils::ImageAlloc& getVkImage() const { return imageAlloc_; }

    [[nodiscard]] uint32_t getWidth() const { return width_; }
    [[nodiscard]] uint32_t getHeight() const { return height_; }

    friend class TextureManager;

    static std::shared_ptr<Texture> createDummy(std::string_view name,  const glm::vec<4, uint8_t>& color = {255, 0, 255, 255});

    uint32_t getTotalSize() const {return data_.size() * sizeof(data_[0]);}
    void stage(const VkUtils::BufferAlloc& stagingBuffer) const;

private:

    void initVkImage();

    vk::Format chooseVkFormat(bool isSrgb) const;

    static int getChannelCount(FREE_IMAGE_TYPE type, uint32_t bpp);

    uint32_t width_{};
    uint32_t height_{};
    uint32_t channelCount_{};
    uint32_t pixelSize_{};
    uint32_t scanWidth_{};

    std::vector<uint8_t> data_;

    FREE_IMAGE_FORMAT freeImageFormat_{};
    FREE_IMAGE_TYPE freeImageType_{};
    vk::Format vkFormat_;

    VkUtils::ImageAlloc imageAlloc_;

    vk::raii::ImageView vkImageView_{nullptr};
    vk::raii::Sampler  vkSampler_{nullptr};

    vk::ImageUsageFlags imageUsageFlags_{};
};