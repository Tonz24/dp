//
// Created by Tonz on 04.08.2025.
//

#include "scene.h"

#include "../engine/engine.h"
#include <imgui/imgui.h>

#include "../engine/managers/resourceManager.h"


Scene::Scene(const std::vector<std::shared_ptr<Mesh>>&& meshes, std::shared_ptr<Camera> camera, std::shared_ptr<Texture> sky): meshes_(meshes), camera_(std::move(camera)), sky_(std::move(sky)) {

    initDescriptorSet();
    initTLAS();
    extractEmissiveMeshes();

    if (!meshes_.empty())
        selectedObject_ = meshes_[0];
}

bool Scene::drawGUI() {

    if (ImGui::CollapsingHeader("Scene")) {
        ImGui::Indent();
        ImGui::Text("Selected mesh: ");
        ImGui::SameLine();

        if (selectedObject_ != nullptr) {
            ImGui::Text(selectedObject_->getResourceName().c_str());

            // rebuild the TLAS
            // TODO: just update the instances, do not rebuild the TLAS vk object itself
            if (selectedObject_->drawGUI()) {
                initTLAS();
                extractEmissiveMeshes();
            }
        }

        if (camera_ != nullptr) {
            camera_->drawGUI();
        }

        ImGui::Unindent();
    }

    return false;
}

void Scene::initDescriptorSet(){
    if (sky_) {
        VkUtils::BufferAlloc stagingBuffer = VkUtils::createBufferVMA(sky_->getTotalSize(),vk::BufferUsageFlagBits::eTransferSrc,VkUtils::stagingAllocFlagsVMA);
        sky_->stage(stagingBuffer);
        VkUtils::destroyBufferVMA(std::move(stagingBuffer));

        skyCdf_ = sky_->getCdf();
    }
}

void Scene::initTLAS() {

    std::vector<vk::AccelerationStructureInstanceKHR> instances{};
    instances.reserve(meshes_.size());

    for (const auto & mesh : meshes_)
        instances.emplace_back(mesh->getBLASInstance());

    vk::DeviceSize instanceBufferSize = meshes_.size() * sizeof(vk::AccelerationStructureInstanceKHR);

    //  gather mesh BLAS instance infos into a staging buffer
    VkUtils::BufferAlloc stagingBuffer = VkUtils::createBufferVMA(instanceBufferSize,vk::BufferUsageFlagBits::eTransferSrc,VkUtils::stagingAllocFlagsVMA);
    VkUtils::BufferAlloc instanceBuffer = VkUtils::createBufferVMA(instanceBufferSize,vk::BufferUsageFlagBits::eTransferDst | VkUtils::accelStructInputFlags);
    memcpy(stagingBuffer.allocationInfo.pMappedData,instances.data(),instanceBufferSize);

    //  Copy the staging buffer into the TLAS input buffer
    VkUtils::copyBuffer(stagingBuffer,instanceBuffer,instanceBufferSize);
    VkUtils::destroyBufferVMA(std::move(stagingBuffer));


    vk::AccelerationStructureGeometryInstancesDataKHR instanceData;
    instanceData.setArrayOfPointers(false);
    instanceData.setData(instanceBuffer.deviceAddress);

    vk::AccelerationStructureGeometryKHR instanceGeometry;
    instanceGeometry.setGeometryType(vk::GeometryTypeKHR::eInstances);
    instanceGeometry.setGeometry({instanceData});
    instanceGeometry.setFlags(vk::GeometryFlagBitsKHR::eOpaque);

    tlas_ = AccelerationStructure(vk::AccelerationStructureTypeKHR::eTopLevel,instanceGeometry,instances.size());
    VkUtils::destroyBufferVMA(std::move(instanceBuffer));

    Renderer::updateTLASDescriptor(tlas_.getAccelStructure());
}

void Scene::extractEmissiveMeshes() {
    // include size data by default
    vk::DeviceSize emissiveSize = sizeof(uint32_t) * 4;
    // accumulate later, serves as the CDF denominator
    float surfaceAreaTotal{0.0f};
    // accumulate later, serves as the first field in the triangle buffer
    uint32_t triangleCount{0};

    // accumulate total surface area from every emissive mesh, figure out the emissive triangle buffer size
    for (const auto & mesh : meshes_) {
        emissiveSize += mesh->getEmissiveTriangles().size() * sizeof(TrianglePacked);
        triangleCount += mesh->getEmissiveTriangles().size();
        surfaceAreaTotal += mesh->getEmissiveSurfaceArea();
    }

    vk::BufferUsageFlags emissiveUsageFlags = vk::BufferUsageFlagBits::eStorageBuffer;
    VmaAllocationCreateFlags createFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkUtils::destroyBufferVMA(std::move(emissiveBuffer_));
    emissiveBuffer_ = VkUtils::createBufferVMA(emissiveSize,emissiveUsageFlags,createFlags);

    memcpy(emissiveBuffer_.allocationInfo.pMappedData,&triangleCount,sizeof(triangleCount));

    uint32_t offset{0};
    for (const auto & mesh : meshes_) {
        auto emissiveTriangles = mesh->getEmissiveTriangles();
        auto trianglesSize = emissiveTriangles.size()  * sizeof(TrianglePacked);
        memcpy(static_cast<uint8_t*>(emissiveBuffer_.allocationInfo.pMappedData) + sizeof(uint32_t)*4 + offset,emissiveTriangles.data(),trianglesSize);
        offset += trianglesSize;
    }

    //normalize each triangle area, accumulate cdf
    uint32_t cdfIndex{0};
    cdf_.clear();
    for (const auto & mesh: meshes_) {
        const auto& triangles = mesh->getEmissiveTriangles();

        for (const auto & tri: triangles) {
            float normArea = tri.area / surfaceAreaTotal;
            float predecessorVal = cdfIndex == 0 ? 0 : cdf_[cdfIndex - 1].pdf;

            cdf_.emplace_back(CDFElement{ cdfIndex , predecessorVal + normArea });
            cdfIndex++;
        }
    }

    std::ranges::sort(cdf_, [](auto &left, auto& right){
        return left.pdf < right.pdf;
    });

    vk::DeviceSize cdfSize = cdf_.size() * sizeof(CDFElement) + sizeof(uint32_t) * 2;

    VkUtils::destroyBufferVMA(std::move(cdfBuffer_));
    cdfBuffer_ = VkUtils::createBufferVMA(cdfSize,emissiveUsageFlags,createFlags);

    uint32_t cdfBufferCount = cdf_.size();
    memcpy(cdfBuffer_.allocationInfo.pMappedData,&cdfBufferCount,sizeof(cdfBufferCount));
    memcpy(static_cast<uint8_t*>(cdfBuffer_.allocationInfo.pMappedData) + sizeof(surfaceAreaTotal),&surfaceAreaTotal,sizeof(surfaceAreaTotal));
    memcpy(static_cast<uint8_t*>(cdfBuffer_.allocationInfo.pMappedData) + sizeof(uint32_t) * 2,cdf_.data(),cdf_.size() * sizeof(cdf_[0]));

    Renderer::updateEmissiveCDF(emissiveBuffer_,cdfBuffer_);
}

void Scene::setSelectedObject(uint32_t objectCId) {
    selectedObject_ = MeshManager::getInstance()->getResource(objectCId);
}