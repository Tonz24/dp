//
// Created by Tonz on 10.09.2025.
//

#include "accelerationStructure.h"

AccelerationStructure::AccelerationStructure(vk::AccelerationStructureTypeKHR type, const vk::AccelerationStructureGeometryKHR& geometry,
    uint32_t primitiveCount) {

     //  setup build info (scratchData and dstAccelerationStructure are filled later)
    vk::AccelerationStructureBuildGeometryInfoKHR buildInfo{
        .type = type,
        .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace, // TODO: allow compaction later
        .mode = vk::BuildAccelerationStructureModeKHR::eBuild,
        .geometryCount = 1,
        .pGeometries  = &geometry,
    };


    //  get build size, setup buffer flags
    auto buildSize = VkUtils::getDevice().getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice,buildInfo,primitiveCount);

    // get the minimum scratch alignment
    auto props = VkUtils::getPhysicalDevice().getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();
    vk::DeviceAddress minimumScratchAlignment = props.get<vk::PhysicalDeviceAccelerationStructurePropertiesKHR>().minAccelerationStructureScratchOffsetAlignment;

    // create blas and scratch buffers (blas buffer is a member)
    storageBuffer_ = VkUtils::createBufferVMA(buildSize.accelerationStructureSize,VkUtils::accelStructStorageFlags);
    VkUtils::BufferAlloc blasScratchBuffer = VkUtils::createBufferVMA(buildSize.buildScratchSize, VkUtils::scratchBufferFlags, minimumScratchAlignment);
    vk::DeviceAddress scratchBufferAddress = VkUtils::getDevice().getBufferAddress({.buffer =  blasScratchBuffer.buffer});


    vk::AccelerationStructureCreateInfoKHR blasCreateInfo{
        .buffer = storageBuffer_.buffer,
        .size = buildSize.accelerationStructureSize,
        .type = type,
    };

    //  create the BLAS
    accelStruct_ = VkUtils::getDevice().createAccelerationStructureKHR(blasCreateInfo);

    //  fill the remaining buildInfo data with scratch buffer and destination BLAS
    buildInfo.scratchData = scratchBufferAddress;
    buildInfo.dstAccelerationStructure = *accelStruct_;

    //  specify the range of primitives to build the BLAS from (entire buffer in this case)
    vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo{
        .primitiveCount = primitiveCount,
        .primitiveOffset = 0,
        .firstVertex = 0,
        .transformOffset = 0
    };


    const vk::AccelerationStructureBuildRangeInfoKHR* pRangeInfos[] = { &buildRangeInfo };

    // build the BLAS
    auto cmdBuf = VkUtils::beginSingleTimeCommand();
    cmdBuf.buildAccelerationStructuresKHR(buildInfo, pRangeInfos);
    VkUtils::endSingleTimeCommand(cmdBuf,VkUtils::QueueType::graphics);


    VkUtils::destroyBufferVMA(std::move(blasScratchBuffer));
}

AccelerationStructure::~AccelerationStructure() {
    accelStruct_.release();
    VkUtils::destroyBufferVMA(std::move(storageBuffer_));
}
