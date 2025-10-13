//
// Created by Tonz on 19.09.2025.
//

#include "raytracingPipeline.h"

#include <ranges>

RaytracingPipeline::RaytracingPipeline(const std::vector<ShaderStageInfo>& shaderInfos, const std::span<const vk::DescriptorSetLayout>& descriptorSetLayouts,
                                       const std::span<const vk::PushConstantRange>& pcsRange, uint32_t maxRecursionDepth)
: GraphicsPipeline(descriptorSetLayouts, pcsRange), maxRecursionDepth_(maxRecursionDepth)
{
    initShaderStages(shaderInfos);
    initShaderGroups(shaderInfos);

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
        .pSetLayouts =  descriptorSetLayouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(pcsRange.size()),
        .pPushConstantRanges = pcsRange.data()
    };
    pipelineLayout_ = vk::raii::PipelineLayout( VkUtils::getDevice(), pipelineLayoutInfo);

    vk::RayTracingPipelineCreateInfoKHR rtPipelineInfo{
        .stageCount = static_cast<uint32_t>(shaderStages_.size()),
        .pStages = shaderStages_.data(),
        .groupCount  = static_cast<uint32_t>(shaderGroups_.size()),
        .pGroups =   shaderGroups_.data(),
        .maxPipelineRayRecursionDepth = maxRecursionDepth_,
        .layout = pipelineLayout_,
        .basePipelineHandle = nullptr,
        .basePipelineIndex = -1,
    };

    pipeline_ = VkUtils::getDevice().createRayTracingPipelineKHR(nullptr,nullptr,rtPipelineInfo);

    initSBT();
}

void RaytracingPipeline::initShaderGroups(const std::vector<ShaderStageInfo>& shaderInfos) {

    //for (const auto & shaderInfo : shaderInfos) {
    for (const auto&  [i, shaderInfo]: std::views::enumerate(shaderInfos) | std::views::as_const) {
        extractGroupCounts(shaderInfo);

        vk::RayTracingShaderGroupCreateInfoKHR groupInfo{};
        // Miss and raygen shaders have type eGeneral
        if (shaderInfo.stage == vk::ShaderStageFlagBits::eRaygenKHR || shaderInfo.stage == vk::ShaderStageFlagBits::eMissKHR) {
            groupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
            groupInfo.generalShader = i;
        }
        else { // closest hit shader has type eTrianglesHitGroup (no procedural geometry is used, so eProceduralHitGroup is never used)
            groupInfo.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
            groupInfo.closestHitShader = i;
        }
        shaderGroups_.emplace_back(groupInfo);
    }
}

void RaytracingPipeline::initSBT() {
    auto props = VkUtils::getPhysicalDevice().getProperties2<vk::PhysicalDeviceProperties2,vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
    auto rtProps = props.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

    uint32_t handleSize = rtProps.shaderGroupHandleSize;
    uint32_t handleSizeAlignment = rtProps.shaderGroupHandleAlignment;
    uint32_t groupCount = shaderGroups_.size();
    uint32_t handleCount = 1 + missCount_ + hitCount_;

    uint32_t handleSizeAligned = alignUp(handleSize, handleSizeAlignment);

    uint32_t rgenStride = alignUp(handleSizeAligned,rtProps.shaderGroupBaseAlignment);
    uint32_t rgenSize = rgenStride; //  size must equal stride

    uint32_t missStride = handleSizeAligned;
    uint32_t missSize = alignUp(missCount_ * handleSizeAligned,rtProps.shaderGroupBaseAlignment);

    uint32_t hitStride = handleSizeAligned;
    uint32_t hitSize = alignUp(hitCount_ * handleSizeAligned,rtProps.shaderGroupBaseAlignment);

    uint32_t dataSize = handleCount * handleSize;
    std::vector<uint8_t> handles(dataSize);

    auto* d = VkUtils::getDevice().getDispatcher();
    if (static_cast<vk::Result>(d->vkGetRayTracingShaderGroupHandlesKHR(*VkUtils::getDevice(), *pipeline_,0,handleCount,dataSize,handles.data())) != vk::Result::eSuccess)
        throw std::runtime_error("ERROR: couldn't get shader group handles!");

    vk::DeviceSize sbtSize = rgenSize + missSize + hitSize;

    sbtBuffer_ = VkUtils::createBufferVMA(sbtSize,VkUtils::sbtFlags | vk::BufferUsageFlagBits::eTransferSrc,VkUtils::stagingAllocFlagsVMA);

    vk::DeviceAddress rgenDeviceAddress = sbtBuffer_.deviceAddress;
    vk::DeviceAddress missDeviceAddress = sbtBuffer_.deviceAddress + rgenSize;
    vk::DeviceAddress hitDeviceAddress = sbtBuffer_.deviceAddress + rgenSize + missSize;

    raygenRegion_ = vk::StridedDeviceAddressRegionKHR{.deviceAddress = rgenDeviceAddress, .stride = rgenStride, .size = rgenSize};
    missRegion_ = vk::StridedDeviceAddressRegionKHR{.deviceAddress = missDeviceAddress, .stride = missStride, .size = missSize};
    hitRegion_ = vk::StridedDeviceAddressRegionKHR{.deviceAddress = hitDeviceAddress, .stride = hitStride, .size = hitSize};

    auto getHandle = [&] (int i) { return handles.data() + i * handleSize; };

    auto*    pSBTBuffer = static_cast<uint8_t*>(sbtBuffer_.allocationInfo.pMappedData);
    uint8_t* pData{nullptr};
    uint32_t handleIndex{0};

    pData = pSBTBuffer;
    memcpy(pData,getHandle(handleIndex++),handleSize);

    // Miss
    pData = pSBTBuffer + raygenRegion_.size;
    for(uint32_t c = 0; c < missCount_; c++){
        memcpy(pData, getHandle(handleIndex++), handleSize);
        pData += missRegion_.stride;
    }

    // Hit
    pData = pSBTBuffer + raygenRegion_.size + missRegion_.size;
    for(uint32_t c = 0; c < hitCount_; c++) {
        memcpy(pData, getHandle(handleIndex++), handleSize);
        pData += hitRegion_.stride;
    }
}

void RaytracingPipeline::extractGroupCounts(const ShaderStageInfo& stageInfo) {
    //  do not check for ray gen since there can only be one of them
    if (stageInfo.stage == vk::ShaderStageFlagBits::eMissKHR)
        missCount_ += 1;
    if (stageInfo.stage == vk::ShaderStageFlagBits::eClosestHitKHR || stageInfo.stage == vk::ShaderStageFlagBits::eAnyHitKHR)
        hitCount_ += 1;
}

// https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/#shaderbindingtable
uint32_t RaytracingPipeline::alignUp(uint32_t size, uint32_t alignment) {
    return (size + (alignment - 1)) & ~(alignment - 1);
}