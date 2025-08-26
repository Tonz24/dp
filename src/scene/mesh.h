//
// Created by Tonz on 29.07.2025.
//

#pragma once
#include <vector>
#include <imgui/imgui.h>

#include "material.h"
#include "transform.h"
#include "../engine/iDrawGui.h"
#include "../engine/vk/vkUtils.h"
#include "../engine/concepts.h"

template <VertexType V>
class Mesh : public ManagedResource, public IDrawGui {
public:

    Mesh(std::vector<V> &&vertexList, std::vector<uint32_t> &&indexList, std::shared_ptr<Material> material):
    vertices_(std::move(vertexList)), indices_(std::move(indexList)), material_(std::move(material)) {

        initBuffers();
    }

    ~Mesh() override {
        VkUtils::destroyBufferVMA(vertexBuffer_);
        VkUtils::destroyBufferVMA(indexBuffer_);
    }

    bool drawGUI() override {
        if (ImGui::CollapsingHeader("Mesh")) {
            ImGui::Indent();
            transform_.drawGUI();
            material_->drawGUI();
            ImGui::Unindent();
        }

        return false;
    }

    void stage(const VkUtils::BufferAlloc& stagingBuffer) const {

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


    void recordDrawCommands(vk::raii::CommandBuffer& cmdBuf, const vk::raii::PipelineLayout& pipelineLayout) const {
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


    [[nodiscard]] const std::vector<V>& getVertices() const {return vertices_;}
    [[nodiscard]] const std::vector<uint32_t >& getIndices() const { return indices_; }
    [[nodiscard]] Transform& getTransform() { return transform_;}
    std::string getResourceType() const override { return "Mesh"; }
    [[nodiscard]] const vk::Buffer & getVertexBuffer() const { return vertexBuffer_.buffer; }
    [[nodiscard]] const vk::Buffer & getIndexBuffer() const { return indexBuffer_.buffer; }
    std::shared_ptr<Material> getMaterial() const {return material_;}




    friend class MeshManager;
private:
    void initBuffers() {
        vk::DeviceSize vertexBufferSize = sizeof(vertices_[0]) * vertices_.size();
        vertexBuffer_ = VkUtils::createBufferVMA(vertexBufferSize,vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst);

        vk::DeviceSize indexBufferSize = sizeof(indices_[0]) * indices_.size();
        indexBuffer_ = VkUtils::createBufferVMA(indexBufferSize,vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst);
    }

    std::vector<V> vertices_{};
    std::vector<uint32_t> indices_{};
    std::shared_ptr<Material> material_{nullptr};

    VkUtils::BufferAlloc vertexBuffer_{};
    VkUtils::BufferAlloc indexBuffer_{};

    Transform transform_{};

};
