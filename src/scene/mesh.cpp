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

void Mesh::updateBLASInstance() {

    const auto& modelMat = transform_.getModelMat();


    vk::TransformMatrixKHR transformMatrix{
        .matrix = std::array{
            std::array{modelMat[0][0],modelMat[1][0],modelMat[2][0],modelMat[3][0]},
            std::array{modelMat[0][1],modelMat[1][1],modelMat[2][1],modelMat[3][1]},
            std::array{modelMat[0][2],modelMat[1][2],modelMat[2][2],modelMat[3][2]}
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

bool Mesh::drawGUI() {
    bool changed{false};
    if (ImGui::CollapsingHeader("Mesh",ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        changed = transform_.drawGUI();
        material_->drawGUI();
        ImGui::Unindent();
    }

    if (changed) // change BLAS instance transformation matrix
        updateBLASInstance();
    return changed;
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
    vertexBuffer_ = VkUtils::createBufferVMA(vertexBufferSize,vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst | VkUtils::accelStructInputFlags);

    vk::DeviceSize indexBufferSize = sizeof(indices_[0]) * indices_.size();
    indexBuffer_ = VkUtils::createBufferVMA(indexBufferSize,vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst |  VkUtils::accelStructInputFlags);
}


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

    // setup instance
    updateBLASInstance();
}
