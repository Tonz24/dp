//
// Created by Tonz on 29.07.2025.
//

#pragma once
#include <vector>


#include "material.h"
#include "transform.h"
#include "vertex.h"
#include "../engine/iDrawGui.h"
#include "../engine/vk/accelerationStructure.h"
#include "../engine/vk/vkUtils.h"


class Mesh : public ManagedResource, public IDrawGui {
public:

    Mesh(std::vector<Vertex3D> &&vertexList, std::vector<uint32_t> &&indexList, std::shared_ptr<Material> material);

    ~Mesh() override;

    void updateBLASInstance();

    bool drawGUI() override;

    void stage(const VkUtils::BufferAlloc& stagingBuffer) const;
    void initBLAS();

    void recordDrawCommands(vk::raii::CommandBuffer& cmdBuf, const vk::raii::PipelineLayout& pipelineLayout) const;

    [[nodiscard]] const std::vector<Vertex3D>& getVertices() const {return vertices_;}
    [[nodiscard]] const std::vector<uint32_t >& getIndices() const { return indices_; }
    [[nodiscard]] Transform& getTransform() { return transform_;}
    [[nodiscard]] std::string getResourceType() const override { return "Mesh"; }
    [[nodiscard]] const vk::Buffer & getVertexBuffer() const { return vertexBuffer_.buffer; }
    [[nodiscard]] const vk::Buffer & getIndexBuffer() const { return indexBuffer_.buffer; }
    [[nodiscard]] std::shared_ptr<Material> getMaterial() const {return material_;}
    [[nodiscard]] const vk::AccelerationStructureInstanceKHR& getBLASInstance() const { return blasInstance_; }

    void updateDescription() const;

    friend class MeshManager;

    struct ObjDescription {
        vk::DeviceAddress vertexBufferAddress;
        vk::DeviceAddress indexBufferAddress;
        uint32_t materialId;
    };

    [[nodiscard]] const ObjDescription& getDescription() const { return description_; }

private:
    void initBuffers();


    std::vector<Vertex3D> vertices_{};
    std::vector<uint32_t> indices_{};
    std::shared_ptr<Material> material_{nullptr};

    VkUtils::BufferAlloc vertexBuffer_{};
    VkUtils::BufferAlloc indexBuffer_{};

    Transform transform_{};

    AccelerationStructure blas_{};

    vk::AccelerationStructureInstanceKHR blasInstance_{};

    ObjDescription description_{};
};
