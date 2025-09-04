//
// Created by Tonz on 03.09.2025.
//

#pragma once
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
    static const vk::raii::DescriptorSetLayout& getDescSetLayoutMaterial() {return descSetLayoutMaterial_;}
    static const vk::raii::DescriptorSetLayout& getDescSetLayoutSky() {return descSetLayoutSky_;}
    static const vk::raii::DescriptorSet& getDescSetFrame(uint32_t frameInFlightIndex);

    [[nodiscard]] static uint8_t* getCamUBOsMapped(uint32_t frameInFlightIndex);
    [[nodiscard]] static uint8_t* getMatUBOsMapped(uint32_t frameInFlightIndex);

    [[nodiscard]] static const std::vector<uint8_t*>& getMatUBOsMapped() {return materialUBOsMapped_;}

protected:
    Renderer() {
        if (!isDescSetLayoutInit_)
            initDescSetLayout();
    }

    virtual void recordCommandBuffer(const Scene& scene, vk::raii::CommandBuffer& cmdBuf, uint32_t frameInFlightIndex, const vk::Image& swapchainImage, const vk::ImageView&
                                     swapchainImageView, const vk::Extent2D&
                                     swapchainExtent) = 0;

private:

    inline static std::vector<VkUtils::BufferAlloc> cameraUBOs_{};
    inline static std::vector<uint8_t*> cameraUBOsMapped_{};

    inline static std::vector<VkUtils::BufferAlloc> materialUBOs_{};
    inline static std::vector<uint8_t*> materialUBOsMapped_{};

    inline static std::vector<vk::raii::DescriptorSet> descSets_{};

    static void initDescSetLayout();
    inline static vk::raii::DescriptorSetLayout descSetLayoutFrame_{nullptr};
    inline static vk::raii::DescriptorSetLayout descSetLayoutMaterial_{nullptr};
    inline static vk::raii::DescriptorSetLayout descSetLayoutSky_{nullptr};
    inline static bool isDescSetLayoutInit_{false};

    static constexpr std::array frameDescriptorBindings{
        vk::DescriptorSetLayoutBinding { // camera UBO
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment
        },
        vk::DescriptorSetLayoutBinding { // material UBO
            .binding = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment
        }
    };

    static constexpr vk::DescriptorSetLayoutCreateInfo frameLayoutInfo{
        .bindingCount = static_cast<uint32_t>(frameDescriptorBindings.size()),
        .pBindings = frameDescriptorBindings.data()
    };

    static constexpr std::array materialBindings{
        vk::DescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment,
        },
        vk::DescriptorSetLayoutBinding {
            .binding = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment,
        },
        vk::DescriptorSetLayoutBinding{
            .binding = 2,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment,
        },
        vk::DescriptorSetLayoutBinding{
            .binding = 3,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment,
        }
    };

    static constexpr vk::DescriptorSetLayoutCreateInfo materialLayoutInfo{
        .bindingCount = static_cast<uint32_t>(materialBindings.size()),
        .pBindings = materialBindings.data()
    };


    static constexpr vk::DescriptorSetLayoutBinding skyBinding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
    };

    static constexpr vk::DescriptorSetLayoutCreateInfo skyLayoutInfo{
        .bindingCount = 1,
        .pBindings = &skyBinding
    };


};