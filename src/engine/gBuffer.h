//
// Created by Tonz on 29.08.2025.
//

#pragma once
#include "managers/managedResource.h"
#include "../scene/texture.h"

class GBuffer : public ManagedResource {
public:

    [[nodiscard]] std::string getResourceType() const override {
        return "G-buffer";
    }

    void transitionToGather(vk::raii::CommandBuffer& cmdBuf) const;
    void transitionToShade(vk::raii::CommandBuffer& cmdBuf) const;
    void transitionToBlit(vk::raii::CommandBuffer& cmdBuf) const;

    ~GBuffer() override = default;


    [[nodiscard]] Texture& getAlbedoMap() const { return *albedoMap_; }
    [[nodiscard]] Texture& getNormalMap() const { return *normalMap_; }
    [[nodiscard]] Texture& getDepthMap() const { return *depthMap_; }
    [[nodiscard]] Texture& getObjectIdMap() const { return *objectIdMap_; }
    [[nodiscard]] Texture& getTarget() const { return *target_; }

    static constexpr vk::Format depthMapVkFormat{vk::Format::eD32Sfloat};
    static constexpr vk::Format idMapVkFormat{vk::Format::eR32Uint};

private:
    friend class GBufferManager;

    GBuffer(std::string_view resourceName, uint32_t width, uint32_t height);


    static constexpr vk::ImageUsageFlags defaultAttachmentUsageFlags{
        vk::ImageUsageFlagBits::eSampled | //  it will be sampled in a shader later
        vk::ImageUsageFlagBits::eColorAttachment | //  render target output
        vk::ImageUsageFlagBits::eTransferSrc // in case of needing to blit into the swapchain
    };


    std::shared_ptr<Texture> albedoMap_{nullptr};
    static constexpr uint32_t albedoMapChannelCount{4};
    static constexpr vk::Format albedoMapVkFormat{vk::Format::eR8G8B8A8Unorm};
    static constexpr vk::ImageUsageFlags albedoMapUsageFlags{defaultAttachmentUsageFlags}; // transfer src for blitting into swapchain

    std::shared_ptr<Texture> normalMap_{nullptr};
    static constexpr uint32_t normalMapChannelCount{4};
    static constexpr vk::Format normalMapVkFormat{vk::Format::eR16G16B16A16Sfloat};
    static constexpr vk::ImageUsageFlags normalMapUsageFlags{defaultAttachmentUsageFlags}; // transfer src for blitting into swapchain

    std::shared_ptr<Texture> depthMap_{nullptr};
    static constexpr uint32_t depthMapChannelCount{1};
    static constexpr vk::ImageUsageFlags depthMapUsageFlags{vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eDepthStencilAttachment}; // sampled because of world space position reconstruction from depth

    std::shared_ptr<Texture> objectIdMap_{nullptr};
    static constexpr uint32_t idMapChannelCount{1};
    static constexpr vk::ImageUsageFlags idMapUsageFlags{vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc};  // transfer src for retrieving id at cursor position

    std::shared_ptr<Texture> target_{nullptr};
    static constexpr uint32_t targetChannelCount{4};
    static constexpr vk::Format targetVkFormat{vk::Format::eR8G8B8A8Unorm};
    static constexpr vk::ImageUsageFlags targetUsageFlags{vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc}; // transfer src for blitting into swapchain

};
