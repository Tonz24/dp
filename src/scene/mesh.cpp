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
    VkUtils::destroyBufferVMA(vertexBuffer_);
    VkUtils::destroyBufferVMA(indexBuffer_);
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

    const PushConstants pcs = {
        .modelMat = transform_.getModelMat(),
        .normalMat = transform_.getNormalMat(),
        .materialId = material_->getCID(),
        .meshId = getCID()
    };

    cmdBuf.pushConstants(pipelineLayout,vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,0, vk::ArrayProxy<const PushConstants>{pcs});

    cmdBuf.drawIndexed(indices_.size(), 1, 0, 0, 0);
}

void Mesh::initBuffers() {
    vk::DeviceSize vertexBufferSize = sizeof(vertices_[0]) * vertices_.size();
    vertexBuffer_ = VkUtils::createBufferVMA(vertexBufferSize,vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst);

    vk::DeviceSize indexBufferSize = sizeof(indices_[0]) * indices_.size();
    indexBuffer_ = VkUtils::createBufferVMA(indexBufferSize,vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst);
}
