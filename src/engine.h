//
// Created by Tonz on 16.07.2025.
//

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <memory>

#include <vulkan/vulkan_raii.hpp>
#include <glm/glm.hpp>

#include "window.h"
#include "engine/scene/camera.h"
#include "engine/scene/mesh.h"
#include "engine/scene/scene.h"

class Engine {
public:
    static Engine& getInstance();

    void run();
    void init();

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags  properties) const;

    const vk::raii::Device &getDevice() const;


    void createBuffer(vk::DeviceSize size, vk::raii::Buffer& buffer, vk::BufferUsageFlags bufferUsage, vk::raii::DeviceMemory& bufferMemory, vk::MemoryPropertyFlags properties) const;
    void copyBuffer(vk::raii::Buffer & srcBuffer, vk::raii::Buffer & dstBuffer, vk::DeviceSize size);

    [[nodiscard]] const Scene& getScene() const { return *scene_; }
    void setScene(std::shared_ptr<Scene> scene) { scene_ = std::move(scene); }

    vk::raii::CommandBuffer beginSingleTimeCommand() const;

    void endSingleTimeCommand(vk::raii::CommandBuffer& cmdBuf) const;

    void copyBufferToImage(const vk::raii::Buffer& buffer, vk::raii::Image& image,uint32_t width, uint32_t height) const;

    [[nodiscard]] const vk::PhysicalDeviceLimits & getDeviceLimits() const { return deviceLimits; }
    [[nodiscard]] const vk::raii::DescriptorPool & getDescriptorPool() const { return descriptorPool_; }
    [[nodiscard]] const vk::raii::DescriptorSetLayout & getDescriptorSetLayoutFrame() const { return descriptorSetLayoutFrame_; }
    [[nodiscard]] const vk::raii::DescriptorSetLayout & getDescriptorSetLayoutMaterial() const { return descriptorSetLayoutMaterial_; }
    [[nodiscard]] uint8_t* getCameraUBO() const {return static_cast<uint8_t *>(uniformBuffersMapped_[frameInFlightIndex_]);}

private:
    friend class VkUtils;

    Engine() = default;

    void initGLFW();
    void initImGui();

    void initVulkan();
    void initVulkanInstance();
    std::vector<const char*> initRequiredInstanceExtensions();
    std::vector<const char*> initValidationLayers();

    void initSurface();

    void initPhysicalDevice();
    void initLogicalDevice();

    void initSwapchain();
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat();
    vk::PresentModeKHR chooseSwapPresentMode();
    vk::Extent2D chooseSwapExtent();
    uint32_t chooseSwapImageCount();
    void cleanupSwapchain();
    void recreateSwapchain();

    [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;

    void initImageViews();
    void initGraphicsPipeline();

    void initCommandPool();
    void initCommandBuffers();

    void initSyncObjects();

    void initDescriptorSetLayout();
    void initUniformBuffers();
    void initDescriptorPool();

    void updateUniformBuffers(uint32_t currentFrame) const;

    void transitionImageLayout(const vk::Image &image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                               vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask,
                               vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask,
                               vk::raii::CommandBuffer &cmdBuf);

    void recordCommandBuffer(uint32_t imageIndex, uint32_t frameInFlightIndex, vk::raii::CommandBuffer &cmdBuf);

    void drawFrame();

    void processInput();

    struct QueueFamilyIndices{
        uint32_t graphicsIndex{std::numeric_limits<uint32_t >::max()};
        uint32_t presentIndex{std::numeric_limits<uint32_t >::max()};
        uint32_t transferIndex{std::numeric_limits<uint32_t >::max()};
    };

    QueueFamilyIndices initQueueFamilyIndices() const;

    void initDebugMessenger();

    void mainLoop();

    void printSupportedExtensions(const std::vector<vk::ExtensionProperties>& supportedExtensions);
    void printSupportedValidationLayers(const std::vector<vk::LayerProperties>& supportedLayers);

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*);


#ifdef NDEBUG
    static constexpr bool ENABLE_VALIDATION_LAYERS{false};
#else
    static constexpr bool ENABLE_VALIDATION_LAYERS{true};
#endif

    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT{1};

    static inline Engine* engineInstance{nullptr};
    std::unique_ptr<Window> window{nullptr};

    static inline const std::vector<const char*> requiredValidationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };

    //https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/
    static inline const std::vector<const char*> requiredDeviceExtensions = {
            vk::KHRSwapchainExtensionName,
            vk::KHRAccelerationStructureExtensionName,
            vk::KHRRayTracingPipelineExtensionName,
            vk::KHRDeferredHostOperationsExtensionName,
            vk::EXTPageableDeviceLocalMemoryExtensionName,
            vk::EXTMemoryPriorityExtensionName
    };

    vk::raii::Context vkContext;
    vk::raii::Instance vkInstance{nullptr};

    vk::raii::SurfaceKHR surface_{nullptr};

    vk::raii::PhysicalDevice physicalDevice{nullptr};
    vk::raii::Device device{nullptr};

    QueueFamilyIndices queueFamilyIndices{};
    vk::raii::Queue graphicsQueue{nullptr};
    vk::raii::Queue presentQueue{nullptr};
    vk::raii::Queue transferQueue{nullptr};

    vk::raii::SwapchainKHR swapChain{nullptr};
    std::vector<vk::Image> swapChainImages{};
    vk::Format swapChainImageFormat{vk::Format::eUndefined};
    vk::Extent2D swapChainExtent{};
    std::vector<vk::raii::ImageView> swapChainImageViews{};

    vk::raii::DescriptorSetLayout descriptorSetLayoutFrame_{nullptr};
    vk::raii::DescriptorSetLayout descriptorSetLayoutMaterial_{nullptr};

    vk::raii::PipelineLayout pipelineLayout{nullptr};
    vk::raii::Pipeline graphicsPipeline{nullptr};

    vk::raii::CommandPool drawCommandPool{nullptr};
    vk::raii::CommandPool transferCommandPool{nullptr};

    std::vector<vk::raii::CommandBuffer> commandBuffers_{};

    std::vector<vk::raii::Semaphore> acquireSemaphores_;

    std::vector<vk::raii::Semaphore> submitSemaphores_{};

    std::vector<vk::raii::Fence> inFlightFences_{};

    std::vector<vk::raii::Buffer> uniformBuffers_{};
    std::vector<vk::raii::DeviceMemory> uniformBufferMemory_{};
    std::vector<void*> uniformBuffersMapped_{};

    vk::raii::DescriptorPool descriptorPool_{nullptr};
    std::vector<vk::raii::DescriptorSet> descriptorSets_{};

    vk::raii::DebugUtilsMessengerEXT debugMessenger{nullptr};

    vk::PhysicalDeviceLimits deviceLimits{};

    bool isInitialized_{false};
    bool isRunning_{false};

    uint32_t frameInFlightIndex_{0};
    uint32_t currentFrameIndex_{0};

    bool framebufferResized_{false};

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        const auto app = static_cast<Engine*>(glfwGetWindowUserPointer(window));
        app->framebufferResized_ = true;
    }

    static inline glm::vec<2,double> cursorPos_{std::numeric_limits<double>::max()};
    bool firstMouse_{true};

    static void mouseMovementCallback(GLFWwindow *window, double dx, double dy) {
        const auto app = static_cast<Engine*>(glfwGetWindowUserPointer(window));

        glm::vec<2,double> newPos{dx,dy};

        if (app->firstMouse_) {
            cursorPos_ = newPos;
            app->firstMouse_ = false;
        }

        glm::vec<2,double> delta = newPos - cursorPos_;
        cursorPos_ = newPos;

        app->scene_->getCamera().updateOrientation(delta.x,delta.y);
    }

    std::shared_ptr<Scene> scene_{};

    vk::raii::DescriptorPool uiPool_{nullptr};

    vk::raii::Image depthImage_{nullptr};
    vk::raii::ImageView depthImageView_{nullptr};
    vk::raii::DeviceMemory depthImageMemory_{nullptr};
    vk::Format depthFormat_{vk::Format::eD32Sfloat};

    void initDepthResources();
    void configureVkUtils() const;
};
