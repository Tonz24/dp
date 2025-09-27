//
// Created by Tonz on 04.08.2025.
//

#include "scene.h"

#include "../engine/engine.h"
#include <imgui/imgui.h>

#include "../engine/renderers/renderer.h"


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
            if (selectedObject_->drawGUI())
                initTLAS();
        }

        if (camera_ != nullptr) {
            camera_->drawGUI();
        }

        ImGui::Unindent();
    }

    return false;
}

void Scene::initDescriptorSet() const {
    if (sky_) {
        VkUtils::BufferAlloc stagingBuffer = VkUtils::createBufferVMA(sky_->getTotalSize(),vk::BufferUsageFlagBits::eTransferSrc,VkUtils::stagingAllocFlagsVMA);
        sky_->stage(stagingBuffer);
        VkUtils::destroyBufferVMA(std::move(stagingBuffer));

        Renderer::registerTextureBindless(*sky_);
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
    float surfaceAreaTotal{0.0f};

    // accumulate total surface area from every emissive mesh, extract triangles
    for (const auto & mesh : meshes_) {
        if (mesh->getMaterial()->isEmissive()) {

            const auto& indices = mesh->getIndices();
            const auto& vertices = mesh->getVertices();

            const auto& modelMat = mesh->getTransform().getModelMat();

            for (uint32_t i = 0; i < indices.size(); i+=3) {

                glm::vec3 emission = mesh->getMaterial()->getEmission();

                //  world space position is of interest
                glm::vec3 v0Pos = modelMat * glm::vec4{vertices[indices[i]].position,1.0f};
                glm::vec3 v1Pos = modelMat * glm::vec4{vertices[indices[i+1]].position,1.0f};
                glm::vec3 v2Pos = modelMat * glm::vec4{vertices[indices[i+2]].position,1.0f};

                //  pack emission into fourth components of position vectors
                TrianglePacked tri{
                    .v0 = {v0Pos,emission.x},
                    .v1 = {v1Pos,emission.y},
                    .v2 = {v2Pos,emission.z},
                };

                glm::vec3 u = tri.v1 - tri.v0;
                glm::vec3 v = tri.v2 - tri.v0;

                tri.area = 0.5f * glm::length(glm::cross(u, v));
                surfaceAreaTotal += tri.area;

                emissiveTriangles_.emplace_back(tri);
            }
        }
    }
    vk::DeviceSize emissiveSize = emissiveTriangles_.size() * sizeof(emissiveTriangles_[0]) + sizeof(uint32_t) * 4;


    vk::BufferUsageFlags emissiveUsageFlags = vk::BufferUsageFlagBits::eStorageBuffer;
    VmaAllocationCreateFlags createFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    emissiveBuffer_ = VkUtils::createBufferVMA(emissiveSize,emissiveUsageFlags,createFlags);

    uint32_t triangleBufferCount = emissiveTriangles_.size();
    memcpy(emissiveBuffer_.allocationInfo.pMappedData,&triangleBufferCount,sizeof(triangleBufferCount));
    memcpy(static_cast<uint8_t*>(emissiveBuffer_.allocationInfo.pMappedData) + sizeof(uint32_t)*4,emissiveTriangles_.data(),emissiveTriangles_.size() * sizeof(emissiveTriangles_[0]));


    //normalize each triangle area, accumulate cdf
    for (uint32_t i = 0; i < emissiveTriangles_.size(); i++) {
        const auto& tri = emissiveTriangles_[i];


        float normArea = tri.area / surfaceAreaTotal;
        float predecessorVal = i == 0 ? 0 : cdf_[i - 1].pdf;

        cdf_.emplace_back(CDFElement{ i , predecessorVal + normArea });
    }

    std::ranges::sort(cdf_, [](auto &left, auto& right){
        return left.pdf < right.pdf;
    });

    vk::DeviceSize cdfSize = cdf_.size() * sizeof(cdf_[0]) + sizeof(uint32_t);
    cdfBuffer_ = VkUtils::createBufferVMA(cdfSize,emissiveUsageFlags,createFlags);


    uint32_t cdfBufferCount = cdf_.size();
    memcpy(cdfBuffer_.allocationInfo.pMappedData,&cdfBufferCount,sizeof(cdfBufferCount));
    memcpy(static_cast<uint8_t*>(cdfBuffer_.allocationInfo.pMappedData) + sizeof(uint32_t),cdf_.data(),cdf_.size() * sizeof(cdf_[0]));

    Renderer::updateEmissiveCDF(emissiveBuffer_,cdfBuffer_);
}