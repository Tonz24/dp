//
// Created by Tonz on 29.07.2025.
//

#include "mesh.h"
#include "../../engine.h"

Mesh::Mesh(std::vector<Vertex> &&vertexList, std::vector<uint32_t> &&indexList, std::shared_ptr<Material> material):
    vertices_(std::move(vertexList)), indices_(std::move(indexList)), material_(std::move(material)) {

    initBuffers();
    stage();
}

void Mesh::stage() {

    //  determine staging buffer size (big enough for holding both vertices and indices)
    auto vertexBufferSize = sizeof(vertices_[0]) * vertices_.size();
    auto indexBufferSize = sizeof(indices_[0]) * indices_.size();
    vk::DeviceSize bufferSize = vertexBufferSize > indexBufferSize ? vertexBufferSize : indexBufferSize;

    vk::raii::Buffer stagingBuffer_{nullptr};
    vk::raii::DeviceMemory stagingBufferMemory_{nullptr};
    //  create the staging buffer
    Engine::getInstance().createBuffer(
        bufferSize,
        stagingBuffer_,
        vk::BufferUsageFlagBits::eTransferSrc,
        stagingBufferMemory_,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);


    //  map the staging buffer to CPU memory
    void* data = stagingBufferMemory_.mapMemory(0,vertexBufferSize);
    memcpy(data,vertices_.data(),vertexBufferSize);
    //  copy data from staging buffer to vertex buffer
    Engine::getInstance().copyBuffer(stagingBuffer_,vertexBuffer_,vertexBufferSize);

    memcpy(data,indices_.data(), indexBufferSize);
    stagingBufferMemory_.unmapMemory();

    Engine::getInstance().copyBuffer(stagingBuffer_,indexBuffer_,indexBufferSize);
}

void Mesh::initBuffers() {
    initVertexBuffer();
    initIndexBuffer();
}

void Mesh::initVertexBuffer() {

    vk::DeviceSize bufferSize = sizeof(vertices_[0]) * vertices_.size();

    Engine::getInstance().createBuffer(
        bufferSize,
        vertexBuffer_,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vertexBufferMemory_,
        vk::MemoryPropertyFlagBits::eDeviceLocal
        );
}

void Mesh::initIndexBuffer() {
    vk::DeviceSize bufferSize = sizeof(indices_[0]) * indices_.size();

    Engine::getInstance().createBuffer(
        bufferSize,
        indexBuffer_,
        vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        indexBufferMemory_,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

}