//
// Created by Tonz on 29.07.2025.
//

#pragma once
#include <vector>

#include "vertex.h"
#include "../material.h"
#include "../transform.h"

class Mesh : public ManagedResource {
public:

    Mesh(std::vector<Vertex>&& vertexList, std::vector<uint32_t >&& indexList, std::shared_ptr<Material> material);

    [[nodiscard]] const std::vector<Vertex>& getVertices() const{
        return vertices_;
    }

    [[nodiscard]] const std::vector<uint32_t >& getIndices() const{
        return indices_;
    }

    [[nodiscard]] Transform& getTransform() {
        return transform_;
    }

    std::string getResourceType() const override {
        return "Mesh";
    }

    [[nodiscard]] const vk::raii::Buffer & getVertexBuffer() const { return vertexBuffer_; }
    [[nodiscard]] const vk::raii::Buffer & getIndexBuffer() const { return indexBuffer_; }

    void stage();

    void recordDrawCommands(vk::raii::CommandBuffer& cmdBuf) const {
        cmdBuf.bindVertexBuffers(0,*vertexBuffer_,{0});
        cmdBuf.bindIndexBuffer(indexBuffer_,0,vk::IndexType::eUint32);
        cmdBuf.drawIndexed(indices_.size(), 1, 0, 0, 0);
    }

    std::shared_ptr<Material> getMaterial() const {return material_;};

private:
    std::vector<Vertex> vertices_{};
    std::vector<uint32_t> indices_{};
    std::shared_ptr<Material> material_{nullptr};

    vk::raii::Buffer vertexBuffer_{nullptr};
    vk::raii::DeviceMemory vertexBufferMemory_{nullptr};

    vk::raii::Buffer indexBuffer_{nullptr};
    vk::raii::DeviceMemory indexBufferMemory_{nullptr};

    Transform transform_{};

    void initBuffers();

    void initVertexBuffer();

    void initIndexBuffer();
};
