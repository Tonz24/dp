//
// Created by Tonz on 29.07.2025.
//

#include "mesh.h"

#include <imgui/imgui.h>

#include "../engine/engine.h"
#include "../engine/vkUtils.h"

Mesh::Mesh(std::vector<Vertex> &&vertexList, std::vector<uint32_t> &&indexList, std::shared_ptr<Material> material):
    vertices_(std::move(vertexList)), indices_(std::move(indexList)), material_(std::move(material)) {

    initBuffers();
    stage();
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

void Mesh::stage() {

    //  determine staging buffer size (big enough for holding both vertices and indices)
    auto vertexBufferSize = sizeof(vertices_[0]) * vertices_.size();
    auto indexBufferSize = sizeof(indices_[0]) * indices_.size();
    vk::DeviceSize bufferSize = vertexBufferSize > indexBufferSize ? vertexBufferSize : indexBufferSize;

    vk::raii::Buffer stagingBuffer_{nullptr};
    vk::raii::DeviceMemory stagingBufferMemory_{nullptr};

    //  create the staging buffer
    VkUtils::createBuffer(
        bufferSize,
        stagingBuffer_,
        vk::BufferUsageFlagBits::eTransferSrc,
        stagingBufferMemory_,
        VkUtils::stagingMemoryFlags);


    //  map the staging buffer to CPU memory
    void* data = stagingBufferMemory_.mapMemory(0,vertexBufferSize);
    memcpy(data,vertices_.data(),vertexBufferSize);
    //  copy data from staging buffer to vertex buffer
    VkUtils::copyBuffer(stagingBuffer_,vertexBuffer_,vertexBufferSize);

    memcpy(data,indices_.data(), indexBufferSize);
    stagingBufferMemory_.unmapMemory();

    VkUtils::copyBuffer(stagingBuffer_,indexBuffer_,indexBufferSize);
}

void Mesh::recordDrawCommands(vk::raii::CommandBuffer& cmdBuf) const {
    cmdBuf.bindVertexBuffers(0,*vertexBuffer_,{0});
    cmdBuf.bindIndexBuffer(indexBuffer_,0,vk::IndexType::eUint32);
    cmdBuf.drawIndexed(indices_.size(), 1, 0, 0, 0);
}

void Mesh::initBuffers() {
    vk::DeviceSize vertexBufferSize = sizeof(vertices_[0]) * vertices_.size();

    VkUtils::createBuffer(
        vertexBufferSize,
        vertexBuffer_,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vertexBufferMemory_,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    vk::DeviceSize indexBufferSize = sizeof(indices_[0]) * indices_.size();

    VkUtils::createBuffer(
        indexBufferSize,
        indexBuffer_,
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        indexBufferMemory_,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );
}
