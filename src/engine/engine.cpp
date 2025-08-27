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

#include "managers/inputManager.h"
#include "vk/vkUtils.h"
#include "../scene/texture.h"
#include "managers/resourceManager.h"

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
    window = std::make_unique<Window>("DP",1280,720,false);

    glfwSetWindowUserPointer(window->getGlfwWindow(),this);
    glfwSetFramebufferSizeCallback(window->getGlfwWindow(),framebufferResizeCallback);
    glfwSetCursorPosCallback(window->getGlfwWindow(),mouseMovementCallback);
    glfwSetKeyCallback(window->getGlfwWindow(),InputManager::keyCallback);

    initVulkan();
    initImGui();

    isInitialized_ = true;
}

bool Engine::drawGUI() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("DP");

    if (scene_)
        return scene_->drawGUI();

    return false;
}


void Engine::initGLFW() {

    if (!glfwInit())
        throw std::runtime_error("ERROR: Failed to initialize GLFW!");

    if (!glfwVulkanSupported())
        throw std::runtime_error("ERROR: Vulkan not supported by GLFW!");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
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
    auto idMapFormat = static_cast<VkFormat>(idMapFormat_);

    std::array formats = {format, /*idMapFormat*/};

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
            .colorAttachmentCount = formats.size(),
            .pColorAttachmentFormats = formats.data(),
            .depthAttachmentFormat = static_cast<VkFormat>(depthFormat_),
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

    if (ENABLE_VALIDATION_LAYERS)
        initDebugMessenger();

    initSurface();

    initPhysicalDevice();
    initLogicalDevice();

    configureVkUtils();

    initSwapchain();
    initImageViews();

    initDescriptorPool();
    initUniformBuffers();
    initDescriptorSetLayout();
    initDescriptorSetLayout2();

    initCommandPool();
    initCommandBuffers();

    initDepthResources();
    initGraphicsPipeline();

    initSyncObjects();

    initIdMapImage();
    initDummyTexture();


    sky_ = TextureManager::getInstance()->registerResource("sky", "../assets/sky/lebombo_4k.exr");

    VkUtils::BufferAlloc stagingBuffer = VkUtils::createBufferVMA(sky_->getTotalSize(),vk::BufferUsageFlagBits::eTransferSrc,VkUtils::stagingAllocFlagsVMA);
    sky_->stage(stagingBuffer);
    VkUtils::destroyBufferVMA(stagingBuffer);

    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = descriptorPool_,
        .descriptorSetCount = 1,
        .pSetLayouts = &*descriptorSetLayoutSky_
    };
    skyDescriptorSet_ = std::move(device_.allocateDescriptorSets(allocInfo).front());

    vk::DescriptorImageInfo descInfo{
        .sampler = sky_->getVkSampler(),
        .imageView = sky_->getVkImageView(),
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };

    vk::WriteDescriptorSet writeDescriptorSet{
        .dstSet = skyDescriptorSet_,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &descInfo
    };

    device_.updateDescriptorSets(writeDescriptorSet,{});
}

void Engine::initVulkanInstance() {
    auto requiredExtensions = initRequiredInstanceExtensions();
    auto validationLayers = initValidationLayers();

    vk::ApplicationInfo appInfo{
            .pApplicationName = "DP",
            .applicationVersion = vk::makeVersion(1,0,0),
            .pEngineName = nullptr,
            .engineVersion = vk::makeVersion(1,0,0),
            .apiVersion = vk::ApiVersion13
    };

    vk::InstanceCreateInfo createInfo{
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

void Engine::initVMAllocator() {}

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
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
        vk::PhysicalDeviceRayTracingPipelineFeaturesKHR,
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR,
        vk::PhysicalDeviceMemoryPriorityFeaturesEXT,
        vk::PhysicalDevicePageableDeviceLocalMemoryFeaturesEXT
        >
            featureChain {
                {.features = {.samplerAnisotropy = vk::True}},                               // vk::PhysicalDeviceFeatures2 (empty for now)
                {.synchronization2 = vk::True, .dynamicRendering = vk::True},      // Enable dynamic rendering from Vulkan 1.3
                {.extendedDynamicState = vk::True }, // Enable extended dynamic state from the extension_
                {},
                {},
                {},
                {}
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
       .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
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
    //return vk::PresentModeKHR::eImmediate;
    return vk::PresentModeKHR::eFifo;
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

    if (ENABLE_VALIDATION_LAYERS)
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

void Engine::initGraphicsPipeline() {
    std::vector descriptorSetLayouts = {*descriptorSetLayoutFrame_, *descriptorSetLayoutMaterial_};
    std::vector colorAttachmentFormats = {swapChainImageFormat, idMapFormat_};

    std::span colorAttachmentFormatsSky{colorAttachmentFormats.begin(),1};

    std::vector descriptorSetLayoutsSky = {*descriptorSetLayoutFrame_, *descriptorSetLayoutSky_};
    rasterPipeline_ = GraphicsPipeline{"shaders/shader_vert.spv","shaders/shader_frag.spv",descriptorSetLayouts,colorAttachmentFormats,true, depthFormat_};
    skyboxPipeline_ = GraphicsPipeline{"shaders/skypass_vert.spv","shaders/skypass_frag.spv",descriptorSetLayoutsSky,colorAttachmentFormatsSky, false};
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
        .commandBufferCount = maxFramesInFlight
    };

    commandBuffers_ = vk::raii::CommandBuffers(device_,commandBufferAllocInfo);
}

void Engine::recordCommandBuffer(uint32_t imageIndex, uint32_t frameInFlightIndex, vk::raii::CommandBuffer &cmdBuf) {
    cmdBuf.reset();
    cmdBuf.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    //  the old layout is undefined
    //  the new layout is color attachment optimal
    //
    //  the old stage mask is TopOfPipe
    //  the old access mask is none
    //
    //  the new stage mask is color attachment output
    //  the new access mask is color attachment write
    VkUtils::transitionImageLayout(swapChainImages[imageIndex],
                                   vk::ImageLayout::eUndefined,
                                   vk::ImageLayout::eColorAttachmentOptimal,                           //
                                   vk::PipelineStageFlagBits2::eColorAttachmentOutput | vk::PipelineStageFlagBits2::eTopOfPipe,
                                   vk::AccessFlagBits2::eNone,
                                   vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                   vk::AccessFlagBits2::eColorAttachmentWrite,
                                   vk::ImageAspectFlagBits::eColor,
                                   cmdBuf);



    renderSky(cmdBuf,imageIndex, frameInFlightIndex);
    renderScene(cmdBuf,imageIndex, frameInFlightIndex);
    renderGUI(cmdBuf, imageIndex);

    //  the old layout is attachment optimal
    //  the new layout is color present src
    //
    //  the old stage mask is color attachment output
    //  the old access mask is color attachment write
    //
    //  the new stage mask is Bottom of pipe
    //  the new access mask is none (nothing else accesses this image)
    VkUtils::transitionImageLayout(swapChainImages[imageIndex],
                                   vk::ImageLayout::eColorAttachmentOptimal,
                                   vk::ImageLayout::ePresentSrcKHR,
                                   vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                   vk::AccessFlagBits2::eColorAttachmentWrite,
                                   vk::PipelineStageFlagBits2::eBottomOfPipe,
                                   vk::AccessFlagBits2::eNone,
                                   vk::ImageAspectFlagBits::eColor,
                                   cmdBuf);

    cmdBuf.end();
}

std::vector<const char *> Engine::initValidationLayers() {
    std::vector<const char*> requiredLayers = ENABLE_VALIDATION_LAYERS ? requiredValidationLayers : std::vector<const char*>();

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
    vk::raii::CommandBuffer& commandBuffer = commandBuffers_[frameInFlightIndex_];

    recordCommandBuffer(imageIndex, frameInFlightIndex_, commandBuffer);

    //  set up the submit info for drawing
    //  set up the wait stage mask as color attachment output
    vk::PipelineStageFlags waitDestinationStageMask{vk::PipelineStageFlagBits::eColorAttachmentOutput};
    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*acquireSemaphore,
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffer,
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
    frameInFlightIndex_ = currentFrameIndex_ % maxFramesInFlight;
}

void Engine::processInput() {

    glm::vec3 velocity{};

    if (glfwGetKey(window->getGlfwWindow(),GLFW_KEY_A) == GLFW_PRESS) {
         velocity.x -= 1.0f;
    }
    if (glfwGetKey(window->getGlfwWindow(),GLFW_KEY_D) == GLFW_PRESS) {
        velocity.x += 1.0f;
    }

    if (glfwGetKey(window->getGlfwWindow(),GLFW_KEY_W) == GLFW_PRESS) {
        velocity.z += 1.0f;
    }
    if (glfwGetKey(window->getGlfwWindow(),GLFW_KEY_S) == GLFW_PRESS) {
        velocity.z -= 1.0f;
    }
    if (window->getCursorMode() == Window::CursorMode::disabled)
        scene_->getCamera().updatePosition(velocity);
}

void Engine::mainLoop() {
    isRunning_ = true;

    while(!glfwWindowShouldClose(window->getGlfwWindow())) {
        glfwPollEvents();
        processInput();
        drawGUI();

        ImGui::End();
        ImGui::Render();

        drawFrame();
    }
    isRunning_ = false;
    device_.waitIdle();
}

void Engine::cleanup() {
    scene_.reset();

    depthTexture_.reset();
    objectIdMap_.reset();
    dummy_.reset();


    cleanUBOs();

    VkUtils::destroy();
    glfwTerminate();
}

void Engine::initSyncObjects() {
    for (uint32_t i = 0; i < maxFramesInFlight; ++i) {
        inFlightFences_.emplace_back(device_,vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
        acquireSemaphores_.emplace_back(device_, vk::SemaphoreCreateInfo{});
    }

    for (uint32_t i = 0; i < swapChainImages.size(); ++i) {
        submitSemaphores_.emplace_back(device_, vk::SemaphoreCreateInfo{});
    }
}

void Engine::initDescriptorSetLayout() {

    //  Frame descriptor layout first
    //  camera UBO

    std::array frameDescriptorBindings{
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

    vk::DescriptorSetLayoutCreateInfo frameLayoutInfo{
        .bindingCount = static_cast<uint32_t>(frameDescriptorBindings.size()),
        .pBindings = frameDescriptorBindings.data()
    };
    descriptorSetLayoutFrame_ = vk::raii::DescriptorSetLayout(device_,frameLayoutInfo);


    //  Material descriptor layout second
    std::vector<vk::DescriptorSetLayoutBinding> materialBindings{};

    for (uint32_t i = 0; i < 4; ++i) {
        vk::DescriptorSetLayoutBinding materialBinding{
            .binding = i,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment,
        };
        materialBindings.emplace_back(materialBinding);
    }


    vk::DescriptorSetLayoutCreateInfo materialLayoutInfo{
        .bindingCount = static_cast<uint32_t>(materialBindings.size()),
        .pBindings = materialBindings.data()
    };
    descriptorSetLayoutMaterial_ = vk::raii::DescriptorSetLayout(device_,materialLayoutInfo);

    std::vector<vk::DescriptorSetLayout> layouts(maxFramesInFlight,*descriptorSetLayoutFrame_);
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = descriptorPool_,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()
    };

    descriptorSets_ = device_.allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < maxFramesInFlight; i++) {

        vk::DescriptorBufferInfo camBufferInfo{
            .buffer = cameraUBOs_[i].buffer,
            .offset = 0,
            .range = sizeof(CameraUBOFormat)
        };

        vk::DescriptorBufferInfo matBufferInfo{
            .buffer = materialUBOs_[i].buffer,
            .offset = 0,
            .range = sizeof(MaterialUBOFormat) * materialLimit
        };

        vk::WriteDescriptorSet writeDescriptorSetCam{
            .dstSet = descriptorSets_[i], //  which descriptor set to update
            .dstBinding = 0, // which binding to update
            .dstArrayElement = 0, //  what element the update starts at
            .descriptorCount = 1, //  how many descriptors are affected
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &camBufferInfo,
        };

        vk::WriteDescriptorSet writeDescriptorSetMat{
            .dstSet = descriptorSets_[i], //  which descriptor set to update
            .dstBinding = 1, // which binding to update
            .dstArrayElement = 0, //  what element the update starts at
            .descriptorCount = 1, //  how many descriptors are affected
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &matBufferInfo,
        };

        device_.updateDescriptorSets({writeDescriptorSetCam, writeDescriptorSetMat},{});
    }
}

void Engine::initDescriptorSetLayout2() {

    vk::DescriptorSetLayoutBinding skyBinding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
    };

    vk::DescriptorSetLayoutCreateInfo skyLayoutInfo{
        .bindingCount = 1,
        .pBindings = &skyBinding
    };

    descriptorSetLayoutSky_ = vk::raii::DescriptorSetLayout(device_,skyLayoutInfo);
}

void Engine::initUniformBuffers() {
    cameraUBOs_.clear();
    cameraUBOsMapped_.clear();

    materialUBOs_.clear();
    materialUBOsMapped_.clear();

    // camera UBO
    for (uint32_t i = 0; i < maxFramesInFlight; ++i) {
        vk::DeviceSize bufferSize = sizeof(CameraUBOFormat);
        auto allocationCreateFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        auto buffer = VkUtils::createBufferVMA(bufferSize,vk::BufferUsageFlagBits::eUniformBuffer, allocationCreateFlags);

        cameraUBOsMapped_.emplace_back(static_cast<unsigned char*>(buffer.allocationInfo.pMappedData));
        cameraUBOs_.emplace_back(std::move(buffer));

    }

    // material UBO
    for (uint32_t i = 0; i < maxFramesInFlight; ++i) {

        vk::DeviceSize bufferSize = sizeof(MaterialUBOFormat) * materialLimit;
        auto allocationCreateFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        auto buffer = VkUtils::createBufferVMA(bufferSize,vk::BufferUsageFlagBits::eUniformBuffer, allocationCreateFlags);

        materialUBOsMapped_.emplace_back(static_cast<unsigned char*>(buffer.allocationInfo.pMappedData));
        materialUBOs_.emplace_back(std::move(buffer));
    }
}

void Engine::initDescriptorPool() {
    std::array poolSize{
        vk::DescriptorPoolSize {
            .type = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1000
        },
        vk::DescriptorPoolSize {
            .type = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1000
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

void Engine::renderSky(vk::raii::CommandBuffer& cmdBuf, uint32_t imageIndex, uint32_t frameInFlightIndex) {

    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);
    vk::RenderingAttachmentInfo colorAttachmentInfo = {
        .imageView = swapChainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor
    };

    vk::RenderingInfo renderingInfo{
        .renderArea = {
            .offset = {
                .x = 0,
                .y = 0
            },
            .extent = swapChainExtent
    },
    .layerCount = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorAttachmentInfo,
    };

    //begin rendering with the specified info
    cmdBuf.beginRendering(renderingInfo);

    //  set dynamic rendering state values
    const vk::Viewport viewport{
        .x = 0,
        .y = static_cast<float>(swapChainExtent.height),
        .width = static_cast<float>(swapChainExtent.width),
        .height = -static_cast<float>(swapChainExtent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    const vk::Rect2D scissor{
        .offset = vk::Offset2D{
            .x = 0,
            .y = 0
        },
        .extent =  swapChainExtent
    };

    cmdBuf.setViewport(0, viewport);
    cmdBuf.setScissor(0, scissor);

    //  bind graphics pipeline and global descriptor set
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, skyboxPipeline_.getGraphicsPipeline());

    //  bind global descriptor set
    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, skyboxPipeline_.getPipelineLayout(), 0, *descriptorSets_[frameInFlightIndex], nullptr);

    //  bind per mesh descriptor set
    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, skyboxPipeline_.getPipelineLayout(), 1, *skyDescriptorSet_, nullptr);

    const PushConstants pcs = { };

    cmdBuf.pushConstants(skyboxPipeline_.getPipelineLayout(),vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,0, vk::ArrayProxy<const PushConstants>{pcs});

    // draw six vertices making up the screen quad
    cmdBuf.draw(6, 1, 0, 0);
    cmdBuf.endRendering();
}

void Engine::renderScene(vk::raii::CommandBuffer& cmdBuf, uint32_t imageIndex, uint32_t frameInFlightIndex) {
    //set up the color attachment
    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);
    vk::RenderingAttachmentInfo colorAttachmentInfo = {
        .imageView = swapChainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor
    };

    vk::RenderingAttachmentInfo idMapAttachmentInfo = {
        .imageView = objectIdMap_->getVkImageView(),
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor
    };

    std::array colorAttachmentInfos = {colorAttachmentInfo,idMapAttachmentInfo};

    vk::ClearValue depthClearColor = vk::ClearDepthStencilValue(1.0f,0);
    vk::RenderingAttachmentInfo depthAttachmentInfo = {
        .imageView = depthTexture_->getVkImageView(),
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = depthClearColor
    };

    vk::RenderingInfo renderingInfo{
        .renderArea = {
                .offset = {
                    .x = 0,
                    .y = 0
                },
                .extent = swapChainExtent
        },
        .layerCount = 1,
        .colorAttachmentCount = colorAttachmentInfos.size(),
        .pColorAttachments = colorAttachmentInfos.data(),
        .pDepthAttachment = &depthAttachmentInfo,
    };

    //begin rendering with the specified info
    cmdBuf.beginRendering(renderingInfo);

    //  set dynamic rendering state values
    const vk::Viewport viewport{
        .x = 0,
        .y = static_cast<float>(swapChainExtent.height),
        .width = static_cast<float>(swapChainExtent.width),
        .height = -static_cast<float>(swapChainExtent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    const vk::Rect2D scissor{
        .offset = vk::Offset2D{
            .x = 0,
            .y = 0
        },
        .extent =  swapChainExtent
    };

    cmdBuf.setViewport(0, viewport);
    cmdBuf.setScissor(0, scissor);

    //  bind graphics pipeline and global descriptor set
    cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, rasterPipeline_.getGraphicsPipeline());
    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, rasterPipeline_.getPipelineLayout(), 0, *descriptorSets_[frameInFlightIndex], nullptr);

    for (const auto &mesh : scene_->getMeshes()) {
        mesh->recordDrawCommands(cmdBuf, rasterPipeline_.getPipelineLayout());
    }
    cmdBuf.endRendering();
}

void Engine::renderGUI(vk::raii::CommandBuffer& cmdBuf, uint32_t imageIndex) {
    // prepare GUI render pass
    vk::RenderingAttachmentInfo guiAttachmentInfo = {
        .imageView = swapChainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eStore,
    };

    vk::RenderingInfo guiRenderingInfo{
        .renderArea = {
            .offset = { .x = 0, .y = 0 },
            .extent = swapChainExtent
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &guiAttachmentInfo,
        .pDepthAttachment = nullptr,
    };

    // render GUI separately
    cmdBuf.beginRendering(guiRenderingInfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),*cmdBuf);
    cmdBuf.endRendering();
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

    cleanupSwapchain();
    initSwapchain();
    initImageViews();
    initDepthResources();
    initIdMapImage();
}

void Engine::cleanupSwapchain() {
    swapChainImageViews.clear();
    swapChain = nullptr;
}

void Engine::initDepthResources() {
    int width{},height{};
    glfwGetFramebufferSize(window->getGlfwWindow(),&width,&height);

    depthTexture_ = TextureManager::getInstance()->registerResource("depth_texture",width, height, 1,depthFormat_, vk::ImageUsageFlagBits::eDepthStencilAttachment,vk::MemoryPropertyFlagBits::eDeviceLocal);

    auto cmdBuf = VkUtils::beginSingleTimeCommand();

    VkUtils::transitionImageLayout(depthTexture_->getVkImage(),
                                   vk::ImageLayout::eUndefined,
                                   vk::ImageLayout::eDepthAttachmentOptimal,
                                   vk::PipelineStageFlagBits2::eTopOfPipe,
                                   vk::AccessFlagBits2::eNone,
                                   vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
                                   vk::AccessFlagBits2::eDepthStencilAttachmentWrite | vk::AccessFlagBits2::eDepthStencilAttachmentRead,
                                   vk::ImageAspectFlagBits::eDepth,
                                   cmdBuf);

    VkUtils::endSingleTimeCommand(cmdBuf,VkUtils::QueueType::graphics);
}

void Engine::initIdMapImage() {
    int width{},height{};
    glfwGetFramebufferSize(window->getGlfwWindow(),&width,&height);

    objectIdMap_ = TextureManager::getInstance()->registerResource("id_map_texture", width, height, 1,idMapFormat_, vk::ImageUsageFlagBits::eColorAttachment,vk::MemoryPropertyFlagBits::eDeviceLocal);

    auto cmdBuf = VkUtils::beginSingleTimeCommand();

    VkUtils::transitionImageLayout(objectIdMap_->getVkImage(),
                                   vk::ImageLayout::eUndefined,
                                   vk::ImageLayout::eColorAttachmentOptimal,
                                   vk::PipelineStageFlagBits2::eTopOfPipe,
                                   vk::AccessFlagBits2::eNone,
                                   vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                   vk::AccessFlagBits2::eColorAttachmentWrite,
                                   vk::ImageAspectFlagBits::eColor,
                                   cmdBuf);

    VkUtils::endSingleTimeCommand(cmdBuf,VkUtils::QueueType::graphics);
}

void Engine::configureVkUtils() const {

    VkUtils::init(&device_,&physicalDevice, &vkInstance, {&graphicsQueue,&presentQueue,&transferQueue}, &graphicsCommandPool_);
}

void Engine::updateUBOs() {

    if (dirtyCameraUBO_) {
        memcpy(cameraUBOsMapped_[frameInFlightIndex_],&cameraUBOStorage_,sizeof(cameraUBOStorage_));
        dirtyCameraUBO_ = false;
   }

    if (dirtyMaterialUBO_) {
        uint8_t* dst = materialUBOsMapped_[frameInFlightIndex_] + materialUpdateIndex_ * sizeof(materialUBOStorage_);
        memcpy(dst, &materialUBOStorage_,sizeof(materialUBOStorage_));
        dirtyMaterialUBO_ = false;
    }
}

void Engine::cleanUBOs() {
    for (uint32_t i = 0; i < maxFramesInFlight; ++i) {
        VkUtils::destroyBufferVMA(cameraUBOs_[i]);
        VkUtils::destroyBufferVMA(materialUBOs_[i]);
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

