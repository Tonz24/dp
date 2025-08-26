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
#include "../scene/camera.h"
#include "../scene/mesh.h"
#include "../scene/scene.h"
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
    [[nodiscard]] const vk::raii::DescriptorSetLayout & getDescriptorSetLayoutFrame() const { return descriptorSetLayoutFrame_; }
    [[nodiscard]] const vk::raii::DescriptorSetLayout & getDescriptorSetLayoutMaterial() const { return descriptorSetLayoutMaterial_; }

    void setCameraUBOStorage(const CameraUBOFormat& data);
    void setMaterialUBOStorage(uint32_t updateIndex, const MaterialUBOFormat& data);

    [[nodiscard]] uint8_t* getMaterialUBO() const {return materialUBOsMapped_[frameInFlightIndex_];}
    [[nodiscard]] const std::vector<uint8_t*>& getMaterialUBOs() const {return materialUBOsMapped_;}

private:
    friend class VkUtils;

    Engine() = default;

    static void initGLFW();
    void initImGui();

    void initDummyTexture();

    void initVulkan();
    void initVulkanInstance();
    std::vector<const char*> initRequiredInstanceExtensions();
    std::vector<const char*> initValidationLayers();

    void initSurface();
    void initVMAllocator();

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

    static constexpr uint32_t maxFramesInFlight{1};
    static constexpr uint32_t materialLimit{100};

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

    vk::raii::DescriptorSetLayout descriptorSetLayoutFrame_{nullptr};
    vk::raii::DescriptorSetLayout descriptorSetLayoutMaterial_{nullptr};

    GraphicsPipeline<Vertex3D> rasterPipeline_{};
    GraphicsPipeline<Vertex2D> skyboxPipeline_{};

    vk::raii::CommandPool graphicsCommandPool_{nullptr};

    std::vector<vk::raii::CommandBuffer> commandBuffers_{};

    std::vector<vk::raii::Semaphore> acquireSemaphores_;

    std::vector<vk::raii::Semaphore> submitSemaphores_{};

    std::vector<vk::raii::Fence> inFlightFences_{};

    std::vector<VkUtils::BufferAlloc> cameraUBOs_{};
    std::vector<uint8_t*> cameraUBOsMapped_{};

    std::vector<VkUtils::BufferAlloc> materialUBOs_{};
    std::vector<uint8_t*> materialUBOsMapped_{};

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

        if (app->window->getCursorMode() == Window::CursorMode::disabled)
            app->scene_->getCamera().updateOrientation(delta.x,delta.y);
    }

    std::shared_ptr<Scene> scene_{};

    vk::raii::DescriptorPool uiPool_{nullptr};


    std::shared_ptr<Texture> depthTexture_{nullptr};
    vk::Format depthFormat_{vk::Format::eD32Sfloat};

    std::shared_ptr<Texture> objectIdMap_{nullptr};
    vk::Format idMapFormat_{vk::Format::eR32Uint};

    std::shared_ptr<Texture> dummy_{nullptr};

    void initDepthResources();
    void initIdMapImage();
    void configureVkUtils() const;
    void updateUBOs();

    void cleanUBOs();

    bool dirtyCameraUBO_{false}, dirtyMaterialUBO_{false};

    CameraUBOFormat cameraUBOStorage_{};
    MaterialUBOFormat materialUBOStorage_{};
    uint32_t materialUpdateIndex_{};

     std::vector<Vertex3D> vertexData = {
/*                                          x		y	  z		                                                    u  v */
        /* top right*/		    {{1.0f,1.0f, 0.0f},	{}, {},{1,1}},
        /* bottom right*/	    {{1.0f,-1.0f, 0.0f},	{}, {},{1, -1}},
        /* bottom left*/  	    {{-1.0f,-1.0f, 0.0f},	{}, {},{-1, -1}},
        /* top left*/		    {{-1.0f, 1.0f, 0.0f},	{}, {},{-1, 1}}
    };

    std::vector<uint32_t> indexData = {
        3, 1, 0,
        3, 2, 1
    };

    std::shared_ptr<Mesh> skyMesh_{nullptr};
    std::shared_ptr<Material> mat_{nullptr};
};
