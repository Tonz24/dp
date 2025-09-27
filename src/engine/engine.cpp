//
// Created by Tonz on 16.07.2025.
//

#include "engine.h"

#include <iostream>
#include <ranges>
#include <set>
#define GLM_ENABLE_EXPERIMENTAL
#include <queue>
#include <glm/gtx/string_cast.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

#include "constants.h"
#include "managers/inputManager.h"
#include "vk/vkUtils.h"
#include "../scene/texture.h"
#include "managers/resourceManager.h"
#include "renderers/renderer.h"

Engine &Engine::getInstance() {
    if (engineInstance == nullptr)
        engineInstance = new Engine();

    return *engineInstance;
}

void Engine::run() {
    if (!isInitialized_)
        init();

    if (!isRunning_)
        mainLoop();
}

void Engine::init() {

    if (isInitialized_)
        return;

    initGLFW();
    initVulkan();
    initImGui();

    isInitialized_ = true;
}

bool Engine::drawGUI() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("DP");

    if (ImGui::CollapsingHeader("Engine",ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        static constexpr std::array items{"G buffer debug","Naive path tracer","NEE path tracer","ReSTIR DI","ReSTIR GI"};
        if (ImGui::Combo("Renderer", &selectedRendererIndex_, items.data(), items.size())) {
            if (selectedRendererIndex_ == 0)
                selectedRenderer_ = rasterRenderer_.get();
            if (selectedRendererIndex_ == 1)
                selectedRenderer_ = rtRenderer_.get();
        }

        if (selectedRenderer_)  selectedRenderer_->drawGUI();
        if (scene_)     scene_->drawGUI();
        ImGui::Unindent();
    }
    if (ImGui::CollapsingHeader("Stats",ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        ImGui::Text(("Total frame time: " + std::to_string(totalFrametime_) + " ms").c_str());
        ImGui::Text(("Draw time: " + std::to_string(drawFrametime_) + " ms").c_str());
        ImGui::Text(("FPS: " + std::to_string(1000.0f / totalFrametime_)).c_str());
        ImGui::Separator();
        ImGui::Text(("Swapchain dimensions: " + std::to_string(swapChainExtent.width) + "x" + std::to_string(swapChainExtent.height)).c_str());
        auto renderDims = selectedRenderer_->getRenderDimensions();
        ImGui::Text(("Render dimensions: " + std::to_string(renderDims.x) + "x" + std::to_string(renderDims.y)).c_str());

        ImGui::Unindent();
    }

    return false;
}


void Engine::initGLFW() {

    if (!glfwInit())
        throw std::runtime_error("ERROR: Failed to initialize GLFW!");

    if (!glfwVulkanSupported())
        throw std::runtime_error("ERROR: Vulkan not supported by GLFW!");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = std::make_unique<Window>("DP",1280,720,false);

    glfwSetWindowUserPointer(window->getGlfwWindow(),this);
    glfwSetFramebufferSizeCallback(window->getGlfwWindow(),framebufferResizeCallback);
    glfwSetCursorPosCallback(window->getGlfwWindow(),mouseMovementCallback);
    glfwSetKeyCallback(window->getGlfwWindow(),InputManager::keyCallback);
    glfwSetMouseButtonCallback(window->getGlfwWindow(),mouseButtonCallback);
}

void Engine::initImGui() {
    //1: create descriptor pool for IMGUI
    // the size of the pool is very oversize, but it's copied from imgui demo itself.
    std::vector<vk::DescriptorPoolSize> poolSizes{
        {.type = vk::DescriptorType::eSampler,.descriptorCount = 1000},
        {.type = vk::DescriptorType::eCombinedImageSampler,.descriptorCount = 1000},
        {.type = vk::DescriptorType::eSampledImage,.descriptorCount = 1000},
        {.type = vk::DescriptorType::eStorageImage,.descriptorCount = 1000},
        {.type = vk::DescriptorType::eUniformTexelBuffer,.descriptorCount = 1000},
        {.type = vk::DescriptorType::eStorageTexelBuffer,.descriptorCount = 1000},
        {.type = vk::DescriptorType::eUniformBuffer,.descriptorCount = 1000},
        {.type = vk::DescriptorType::eStorageBuffer,.descriptorCount = 1000},
        {.type = vk::DescriptorType::eUniformBufferDynamic,.descriptorCount = 1000},
        {.type = vk::DescriptorType::eStorageBufferDynamic,.descriptorCount = 1000},
        {.type = vk::DescriptorType::eInputAttachment,.descriptorCount = 1000},

    };

    vk::DescriptorPoolCreateInfo poolInfo = {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = 1000,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    };

    uiPool_ = vk::raii::DescriptorPool(device_,poolInfo);

    auto format = static_cast<VkFormat>(swapChainImageFormat);

    ImGui_ImplVulkan_InitInfo initInfo = {
        .ApiVersion = vk::ApiVersion13,
        .Instance = *vkInstance,
        .PhysicalDevice = *physicalDevice,
        .Device = *device_,
        .Queue = *graphicsQueue,
        .DescriptorPool = *uiPool_,
        .MinImageCount = chooseSwapImageCount(),
        .ImageCount = chooseSwapImageCount(),
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .UseDynamicRendering = true,
        .PipelineRenderingCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &format,
            .depthAttachmentFormat = static_cast<VkFormat>(GBuffer::depthMapVkFormat),
        },
    };

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan(window->getGlfwWindow(),true);

    ImGui_ImplVulkan_Init(&initInfo);
}

void Engine::initDummyTexture() {
    dummy_ = Texture::createDummy("dummy");
}

void Engine::initVulkan() {

    initVulkanInstance();

    if (Constants::enableValidationLayers)
        initDebugMessenger();

    initSurface();

    initPhysicalDevice();
    initLogicalDevice();

    configureVkUtils();

    initSwapchain();
    initImageViews();

    initDescriptorPool();

    initCommandPool();
    initCommandBuffers();

    initSyncObjects();

    initDummyTexture();
    Renderer::initLayouts();

    VmaAllocationCreateFlags allocationCreateFlags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    idMapTransferBuffer_ = VkUtils::createBufferVMA(sizeof(uint32_t),vk::BufferUsageFlagBits::eTransferDst, allocationCreateFlags);

    gBuffer_ = GBufferManager::getInstance()->registerResource("gbuffer_test",1280,720);
    rasterRenderer_ = std::make_shared<DeferredRenderer>(gBuffer_);
    rtRenderer_ = std::make_shared<RaytracingRenderer>(gBuffer_);
    selectedRenderer_ = rasterRenderer_.get();
}

void Engine::initVulkanInstance() {
    auto requiredExtensions = initRequiredInstanceExtensions();
    auto validationLayers = initValidationLayers();

    vk::ApplicationInfo appInfo{
            .pApplicationName = "DP",
            .applicationVersion = vk::makeVersion(1,0,0),
            .pEngineName = nullptr,
            .engineVersion = vk::makeVersion(1,0,0),
            .apiVersion = vk::ApiVersion14
    };


    std::vector enables = Constants::enableValidationLayers ? std::vector{
        vk::ValidationFeatureEnableEXT::eGpuAssisted,
        vk::ValidationFeatureEnableEXT::eGpuAssistedReserveBindingSlot,
        vk::ValidationFeatureEnableEXT::eSynchronizationValidation,
        vk::ValidationFeatureEnableEXT::eBestPractices
    } : std::vector<vk::ValidationFeatureEnableEXT>{};

    vk::ValidationFeaturesEXT validationFeatures{
        .enabledValidationFeatureCount = static_cast<uint32_t>(enables.size()),
        .pEnabledValidationFeatures    = enables.data(),
    };

    vk::InstanceCreateInfo createInfo{
            .pNext = &validationFeatures,
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
            .ppEnabledLayerNames = validationLayers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
            .ppEnabledExtensionNames = requiredExtensions.data(),
    };

    vkInstance = vk::raii::Instance(vkContext, createInfo);
}

void Engine::initSurface() {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(*vkInstance,window->getGlfwWindow(),nullptr,&surface) != 0)
        throw std::runtime_error("ERROR: Failed to create window surface!");

    surface_ = vk::raii::SurfaceKHR(vkInstance, surface);
}

Engine::QueueFamilyIndices Engine::initQueueFamilyIndices() const {
    auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

    std::set<uint32_t> graphicsFamilyIndices{};
    std::set<uint32_t> presentFamilyIndices{};
    bool transferIndexSet{false};
    uint32_t transferFamilyIndex{};

    //  identify which families support graphics, presentation and transfer
    for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i) {
        const auto& familyProps = queueFamilyProperties[i];

        if (familyProps.queueFlags & vk::QueueFlagBits::eGraphics)
            graphicsFamilyIndices.insert(i);

        if (physicalDevice.getSurfaceSupportKHR(i,surface_))
            presentFamilyIndices.insert(i);

        //  family for transfer is the first family that doesn't support graphics but supports transfer
        if (familyProps.queueFlags & vk::QueueFlagBits::eTransfer && !(familyProps.queueFlags & vk::QueueFlagBits::eGraphics)) {
            if (!transferIndexSet) {
                transferFamilyIndex = i;
                transferIndexSet = true;
            }
        }
    }

    if (graphicsFamilyIndices.empty())
        throw std::runtime_error("ERROR: selected device doesn't have a graphics capable queue family!");

    if (presentFamilyIndices.empty())
        throw std::runtime_error("ERROR: selected device doesn't have a presentation capable queue family!");

    // if there is a single family that supports both graphics and presentation, return its indices
    for (const auto &graphicsFamilyIndex: graphicsFamilyIndices){
        if (presentFamilyIndices.contains(graphicsFamilyIndex))
            return {
                .graphicsIndex = graphicsFamilyIndex,
                .presentIndex = graphicsFamilyIndex,
                .transferIndex = transferFamilyIndex,
            };
    }

    // otherwise return the first indices_ in respective categories
    return {
        .graphicsIndex = *graphicsFamilyIndices.begin(),
        .presentIndex = *presentFamilyIndices.begin(),
        .transferIndex = transferFamilyIndex,
    };
}

void Engine::initPhysicalDevice() {
    auto devices = vkInstance.enumeratePhysicalDevices();

    if (devices.empty())
        throw std::runtime_error("ERROR: Failed to find GPUs with Vulkan support!");

    uint32_t selectedDeviceIndex{};

    bool deviceFound{false};


    for (auto  [deviceIndex, availableDevice]: std::views::enumerate(devices) | std::views::as_const){

        auto deviceSupportedExtensions = availableDevice.enumerateDeviceExtensionProperties();

        bool areRequiredExtensionsSupported{true};
        for (const auto &requiredDeviceExtension: requiredDeviceExtensions){

            bool isRequiredDeviceExtensionSupported{false};
            for (const auto &deviceSupportedExtension: deviceSupportedExtensions){
                if (std::strcmp(requiredDeviceExtension,deviceSupportedExtension.extensionName) == 0){
                    isRequiredDeviceExtensionSupported = true;
                    break;
                }
            }

            if (!isRequiredDeviceExtensionSupported) {
                areRequiredExtensionsSupported = false;
                break;
            }
        }

        if (areRequiredExtensionsSupported){
            deviceFound = true;
            selectedDeviceIndex = deviceIndex;
            break;
        }
    }

    if (!deviceFound)
        throw std::runtime_error("ERROR: Failed to locate device supporting required extensions!");

    physicalDevice = vk::raii::PhysicalDevice(devices[selectedDeviceIndex]);
    deviceLimits = physicalDevice.getProperties().limits;
    std::cout << "Selected device: " << physicalDevice.getProperties().deviceName << std::endl;
}

void Engine::initLogicalDevice() {

    queueFamilyIndices = initQueueFamilyIndices();

    float graphicsFamilyPriority{1.0f};

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{
    {
            .queueFamilyIndex = queueFamilyIndices.graphicsIndex,
            .queueCount = 1,
            .pQueuePriorities = &graphicsFamilyPriority
        },
        {
            .queueFamilyIndex = queueFamilyIndices.transferIndex,
            .queueCount = 1,
            .pQueuePriorities = &graphicsFamilyPriority
        }
    };
    

    // Create a chain of feature structures
    vk::StructureChain<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
        vk::PhysicalDeviceRayTracingPipelineFeaturesKHR,
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
        vk::PhysicalDeviceMemoryPriorityFeaturesEXT,
        vk::PhysicalDevicePageableDeviceLocalMemoryFeaturesEXT,
        vk::PhysicalDeviceRayQueryFeaturesKHR
        >
            featureChain {
                {.features = {.samplerAnisotropy = vk::True,}}, // vk::PhysicalDeviceFeatures2 (empty for now)
                {.storageBuffer8BitAccess = vk::True, .scalarBlockLayout = true, .timelineSemaphore = vk::True,  .bufferDeviceAddress = vk::True,  .vulkanMemoryModel = vk::True,  .vulkanMemoryModelDeviceScope = vk::True,},
                {.shaderDemoteToHelperInvocation =  vk::True, .synchronization2 = vk::True, .dynamicRendering = vk::True,},      // Enable dynamic rendering from Vulkan 1.3
                {.extendedDynamicState = vk::True }, // Enable extended dynamic state from the extension_
                {.rayTracingPipeline = vk::True},
                {.accelerationStructure = vk::True},
                {},
                {},
                {.rayQuery = vk::True}
    };

    vk::DeviceCreateInfo deviceCreateInfo{
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size()),
        .ppEnabledExtensionNames = requiredDeviceExtensions.data()
    };

    device_ = vk::raii::Device(physicalDevice,deviceCreateInfo);

    graphicsQueue = vk::raii::Queue(device_,queueFamilyIndices.graphicsIndex,0);
    presentQueue = vk::raii::Queue(device_,queueFamilyIndices.presentIndex,0);
    transferQueue = vk::raii::Queue(device_,queueFamilyIndices.transferIndex,0);
}

void Engine::initSwapchain() {
    auto surfaceFormat = chooseSwapSurfaceFormat();
    auto presentMode = chooseSwapPresentMode();
    auto swapExtent = chooseSwapExtent();
    uint32_t swapImageCount = chooseSwapImageCount();

    const auto& surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface_);

    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
       .flags = vk::SwapchainCreateFlagsKHR(),
       .surface = surface_,
       .minImageCount = swapImageCount,
       .imageFormat = surfaceFormat.format,
       .imageColorSpace = surfaceFormat.colorSpace,
       .imageExtent = swapExtent,
       .imageArrayLayers = 1,
       .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
       .imageSharingMode = vk::SharingMode::eExclusive,
       .preTransform = surfaceCapabilities.currentTransform,
       .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
       .presentMode = presentMode,
       .clipped = true,
       .oldSwapchain = nullptr
    };

    uint32_t familyIndices[] = {queueFamilyIndices.graphicsIndex, queueFamilyIndices.presentIndex};

    if (queueFamilyIndices.presentIndex != queueFamilyIndices.graphicsIndex) {
        swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = familyIndices;
    } else {
        swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
        swapChainCreateInfo.queueFamilyIndexCount = 0; // Optional
        swapChainCreateInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    swapChain = vk::raii::SwapchainKHR(device_, swapChainCreateInfo);

    swapChainImages = swapChain.getImages();
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = swapExtent;
}

//TODO: Figure out how to separate the framebuffer image from the swapchain, as it's supposedly better
// https://www.reddit.com/r/vulkan/comments/p3iy0o/why_use_bgra_instead_of_rgba/
vk::SurfaceFormatKHR Engine::chooseSwapSurfaceFormat() {
    const auto& availableSurfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface_);

    for (const auto &format: availableSurfaceFormats){
        if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear){
            return format;
        }
    }

    std::cerr << "WARNING: B8G8R8A8srgb surface format not supported, using the default one!" << std::endl;

    return availableSurfaceFormats[0];
}

vk::PresentModeKHR Engine::chooseSwapPresentMode() {
    const auto& availablePresentModes = physicalDevice.getSurfacePresentModesKHR(surface_);

//    TODO: Figure out whether eMailbox is better with path tracing (replacing rendered frames before integrating them doesn't sound like a good idea)
//    for (const auto &presentMode: availablePresentModes){
//        if (presentMode == vk::PresentModeKHR::eMailbox)
//            return presentMode;
//    }

    //eFifo is guaranteed to be available (https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/01_Presentation/01_Swap_chain.html)
    return vk::PresentModeKHR::eImmediate;
    //return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Engine::chooseSwapExtent() {
    const auto & surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface_);

    if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return surfaceCapabilities.currentExtent;

    int width, height;
    glfwGetFramebufferSize(window->getGlfwWindow(),&width,&height);

    return {
            std::clamp<uint32_t>(width,surfaceCapabilities.minImageExtent.width,surfaceCapabilities.maxImageExtent.width),
            std::clamp<uint32_t>(height,surfaceCapabilities.minImageExtent.height,surfaceCapabilities.maxImageExtent.height),
    };
}

uint32_t Engine::chooseSwapImageCount() {
    auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface_);

    uint32_t swapImageCount = surfaceCapabilities.minImageCount + 1;

    if (surfaceCapabilities.maxImageCount > 0 && swapImageCount > surfaceCapabilities.maxImageCount)
        swapImageCount = surfaceCapabilities.maxImageCount;

    return swapImageCount;
}

std::vector<const char*> Engine::initRequiredInstanceExtensions() {
    uint32_t glfwExtensionCount = 0;
    auto glfwRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    auto vkProvidedExtensions = vkContext.enumerateInstanceExtensionProperties();

    std::vector requiredExtensions(glfwRequiredExtensions, glfwRequiredExtensions + glfwExtensionCount);

    if (Constants::enableValidationLayers)
        requiredExtensions.push_back("VK_EXT_debug_utils");

    printSupportedExtensions(vkProvidedExtensions);

    for (const auto &requiredExtension: requiredExtensions){
        bool isRequiredExtensionSupported{false};
        for (const auto &vkProvidedExtension: vkProvidedExtensions){
            if (std::strcmp(requiredExtension,vkProvidedExtension.extensionName) == 0){
                isRequiredExtensionSupported = true;
                break;
            }
        }
        if (!isRequiredExtensionSupported)
            throw std::runtime_error("ERROR: extension_ required by instance (" + std::string{requiredExtension} + ") not supported by this Vulkan implementation!");
    }

    return requiredExtensions;
}

void Engine::initImageViews() {
    swapChainImageViews.clear();

    vk::ImageViewCreateInfo imageViewCreateInfo{
        .flags = vk::ImageViewCreateFlags(),
        .viewType = vk::ImageViewType::e2D,
        .format = swapChainImageFormat,
        .components = vk::ComponentMapping{
            .r = vk::ComponentSwizzle::eIdentity,
            .g = vk::ComponentSwizzle::eIdentity,
            .b = vk::ComponentSwizzle::eIdentity,
            .a = vk::ComponentSwizzle::eIdentity,
        },
        .subresourceRange = vk::ImageSubresourceRange{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
        }
    };

    for (const auto& image : swapChainImages) {
        imageViewCreateInfo.image = image;
        swapChainImageViews.emplace_back( device_, imageViewCreateInfo );
    }
}

void Engine::initCommandPool() {

    vk::CommandPoolCreateInfo drawPoolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = queueFamilyIndices.graphicsIndex
    };
    graphicsCommandPool_ = vk::raii::CommandPool(device_, drawPoolInfo);
}

void Engine::initCommandBuffers() {
    vk::CommandBufferAllocateInfo commandBufferAllocInfo{
        .commandPool = graphicsCommandPool_,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = Constants::maxFramesInFlight
    };

    commandBuffers_ = vk::raii::CommandBuffers(device_,commandBufferAllocInfo);
}

std::vector<const char *> Engine::initValidationLayers() {
    std::vector<const char*> requiredLayers = Constants::enableValidationLayers ? requiredValidationLayers : std::vector<const char*>();

    auto vkSupportedValidationLayers = vkContext.enumerateInstanceLayerProperties();
    printSupportedValidationLayers(vkSupportedValidationLayers);

    for (const auto &requiredLayer: requiredLayers){

        bool isRequiredLayerSupported{false};
        for (const auto &supportedLayer: vkSupportedValidationLayers){
            if (std::strcmp(requiredLayer,supportedLayer.layerName) == 0){
                isRequiredLayerSupported = true;
                break;
            }
        }

        if (!isRequiredLayerSupported)
            throw std::runtime_error("ERROR: Required validation layer (" + std::string{requiredLayer} + ") not supported by this Vulkan implementation!");
    }

    return requiredLayers;
}

void Engine::initDebugMessenger() {
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags( vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError );
    vk::DebugUtilsMessageTypeFlagsEXT    messageTypeFlags( vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation );

    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
            .messageSeverity = severityFlags,
            .messageType = messageTypeFlags,
            .pfnUserCallback = &debugCallback
    };
    debugMessenger = vkInstance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
}

void Engine::printSupportedExtensions(const std::vector<vk::ExtensionProperties> &supportedExtensions) {
    std::cout << "Extensions supported by Vulkan instance:\n";
    for (const auto &extension: supportedExtensions)
        std::cout << "\t" << extension.extensionName << "\n";
    std::cout << std::endl;


}

void Engine::printSupportedValidationLayers(const std::vector<vk::LayerProperties> &supportedLayers) {
    std::cout << "Validation layers supported by Vulkan instance:\n";
    for (const auto &layer: supportedLayers)
        std::cout << "\t" << layer.layerName << "\n";
    std::cout << std::endl;
}

vk::Bool32
Engine::debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type,
                      const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void *) {

    std::string msg;

    msg.append( "==============================Debug callback==============================\n");
    msg.append( "\tSeverity: " +  to_string(severity) + "\n");
    msg.append( "\tType: " + to_string(type) + "\n");
    msg.append( "\tObjects:\n");

    for (uint32_t i = 0; i < pCallbackData->objectCount; ++i){

        std::string objectName{"None"};

        if (pCallbackData->pObjects[i].pObjectName != nullptr)
            objectName = pCallbackData->pObjects[i].pObjectName;


        msg.append( "\t\tHandle: " + std::to_string(pCallbackData->pObjects[i].objectHandle) +"\n");
        msg.append( "\t\tName: " + objectName +"\n");
        msg.append( "\t\tType: " + to_string(pCallbackData->pObjects[i].objectType) + "\n\n");
    }

    msg.append( "\tMessage: " + std::string{pCallbackData->pMessage} + "\n");
    msg.append( "==========================================================================\n");

    std::cout << msg << std::endl;

    return vk::False;
}


vk::raii::ShaderModule Engine::createShaderModule(const std::vector<char> &code) const {
    vk::ShaderModuleCreateInfo createInfo{
            .codeSize = code.size() * sizeof(char),
            .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };
    return vk::raii::ShaderModule{device_, createInfo};
}


void Engine::drawFrame() {



    //  reset the current frame's fence
    vk::raii::Fence& frameFence = inFlightFences_[frameInFlightIndex_];
    device_.waitForFences(*frameFence, vk::True, UINT64_MAX );

    updateUBOs();

    //  acquire next swapchain image
    vk::raii::Semaphore& acquireSemaphore = acquireSemaphores_[frameInFlightIndex_];
    auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, *acquireSemaphore, nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapchain();
        return;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
        throw std::runtime_error("ERROR: Failed to acquire swap chain image!");

    device_.resetFences(*frameFence);

    vk::raii::Semaphore& submitSemaphore = submitSemaphores_[imageIndex];

    //  record command buffer for this frame
    vk::raii::CommandBuffer& cmdBuf = commandBuffers_[frameInFlightIndex_];

    selectedRenderer_->render(*scene_,cmdBuf,frameInFlightIndex_,swapChainImages[imageIndex],swapChainImageViews[imageIndex],swapChainExtent);

    //  set up the submit info for drawing
    //  set up the wait stage mask as color attachment output
    vk::PipelineStageFlags waitDestinationStageMask{vk::PipelineStageFlagBits::eColorAttachmentOutput};
    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*acquireSemaphore,
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*cmdBuf,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*submitSemaphore
    };

    graphicsQueue.submit(submitInfo,*frameFence);

    const vk::PresentInfoKHR presentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*submitSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &*swapChain,
        .pImageIndices = &imageIndex,
        .pResults = nullptr
    };

    //  do this with exceptions because of vulkan raii (the error gets thrown as an exception before being returned from the function call)
    try {
        vk::Result thisResult = presentQueue.presentKHR( presentInfoKHR );
    }
    catch (const vk::OutOfDateKHRError& e) {
        result = vk::Result::eErrorOutOfDateKHR;
    }
    catch (const vk::Error& e) {
        throw std::runtime_error("ERROR: Failed to acquire swap chain image! (" + std::string{e.what()} + ")");
    }
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized_) {
        framebufferResized_ = false;
        recreateSwapchain();
        return;
    }

    currentFrameIndex_ += 1;
    frameInFlightIndex_ = currentFrameIndex_ % Constants::maxFramesInFlight;
}

void Engine::processInput() {

    glm::vec3 velocity{};

    if (glfwGetKey(window->getGlfwWindow(),GLFW_KEY_A) == GLFW_PRESS) {
         velocity.x -= 1.0f * deltaTime_;
    }
    if (glfwGetKey(window->getGlfwWindow(),GLFW_KEY_D) == GLFW_PRESS) {
        velocity.x += 1.0f * deltaTime_;
    }

    if (glfwGetKey(window->getGlfwWindow(),GLFW_KEY_W) == GLFW_PRESS) {
        velocity.z += 1.0f * deltaTime_;
    }
    if (glfwGetKey(window->getGlfwWindow(),GLFW_KEY_S) == GLFW_PRESS) {
        velocity.z -= 1.0f * deltaTime_;
    }
    if (window->getCursorMode() == Window::CursorMode::disabled)
        scene_->getCamera().updatePosition(velocity);
}

void Engine::mainLoop() {
    isRunning_ = true;

    while(!glfwWindowShouldClose(window->getGlfwWindow())) {
        std::chrono::steady_clock::time_point frameStart = std::chrono::steady_clock::now();
        float currentTime = glfwGetTime();
        deltaTime_ = currentTime - oldTime_;
        oldTime_ = currentTime;

        glfwPollEvents();
        processInput();

        drawGUI();
        ImGui::End();
        ImGui::Render();

        std::chrono::steady_clock::time_point renderStart = std::chrono::steady_clock::now();
        drawFrame();
        std::chrono::steady_clock::time_point renderEnd = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point frameEnd = std::chrono::steady_clock::now();

        drawFrametime_ = std::chrono::duration_cast<std::chrono::microseconds>(renderEnd - renderStart).count() / 1000.0f;
        totalFrametime_ = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - frameStart).count() / 1000.0f;
    }
    isRunning_ = false;
    device_.waitIdle();
}

void Engine::cleanup() {
    scene_.reset();

    dummy_.reset();
    gBuffer_.reset();

    VkUtils::destroyBufferVMA(std::move(idMapTransferBuffer_));

    rasterRenderer_.reset();
    rtRenderer_.reset();
    Renderer::destroy();

    VkUtils::destroy();
    glfwTerminate();
}

void Engine::initSyncObjects() {
    for (uint32_t i = 0; i < Constants::maxFramesInFlight; ++i) {
        inFlightFences_.emplace_back(device_,vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
        acquireSemaphores_.emplace_back(device_, vk::SemaphoreCreateInfo{});
    }

    for (uint32_t i = 0; i < swapChainImages.size(); ++i) {
        submitSemaphores_.emplace_back(device_, vk::SemaphoreCreateInfo{});
    }
}


void Engine::initDescriptorPool() {
    std::array poolSize{
        vk::DescriptorPoolSize {
            .type = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 100
        },
        vk::DescriptorPoolSize {
            .type = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = Constants::bindlessTextureLimit + Constants::textureSamplerLimit
        },
        vk::DescriptorPoolSize {
            .type = vk::DescriptorType::eAccelerationStructureKHR,
            .descriptorCount = 1
        },
        vk::DescriptorPoolSize {
            .type = vk::DescriptorType::eStorageImage,
            .descriptorCount = 1
        },
        vk::DescriptorPoolSize {
            .type = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 10
        }
    };

    vk::DescriptorPoolCreateInfo poolInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = 1000,
        .poolSizeCount = poolSize.size(),
        .pPoolSizes = poolSize.data()
    };

    descriptorPool_ = vk::raii::DescriptorPool(device_,poolInfo);
}

void Engine::recreateSwapchain() {

    //  if width or height is 0 (the window is minimized), the program hangs here until it is not minimized again
    int width{0}, height{0};
    glfwGetFramebufferSize(window->getGlfwWindow(),&width,&height);
    while (width == 0 || height == 0){
        glfwGetFramebufferSize(window->getGlfwWindow(),&width,&height);
        glfwWaitEvents();
    }

    device_.waitIdle();

    selectedRenderer_->resizeScreen(width,height);
    cleanupSwapchain();
    initSwapchain();
    initImageViews();
}

void Engine::cleanupSwapchain() {
    swapChainImageViews.clear();
    swapChain = nullptr;
}


void Engine::configureVkUtils() const {

    VkUtils::init(&device_,&physicalDevice, &vkInstance, {&graphicsQueue,&presentQueue,&transferQueue}, &graphicsCommandPool_);
}

void Engine::updateUBOs() {

    if (dirtyCameraUBO_) {
        memcpy(selectedRenderer_->getCamUBOsMapped(frameInFlightIndex_),&cameraUBOStorage_,sizeof(cameraUBOStorage_));
        dirtyCameraUBO_ = false;
    }

    if (dirtyMaterialUBO_) {
        uint8_t* dst = selectedRenderer_->getMatUBOsMapped(frameInFlightIndex_) + materialUpdateIndex_ * sizeof(materialUBOStorage_);
        memcpy(dst, &materialUBOStorage_,sizeof(materialUBOStorage_));
        dirtyMaterialUBO_ = false;
    }
}



void Engine::setCameraUBOStorage(const CameraUBOFormat& data) {
    dirtyCameraUBO_ = true;
    cameraUBOStorage_ = data;
}

void Engine::setMaterialUBOStorage(uint32_t updateIndex, const MaterialUBOFormat& data) {
    dirtyMaterialUBO_ = true;
    materialUpdateIndex_ = updateIndex;
    materialUBOStorage_ = data;
}

void Engine::clickSceneObject(const glm::vec<2,double>& cursorPos) const {
    auto xPos = static_cast<int32_t>(glm::floor(cursorPos.x));
    auto yPos = static_cast<int32_t>(glm::floor(cursorPos.y));

    auto cmdBuf = VkUtils::beginSingleTimeCommand();

    gBuffer_->getObjectIdMap().transitionLayout(vk::ImageLayout::eTransferSrcOptimal,vk::PipelineStageFlagBits2::eTransfer,vk::AccessFlagBits2::eTransferRead,cmdBuf);

    VkUtils::copyImageToBuffer(gBuffer_->getObjectIdMap().getVkImage(),idMapTransferBuffer_,xPos,1,yPos,1,cmdBuf);

    gBuffer_->getObjectIdMap().transitionLayout(vk::ImageLayout::eColorAttachmentOptimal,vk::PipelineStageFlagBits2::eColorAttachmentOutput,vk::AccessFlagBits2::eColorAttachmentWrite,cmdBuf);

    VkUtils::endSingleTimeCommand(cmdBuf,VkUtils::QueueType::graphics);

    uint32_t clickedObjectId = *static_cast<uint32_t*>(idMapTransferBuffer_.allocationInfo.pMappedData);

    std::cout << clickedObjectId << std::endl;

    // id 0 is reserved as invalid
    if (clickedObjectId != 0)
        scene_->setSelectedObject(clickedObjectId);
}
