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

    [[nodiscard]] const uint8_t* getData() const{
        return data_;
    }

    ~Texture() override;

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&&) = delete;
    Texture& operator=(Texture&&) = delete;

    explicit Texture(std::string_view fileName);


    void expand();

    [[nodiscard]] std::string getResourceType() const override;

    [[nodiscard]] const vk::raii::ImageView & getVkImageView() const { return vkImageView_; }
    [[nodiscard]] const vk::raii::Sampler & getVkSampler() const { return vkSampler_; }

private:

    Texture(uint32_t width, uint32_t height, uint32_t channels, vk::Format format);
    void uploadToDevice();
    void initImageViewAndSampler();
    void assignVkFormat();
    void transitionImageLayout(vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

    uint32_t width_{};
    uint32_t height_{};
    uint32_t channels_{};
    uint32_t pixelSize_{};
    uint32_t scanWidth_{};

    uint8_t* data_{nullptr};

    FREE_IMAGE_FORMAT freeImageFormat_{};
    FREE_IMAGE_TYPE freeImageType_{};
    vk::Format vkFormat_;

    bool renameOnGenerate{false};

    vk::raii::Image vkImage_{nullptr};
    vk::raii::DeviceMemory vkImageMemory_{nullptr};

    vk::raii::ImageView vkImageView_{nullptr};
    vk::raii::Sampler   vkSampler_{nullptr};


};