//
// Created by Tonz on 16.07.2025.
//

#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <memory>

#include <vulkan/vulkan_raii.hpp>
#include <glm/glm.hpp>
#include <imgui/imgui.h>

#include "window.h"
#include "../scene/camera.h"
#include "../scene/mesh.h"
#include "../scene/scene.h"
#include "renderers/deferredRenderer.h"
#include "vk/graphicsPipeline.h"

class Engine : public IDrawGui {
public:
    static Engine& getInstance();

    void run();
    void init();
    void cleanup();

    const vk::raii::Device &getDevice() const {return device_;}

    [[nodiscard]] const Scene& getScene() const { return *scene_; }
    void setScene(std::shared_ptr<Scene> scene) { scene_ = std::move(scene); }

    bool drawGUI() override;

    [[nodiscard]] const vk::PhysicalDeviceLimits & getDeviceLimits() const { return deviceLimits; }
    [[nodiscard]] const vk::raii::DescriptorPool & getDescriptorPool() const { return descriptorPool_; }


    void setCameraUBOStorage(const CameraUBOFormat& data);
    void setMaterialUBOStorage(uint32_t updateIndex, const MaterialUBOFormat& data);


    [[nodiscard]] const vk::raii::PhysicalDevice& getPhysicalDevice() const { return physicalDevice; }
    [[nodiscard]] float getDeltaTime() const {return deltaTime_;}

private:
    friend class VkUtils;

    Engine() = default;

    void initGLFW();
    void initImGui();

    void initDummyTexture();

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

    void initCommandPool();
    void initCommandBuffers();

    void initSyncObjects();

    void initDescriptorPool();

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
        vk::EXTMemoryPriorityExtensionName,
        vk::KHRRayQueryExtensionName
};

    vk::raii::Context vkContext;
    vk::raii::Instance vkInstance{nullptr};

    vk::raii::SurfaceKHR surface_{nullptr};

    vk::raii::PhysicalDevice physicalDevice{nullptr};
    vk::raii::Device device_{nullptr};

    QueueFamilyIndices queueFamilyIndices{};
    vk::raii::Queue graphicsQueue{nullptr};
    vk::raii::Queue presentQueue{nullptr};
    vk::raii::Queue transferQueue{nullptr};

    vk::raii::SwapchainKHR swapChain{nullptr};
    std::vector<vk::Image> swapChainImages{};
    vk::Format swapChainImageFormat{vk::Format::eUndefined};
    vk::Extent2D swapChainExtent{};
    std::vector<vk::raii::ImageView> swapChainImageViews{};

    vk::raii::CommandPool graphicsCommandPool_{nullptr};

    std::vector<vk::raii::CommandBuffer> commandBuffers_{};

    std::vector<vk::raii::Semaphore> acquireSemaphores_;
    std::vector<vk::raii::Semaphore> submitSemaphores_{};
    std::vector<vk::raii::Fence> inFlightFences_{};


    vk::raii::DescriptorPool descriptorPool_{nullptr};

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

        if (app->window->getCursorMode() == Window::CursorMode::disabled)
            app->scene_->getCamera().updateOrientation(delta.x,delta.y);
    }


    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods){
        const auto app = static_cast<Engine*>(glfwGetWindowUserPointer(window));

        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && app->window->getCursorMode() == Window::CursorMode::normal && !ImGui::GetIO().WantCaptureMouse)
            app->clickSceneObject(cursorPos_);
    }

    std::shared_ptr<Scene> scene_{};

    vk::raii::DescriptorPool uiPool_{nullptr};

    std::shared_ptr<Texture> dummy_{nullptr};

    void configureVkUtils() const;
    void updateUBOs();


    bool dirtyCameraUBO_{false}, dirtyMaterialUBO_{false};

    CameraUBOFormat cameraUBOStorage_{};
    MaterialUBOFormat materialUBOStorage_{};
    uint32_t materialUpdateIndex_{};

    VkUtils::BufferAlloc idMapTransferBuffer_{};
    void clickSceneObject(const glm::vec<2,double>& cursorPos) const;

    std::shared_ptr<GBuffer> gBuffer_{nullptr};

    int selectedRendererIndex_{0};
    std::shared_ptr<DeferredRenderer> renderer_{};

    float drawFrametime_{};
    float totalFrametime_{};
    float deltaTime_{};
    float oldTime_{};
    uint64_t frameCtr_{0};
};
