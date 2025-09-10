//
// Created by Tonz on 03.09.2025.
//

#include "renderer.h"

#include "../engine.h"

void Renderer::initLayouts() {
    if (!isDescSetLayoutInit_)
        initDescSetLayout();
}

void Renderer::destroy() {
    for (uint32_t i = 0; i < Constants::maxFramesInFlight; ++i) {
        VkUtils::destroyBufferVMA(std::move(cameraUBOs_[i]));
        VkUtils::destroyBufferVMA(std::move(materialUBOs_[i]));
    }
}

const vk::raii::DescriptorSet& Renderer::getDescSetFrame(uint32_t frameInFlightIndex) {

    if (frameInFlightIndex >= Constants::maxFramesInFlight)
        throw std::runtime_error("ERROR: invalid index!");

    return descSetsFrame_[frameInFlightIndex];
}

uint8_t* Renderer::getCamUBOsMapped(uint32_t frameInFlightIndex) {
    if (frameInFlightIndex >= Constants::maxFramesInFlight)
        throw std::runtime_error("ERROR: invalid index!");

    return cameraUBOsMapped_[frameInFlightIndex];
}

uint8_t* Renderer::getMatUBOsMapped(uint32_t frameInFlightIndex) {
    if (frameInFlightIndex >= Constants::maxFramesInFlight)
        throw std::runtime_error("ERROR: invalid index!");

    return materialUBOsMapped_[frameInFlightIndex];
}

void Renderer::updateTLASDescriptor(const vk::raii::AccelerationStructureKHR& tlas) {
    for (uint32_t i = 0; i < Constants::maxFramesInFlight; ++i) {
        vk::WriteDescriptorSetAccelerationStructureKHR tlasWriteInfo{
            .accelerationStructureCount = 1,
            .pAccelerationStructures = &*tlas
        };

        vk::WriteDescriptorSet descWrite{
            .pNext = &tlasWriteInfo,
            .dstSet = descSetsFrame_[i],
            .dstBinding = 3,
            .descriptorCount = 1,
            .descriptorType =  vk::DescriptorType::eAccelerationStructureKHR
        };
        VkUtils::getDevice().updateDescriptorSets(descWrite,{});
    }
}

void Renderer::initDescSetLayout() {

    //  Frame descriptor layout first
    descSetLayoutFrame_ = vk::raii::DescriptorSetLayout(VkUtils::getDevice(),frameLayoutInfo);

    //  Material descriptor layout second
    descSetLayoutMaterial_ = vk::raii::DescriptorSetLayout(VkUtils::getDevice(),materialLayoutInfo);

    // Sky descriptor layout last
    descSetLayoutSky_ = vk::raii::DescriptorSetLayout(VkUtils::getDevice(),skyLayoutInfo);

    // camera UBO
    for (uint32_t i = 0; i < Constants::maxFramesInFlight; ++i) {
        vk::DeviceSize bufferSize = sizeof(CameraUBOFormat);
        VmaAllocationCreateFlags allocationCreateFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT  | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        auto buffer = VkUtils::createBufferVMA(bufferSize,vk::BufferUsageFlagBits::eUniformBuffer, allocationCreateFlags);

        cameraUBOsMapped_.emplace_back(static_cast<unsigned char*>(buffer.allocationInfo.pMappedData));
        cameraUBOs_.emplace_back(std::move(buffer));

    }

    // material UBO
    for (uint32_t i = 0; i < Constants::maxFramesInFlight; ++i) {

        vk::DeviceSize bufferSize = sizeof(MaterialUBOFormat) * Constants::materialLimit;
        VmaAllocationCreateFlags allocationCreateFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        auto buffer = VkUtils::createBufferVMA(bufferSize,vk::BufferUsageFlagBits::eUniformBuffer, allocationCreateFlags);

        materialUBOsMapped_.emplace_back(static_cast<unsigned char*>(buffer.allocationInfo.pMappedData));
        materialUBOs_.emplace_back(std::move(buffer));
    }

    std::vector<vk::DescriptorSetLayout> layouts(Constants::maxFramesInFlight,*descSetLayoutFrame_);
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = Engine::getInstance().getDescriptorPool(),
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()
    };

    descSetsFrame_ = VkUtils::getDevice().allocateDescriptorSets(allocInfo);

    auto dummy = TextureManager::getInstance()->getResource("dummy");

    for (size_t i = 0; i < Constants::maxFramesInFlight; i++) {

        vk::DescriptorBufferInfo camBufferInfo{
            .buffer = cameraUBOs_[i].buffer,
            .offset = 0,
            .range = sizeof(CameraUBOFormat)
        };

        vk::WriteDescriptorSet writeDescriptorSetCam{
            .dstSet = descSetsFrame_[i], //  which descriptor set to update
            .dstBinding = 0, // which binding to update
            .dstArrayElement = 0, //  what element the update starts at
            .descriptorCount = 1, //  how many descriptors are affected
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &camBufferInfo,
        };

        vk::DescriptorImageInfo dummyInfo{
            .sampler =  dummy->getVkSampler(),
            .imageView = dummy->getVkImageView(),
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };

        vk::WriteDescriptorSet writeDescriptorSetDummy{
            .dstSet = descSetsFrame_[i], //  which descriptor set to update
            .dstBinding = 2, // which binding to update
            .dstArrayElement = 0, //  what element the update starts at
            .descriptorCount = 1, //  how many descriptors are affected
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &dummyInfo,
        };



        vk::DescriptorBufferInfo matBufferInfo{
            .buffer = materialUBOs_[i].buffer,
            .offset = 0,
            .range = sizeof(MaterialUBOFormat) * Constants::materialLimit
        };

        vk::WriteDescriptorSet writeDescriptorSetMat{
            .dstSet = descSetsFrame_[i], //  which descriptor set to update
            .dstBinding = 1, // which binding to update
            .dstArrayElement = 0, //  what element the update starts at
            .descriptorCount = 1, //  how many descriptors are affected
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &matBufferInfo,
        };

        VkUtils::getDevice().updateDescriptorSets({writeDescriptorSetCam, writeDescriptorSetDummy, writeDescriptorSetMat},{});
    }

    isDescSetLayoutInit_ = true;
}
