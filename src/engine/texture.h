//
// Created by Tonz on 23.07.2025.
//

#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <FreeImage.h>

#include <string>

#include "managers/managedResource.h"

class Texture : public ManagedResource{
public:


    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&&) = delete;
    Texture& operator=(Texture&&) = delete;

    explicit Texture(std::string_view fileName);

    void expand();

    [[nodiscard]] std::string getResourceType() const override;

    [[nodiscard]] const vk::raii::ImageView & getVkImageView() const { return vkImageView_; }
    [[nodiscard]] const vk::raii::Sampler & getVkSampler() const { return vkSampler_; }
    [[nodiscard]] const vk::raii::Image& getVkImage() const { return vkImage_; }

    [[nodiscard]] uint32_t getWidth() const { return width_; }
    [[nodiscard]] uint32_t getHeight() const { return height_; }

    friend class TextureManager;

    static void initDummy();

    static Texture& getDummy() {return *dummy_;}
    Texture(uint32_t width, uint32_t height, uint32_t channels, vk::Format format, vk::ImageUsageFlags imageUsage, vk::MemoryPropertyFlags memoryProperties);


private:

    void initVkImage();
    void uploadToDevice();
    void initImageViewAndSampler();
    void assignVkFormat();


    uint32_t width_{};
    uint32_t height_{};
    uint32_t channels_{};
    uint32_t pixelSize_{};
    uint32_t scanWidth_{};

    std::vector<uint8_t> data_;

    FREE_IMAGE_FORMAT freeImageFormat_{};
    FREE_IMAGE_TYPE freeImageType_{};
    vk::Format vkFormat_;

    vk::raii::Image vkImage_{nullptr};
    vk::raii::DeviceMemory vkImageMemory_{nullptr};

    vk::raii::ImageView vkImageView_{nullptr};
    vk::raii::Sampler   vkSampler_{nullptr};

    vk::ImageUsageFlags imageUsageFlags_{};
    vk::MemoryPropertyFlags memoryPropertyFlags_{};

   static inline Texture* dummy_{nullptr};
};