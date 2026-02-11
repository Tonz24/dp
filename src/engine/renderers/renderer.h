//
// Created by Tonz on 03.09.2025.
//

#pragma once
#include "../constants.h"
#include "../../scene/scene.h"

class Renderer : public IDrawGui {
public:
    virtual void render(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const vk::Image& swapchainImage, const vk::ImageView&
                        swapchainImageView, const vk::Extent2D&
                        swapchainExtent) = 0;

    ~Renderer() override;

    void setExportSignal(std::string_view fileName);

    virtual uint32_t getAccumulatedFrameCount() {return 0;}

    virtual void resetAccumulator() {};

    static void initLayouts();
    static void destroy();

    virtual void resizeScreen(uint32_t newWidth,uint32_t newHeight) = 0;
    [[nodiscard]] virtual glm::vec<2,uint32_t> getRenderDimensions() const = 0;

    static const vk::raii::DescriptorSetLayout& getDescSetLayoutFrame() {return descSetLayoutFrame_;}
    static const vk::raii::DescriptorSet& getDescSetFrame(uint32_t frameInFlightIndex);

    [[nodiscard]] static uint8_t* getCamUBOsMapped(uint32_t frameInFlightIndex);
    [[nodiscard]] static uint8_t* getMatUBOsMapped(uint32_t frameInFlightIndex);

    [[nodiscard]] static const std::vector<uint8_t*>& getMatUBOsMapped() {return materialUBOsMapped_;}

    static void updateTLASDescriptor(const vk::raii::AccelerationStructureKHR& tlas);
    static void updateEmissiveCDF(const VkUtils::BufferAlloc& trianglesBuffer, const VkUtils::BufferAlloc& cdfBuffer);

    static void registerTextureBindless(const Texture& texture);
    static void registerTextureStorage(const Texture& texture);
    static void uploadObjDescription(const Mesh& mesh);
    static void setReservoirSSBOs(const std::array<VkUtils::BufferAlloc, 3>& reservoirBuffers);

    void flushExportBuffer();

protected:
    Renderer() {
        if (!isDescSetLayoutInit_)
            initDescSetLayout();
    }

    virtual void recordCommandBuffer(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const vk::Image& swapchainImage, const vk::ImageView&
                                     swapchainImageView, const vk::Extent2D&
                                     swapchainExtent) = 0;

    virtual void recordPresentBuffer(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex,
                                           const vk::Image& swapchainImage, const vk::ImageView& swapchainImageView, const vk::Extent2D& swapchainExtent) = 0;

    virtual void setPcsData() = 0;

    bool exportSignal_{false};
    std::string_view exportFileName_{};
    VkUtils::BufferAlloc exportBuffer_{};
    vk::Extent2D exportExtent_;
    bool pendingExport_{false};
    bool isCameraMoving_{false};


    void recordSwapchainImageExport(const vk::Image& swapchainImage, vk::Extent2D extent, std::string_view fileName,
                                    const vk::raii::CommandBuffer& cmdBuf);

private:

    inline static std::vector<VkUtils::BufferAlloc> cameraUBOs_{};
    inline static std::vector<uint8_t*> cameraUBOsMapped_{};

    inline static std::vector<VkUtils::BufferAlloc> materialUBOs_{};
    inline static std::vector<uint8_t*> materialUBOsMapped_{};

    inline static std::vector<vk::raii::DescriptorSet> descSetsFrame_{};

    inline static std::vector<VkUtils::BufferAlloc> objDescSSBOs_{};
    inline static std::vector<uint8_t*> objDescSSBOsMapped_{};

    static void initDescSetLayout();
    inline static vk::raii::DescriptorSetLayout descSetLayoutFrame_{nullptr};
    inline static bool isDescSetLayoutInit_{false};

    static constexpr std::array frameDescriptorBindings{
        vk::DescriptorSetLayoutBinding { // camera UBO
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eRaygenKHR
        },
        vk::DescriptorSetLayoutBinding { // material UBO
            .binding = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eClosestHitKHR |  vk::ShaderStageFlagBits::eRaygenKHR
        },
        vk::DescriptorSetLayoutBinding { // Bindless textures
            .binding = 2,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = Constants::bindlessTextureLimit,
            .stageFlags = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR
        },
        vk::DescriptorSetLayoutBinding { // TLAS
            .binding = 3,
            .descriptorType = vk::DescriptorType::eAccelerationStructureKHR,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eRaygenKHR |  vk::ShaderStageFlagBits::eClosestHitKHR
        },
        vk::DescriptorSetLayoutBinding { // Ray tracing target
            .binding = 4,
            .descriptorType = vk::DescriptorType::eStorageImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR
        },
        vk::DescriptorSetLayoutBinding { // per mesh object description
            .binding = 5,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR
        },
        vk::DescriptorSetLayoutBinding { // Bindless textures UINT
            .binding = 6,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = Constants::bindlessTextureUintLimit,
            .stageFlags = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR
        },
        vk::DescriptorSetLayoutBinding { // emissive triangles buffer
            .binding = 7,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eFragment
        },
        vk::DescriptorSetLayoutBinding { // emissive cdf
            .binding = 8,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eFragment
        },
        vk::DescriptorSetLayoutBinding { // reservoir buffers
            .binding = 9,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 3,
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR
        }
    };

    static constexpr std::array<vk::DescriptorBindingFlags, frameDescriptorBindings.size()> bindingFlags{
        vk::DescriptorBindingFlagBits::eUpdateAfterBind,
        vk::DescriptorBindingFlagBits::eUpdateAfterBind,
        vk::DescriptorBindingFlagBits::eUpdateAfterBind,
        vk::DescriptorBindingFlagBits::eUpdateAfterBind,
        vk::DescriptorBindingFlagBits::eUpdateAfterBind,
        vk::DescriptorBindingFlagBits::eUpdateAfterBind,
        vk::DescriptorBindingFlagBits::eUpdateAfterBind,
        vk::DescriptorBindingFlagBits::eUpdateAfterBind,
        vk::DescriptorBindingFlagBits::eUpdateAfterBind
    };

    static constexpr vk::DescriptorSetLayoutBindingFlagsCreateInfo  createInfo{
        .bindingCount = static_cast<uint32_t>(frameDescriptorBindings.size()),
        .pBindingFlags = bindingFlags.data()
    };

    static inline const vk::DescriptorSetLayoutCreateInfo frameLayoutInfo{
        .pNext = createInfo,
        .flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool,
        .bindingCount = static_cast<uint32_t>(frameDescriptorBindings.size()),
        .pBindings = frameDescriptorBindings.data()
    };
};