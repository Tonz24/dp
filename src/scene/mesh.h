//
// Created by Tonz on 29.07.2025.
//

#pragma once
#include <vector>

#include "Vertex.h"
#include "material.h"
#include "transform.h"
#include "../engine/iDrawGui.h"
#include "../engine/vk/vkUtils.h"

class Mesh : public ManagedResource, public IDrawGui {
public:


    Mesh(std::vector<Vertex3D>&& vertexList, std::vector<uint32_t >&& indexList, std::shared_ptr<Material> material);

    ~Mesh() override;

    [[nodiscard]] const std::vector<Vertex3D>& getVertices() const {return vertices_;}
    [[nodiscard]] const std::vector<uint32_t >& getIndices() const { return indices_; }
    [[nodiscard]] Transform& getTransform() { return transform_;}
    std::string getResourceType() const override { return "Mesh"; }
    [[nodiscard]] const vk::Buffer & getVertexBuffer() const { return vertexBuffer_.buffer; }
    [[nodiscard]] const vk::Buffer & getIndexBuffer() const { return indexBuffer_.buffer; }
    std::shared_ptr<Material> getMaterial() const {return material_;}

    bool drawGUI() override;

    void stage(const VkUtils::BufferAlloc& stagingBuffer) const;

    void recordDrawCommands(vk::raii::CommandBuffer& cmdBuf, const vk::raii::PipelineLayout& pipelineLayout) const;

    friend class MeshManager;
private:
    std::vector<Vertex3D> vertices_{};
    std::vector<uint32_t> indices_{};
    std::shared_ptr<Material> material_{nullptr};

    VkUtils::BufferAlloc vertexBuffer_{};
    VkUtils::BufferAlloc indexBuffer_{};

    Transform transform_{};

    void initBuffers();
};
