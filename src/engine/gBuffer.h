//
// Created by Tonz on 29.08.2025.
//

#pragma once
#include "uboFormat.h"
#include "managers/managedResource.h"
#include "../scene/texture.h"

class GBuffer : public ManagedResource {
public:

    [[nodiscard]] std::string getResourceType() const override {
        return "G-buffer";
    }

    void transitionToFill(vk::raii::CommandBuffer& cmdBuf) const;
    void transitionToShade(vk::raii::CommandBuffer& cmdBuf) const;
    void transitionToBlit(vk::raii::CommandBuffer& cmdBuf) const;
    void transitionToTrace(vk::raii::CommandBuffer& cmdBuf) const;

    ~GBuffer() override = default;


    void resizeContents(uint32_t width, uint32_t height);

    [[nodiscard]] Texture& getAlbedoMap() const { return *albedoMap_; }
    [[nodiscard]] Texture& getNormalMap() const { return *normalMap_; }
    [[nodiscard]] Texture& getMaterialMap() const { return *materialIdMap_; }
    [[nodiscard]] Texture& getTarget() const { return *target_; }
    [[nodiscard]] Texture& getDepthMap() const { return *depthMap_; }
    [[nodiscard]] Texture& getObjectIdMap() const { return *objectIdMap_; }
    [[nodiscard]] Texture& getAccumulator() const { return *accumulator_; }

    static constexpr vk::ImageUsageFlags defaultAttachmentUsageFlags{
        vk::ImageUsageFlagBits::eSampled | //  will be sampled in a shader later
        vk::ImageUsageFlagBits::eColorAttachment | //  render target output
        vk::ImageUsageFlagBits::eTransferSrc // in case of needing to blit into the swapchain
    };

    static constexpr vk::Format albedoMapVkFormat{vk::Format::eR8G8B8A8Unorm};
    static constexpr vk::ImageUsageFlags albedoMapUsageFlags{defaultAttachmentUsageFlags}; // transfer src for blitting into swapchain

    static constexpr vk::Format normalMapVkFormat{vk::Format::eR16G16B16A16Sfloat};
    static constexpr vk::ImageUsageFlags normalMapUsageFlags{defaultAttachmentUsageFlags}; // transfer src for blitting into swapchain

    static constexpr vk::Format materialMapVkFormat{vk::Format::eR32Uint};
    static constexpr vk::ImageUsageFlags materialMapUsageFlags{defaultAttachmentUsageFlags};  // transfer src for retrieving id at cursor position

    static constexpr vk::Format targetVkFormat{vk::Format::eB10G11R11UfloatPack32};
    static constexpr vk::ImageUsageFlags targetUsageFlags{defaultAttachmentUsageFlags}; // transfer src for blitting into swapchain, sampled for reading skybox in ray gen shader
    static constexpr vk::FormatFeatureFlags targetFormatFlags{vk::FormatFeatureFlagBits::eColorAttachment | vk::FormatFeatureFlagBits::eTransferSrc };


    //  acceptable formats for target G buffer texture
    //  in descending order

    static constexpr std::array targetAcceptableFormats{
        vk::Format::eB10G11R11UfloatPack32,
        vk::Format::eR32G32B32A32Sfloat,
        vk::Format::eR16G16B16A16Sfloat
    };

    static constexpr vk::Format depthMapVkFormat{vk::Format::eD32Sfloat};
    static constexpr vk::ImageUsageFlags depthMapUsageFlags{vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eDepthStencilAttachment}; // sampled because of world space position reconstruction from depth

    static constexpr vk::Format idMapVkFormat{vk::Format::eR32Uint};
    static constexpr vk::ImageUsageFlags idMapUsageFlags{vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc};  // transfer src for retrieving id at cursor position

    static constexpr vk::Format accumulatorFormat{vk::Format::eR32G32B32A32Sfloat};
    static constexpr vk::ImageUsageFlags accumulatorUsageFlags{vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled};


    static constexpr std::array attachmentFormats{albedoMapVkFormat, normalMapVkFormat, idMapVkFormat, materialMapVkFormat};

    static constexpr vk::PushConstantRange pcsFillRange{
        .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        .offset = 0,
        .size = static_cast<uint32_t>(sizeof(PcsGBufferFill))
    };

    static constexpr vk::PushConstantRange pcsShadeRange{
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .offset = 0,
        .size = static_cast<uint32_t>(sizeof(PcsGBufferShade))
    };

    static vk::Format getTargetVkFormat();


private:
    friend class GBufferManager;

    GBuffer(std::string_view resourceName, uint32_t width, uint32_t height);

    std::shared_ptr<Texture> albedoMap_{nullptr};
    std::shared_ptr<Texture> normalMap_{nullptr};
    std::shared_ptr<Texture> materialIdMap_{nullptr};
    std::shared_ptr<Texture> target_{nullptr};

    std::shared_ptr<Texture> accumulator_{nullptr};


    std::shared_ptr<Texture> depthMap_{nullptr};
    std::shared_ptr<Texture> objectIdMap_{nullptr};

    void createTextures(const std::string& prefix, uint32_t width,uint32_t height);

};


