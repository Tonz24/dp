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

    ~Renderer() override = default;

    static void initLayouts();
    static void destroy();

    virtual void resizeScreen(uint32_t newWidth,uint32_t newHeight) = 0;
    virtual glm::vec<2,uint32_t> getRenderDimensions() const = 0;

    static const vk::raii::DescriptorSetLayout& getDescSetLayoutFrame() {return descSetLayoutFrame_;}
    static const vk::raii::DescriptorSet& getDescSetFrame(uint32_t frameInFlightIndex);

    [[nodiscard]] static uint8_t* getCamUBOsMapped(uint32_t frameInFlightIndex);
    [[nodiscard]] static uint8_t* getMatUBOsMapped(uint32_t frameInFlightIndex);

    [[nodiscard]] static const std::vector<uint8_t*>& getMatUBOsMapped() {return materialUBOsMapped_;}

    static void updateTLASDescriptor(const vk::raii::AccelerationStructureKHR& tlas);

    static void registerTextureBindless(const Texture& texture);
    static void registerTextureStorage(const Texture& texture);

protected:
    Renderer() {
        if (!isDescSetLayoutInit_)
            initDescSetLayout();
    }

    virtual void recordCommandBuffer(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const vk::Image& swapchainImage, const vk::ImageView&
                                     swapchainImageView, const vk::Extent2D&
                                     swapchainExtent) = 0;

    static constexpr vk::PushConstantRange pcsSkyRange{
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .offset = 0,
        .size = static_cast<uint32_t>(sizeof(PcsSky))
    };

private:

    inline static std::vector<VkUtils::BufferAlloc> cameraUBOs_{};
    inline static std::vector<uint8_t*> cameraUBOsMapped_{};

    inline static std::vector<VkUtils::BufferAlloc> materialUBOs_{};
    inline static std::vector<uint8_t*> materialUBOsMapped_{};

    inline static std::vector<vk::raii::DescriptorSet> descSetsFrame_{};

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
            .stageFlags = vk::ShaderStageFlagBits::eFragment
        },
        vk::DescriptorSetLayoutBinding { // Bindless textures
            .binding = 2,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = Constants::bindlessTextureLimit,
            .stageFlags = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eRaygenKHR
        },
        vk::DescriptorSetLayoutBinding { // TLAS
            .binding = 3,
            .descriptorType = vk::DescriptorType::eAccelerationStructureKHR,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eRaygenKHR // TODO: add other stages for ray tracing pipeline
        },
        vk::DescriptorSetLayoutBinding { // Ray tracing target
            .binding = 4,
            .descriptorType = vk::DescriptorType::eStorageImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR // TODO: add other stages for ray tracing pipeline
        }
    };

    static constexpr vk::DescriptorSetLayoutCreateInfo frameLayoutInfo{
        .bindingCount = static_cast<uint32_t>(frameDescriptorBindings.size()),
        .pBindings = frameDescriptorBindings.data()
    };
};