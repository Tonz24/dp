//
// Created by Tonz on 29.07.2025.
//

#include "mesh.h"

#include <imgui/imgui.h>
#include "../engine/engine.h"

Mesh::Mesh(std::vector<Vertex3D>&& vertexList, std::vector<uint32_t>&& indexList, std::shared_ptr<Material> material):
    vertices_(std::move(vertexList)), indices_(std::move(indexList)), material_(std::move(material)) {

    initBuffers();
}

Mesh::~Mesh() {
    VkUtils::destroyBufferVMA(std::move(vertexBuffer_));
    VkUtils::destroyBufferVMA(std::move(indexBuffer_));
}

bool Mesh::drawGUI() {
    if (ImGui::CollapsingHeader("Mesh")) {
        ImGui::Indent();
        transform_.drawGUI();
        material_->drawGUI();
        ImGui::Unindent();
    }

    return false;
}

void Mesh::stage(const VkUtils::BufferAlloc& stagingBuffer) const {

    if (stagingBuffer.allocationInfo.pMappedData == nullptr)
        throw std::runtime_error("ERROR: Mapped pointer points to NULL!");

    auto vertexBufferSize = sizeof(vertices_[0]) * vertices_.size();
    auto indexBufferSize = sizeof(indices_[0]) * indices_.size();

    //  copy from vertices vector to staging buffer
    memcpy(stagingBuffer.allocationInfo.pMappedData,vertices_.data(),vertexBufferSize);

    //  copy from staging buffer to vertex buffer
    VkUtils::copyBuffer(stagingBuffer,vertexBuffer_,vertexBufferSize);

    // copy from indices vector to staging buffer
    memcpy(stagingBuffer.allocationInfo.pMappedData,indices_.data(), indexBufferSize);

    // copy from staging buffer to indices buffer
    VkUtils::copyBuffer(stagingBuffer,indexBuffer_,indexBufferSize);
}

void Mesh::recordDrawCommands(vk::raii::CommandBuffer& cmdBuf, const vk::raii::PipelineLayout& pipelineLayout) const {
    cmdBuf.bindVertexBuffers(0,vertexBuffer_.buffer,{0});
    cmdBuf.bindIndexBuffer(indexBuffer_.buffer,0,vk::IndexType::eUint32);
    //  bind per mesh descriptor set
    cmdBuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, *getMaterial()->getDescriptorSet(), nullptr);

    const PcsGBufferFill pcs = {
        .modelMat = transform_.getModelMat(),
        .normalMat = transform_.getNormalMat(),
        .materialId = material_->getCID(),
        .meshId = getCID()
    };

    cmdBuf.pushConstants(pipelineLayout,vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,0, vk::ArrayProxy<const PcsGBufferFill>{pcs});

    cmdBuf.drawIndexed(indices_.size(), 1, 0, 0, 0);
}

void Mesh::initBuffers() {
    vk::DeviceSize vertexBufferSize = sizeof(vertices_[0]) * vertices_.size();
    vertexBuffer_ = VkUtils::createBufferVMA(vertexBufferSize,vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR);

    vk::DeviceSize indexBufferSize = sizeof(indices_[0]) * indices_.size();
    indexBuffer_ = VkUtils::createBufferVMA(indexBufferSize,vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR);
}

// https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/#raytracingsetup/main
// https://github.com/yknishidate/single-file-vulkan-pathtracing/blob/master/main.cpp
void Mesh::initBLAS() {

    uint32_t maxPrimitiveCount = static_cast<uint32_t>(indices_.size() / 3);


    //  setup triangle data (consume the whole vertex/index buffer pair for one BLAS)
    vk::AccelerationStructureGeometryTrianglesDataKHR triangleData{};
        triangleData.setVertexFormat(vk::Format::eR32G32B32Sfloat);
        triangleData.setVertexData(vertexBuffer_.deviceAddress + offsetof(Vertex3D,position)); // in case the vertex struct changes)
        triangleData.setVertexStride(sizeof(Vertex3D));
        triangleData.setMaxVertex( static_cast<uint32_t>(vertices_.size() - 1));
        triangleData.setIndexType(vk::IndexType::eUint32);
        triangleData.setIndexData(indexBuffer_.deviceAddress);
        triangleData.setTransformData(nullptr);


    //  set everything as opaque triangles for now
    vk::AccelerationStructureGeometryKHR geometryData{};
    geometryData.setGeometryType(vk::GeometryTypeKHR::eTriangles);
    geometryData.setGeometry(triangleData);
    geometryData.setFlags(vk::GeometryFlagBitsKHR::eOpaque); //TODO: change to no opaque if transparent materials are present

    blas_ = AccelerationStructure( vk::AccelerationStructureTypeKHR::eBottomLevel,geometryData,maxPrimitiveCount);

    /*//  specify the range of primitives to build the BLAS from (entire buffer in this case)
    vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo{
        .primitiveCount = maxPrimitiveCount,
        .primitiveOffset = 0,
        .firstVertex = 0,
        .transformOffset = 0
    };

    //  each mesh is responsible for its own BLAS, TLAS is maintained by scene
    vk::AccelerationStructureTypeKHR buildType = vk::AccelerationStructureTypeKHR::eBottomLevel;

    //  setup build info (scratchData and dstAccelerationStructure are filled later)
    vk::AccelerationStructureBuildGeometryInfoKHR buildInfo{
        .type = buildType,
        .flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace, // TODO: allow compaction later
        .mode = vk::BuildAccelerationStructureModeKHR::eBuild,
        .geometryCount = 1,
        .pGeometries  = &geometryData,
    };


    //  get build size, setup buffer flags
    auto buildSize = VkUtils::getDevice().getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice,buildInfo,{maxPrimitiveCount});

    // get the minimum scratch alignment
    auto props = VkUtils::getPhysicalDevice().getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();
    vk::DeviceAddress minimumScratchAlignment = props.get<vk::PhysicalDeviceAccelerationStructurePropertiesKHR>().minAccelerationStructureScratchOffsetAlignment;

    // create blas and scratch buffers (blas buffer is a member)
    blasStorageBuffer_ = VkUtils::createBufferVMA(buildSize.accelerationStructureSize,VkUtils::accelStructStorageFlags);
    VkUtils::BufferAlloc blasScratchBuffer = VkUtils::createBufferVMA(buildSize.buildScratchSize, VkUtils::scratchBufferFlags, minimumScratchAlignment);
    vk::DeviceAddress scratchBufferAddress = VkUtils::getDevice().getBufferAddress({.buffer =  blasScratchBuffer.buffer});


    vk::AccelerationStructureCreateInfoKHR blasCreateInfo{
        .buffer = blasStorageBuffer_.buffer,
        .size = buildSize.accelerationStructureSize,
        .type = buildType,
    };

    //  create the BLAS
    blas_ = VkUtils::getDevice().createAccelerationStructureKHR(blasCreateInfo);

    //  fill the remaining buildInfo data with scratch buffer and destination BLAS
    buildInfo.scratchData = scratchBufferAddress;
    buildInfo.dstAccelerationStructure = *blas_;


    const vk::AccelerationStructureBuildRangeInfoKHR* pRangeInfos[] = { &buildRangeInfo };

    // build the BLAS
    auto cmdBuf = VkUtils::beginSingleTimeCommand();
    //cmdBuf.buildAccelerationStructuresKHR(h,p);
    cmdBuf.buildAccelerationStructuresKHR(buildInfo, pRangeInfos);
    VkUtils::endSingleTimeCommand(cmdBuf,VkUtils::QueueType::graphics);


    VkUtils::destroyBufferVMA(std::move(blasScratchBuffer));
    */

    // setup instance
    vk::TransformMatrixKHR transformMatrix{
        .matrix = std::array{
            std::array{1.0f,0.0f,0.0f,0.0f},
            std::array{0.0f,1.0f,0.0f,0.0f},
            std::array{0.0f,0.0f,1.0f,0.0f}
        }
    };

    //  just one instance per mesh for now
    blasInstance_ = vk::AccelerationStructureInstanceKHR{
        .transform = transformMatrix,
        .instanceCustomIndex = getCID(),
        .mask = 0xFF,
        .flags = static_cast<VkGeometryInstanceFlagsKHR>(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable),
        .accelerationStructureReference = blas_.getStorageBuffer().deviceAddress,
    };
}
