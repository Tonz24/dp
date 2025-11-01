//
// Created by Tonz on 03.09.2025.
//

#include "renderer.h"

#include "../engine.h"
#include "../managers/resourceManager.h"

Renderer::~Renderer() {
    VkUtils::destroyBufferVMA(std::move(exportBuffer_));
}

void Renderer::setExportSignal(std::string_view fileName) {
    exportSignal_ = true;
    exportFileName_ = fileName;
}

void Renderer::initLayouts() {
    if (!isDescSetLayoutInit_)
        initDescSetLayout();
}

void Renderer::destroy() {
    for (uint32_t i = 0; i < Constants::maxFramesInFlight; ++i) {
        VkUtils::destroyBufferVMA(std::move(cameraUBOs_[i]));
        VkUtils::destroyBufferVMA(std::move(objDescSSBOs_[i]));
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

void Renderer::updateEmissiveCDF(const VkUtils::BufferAlloc& trianglesBuffer, const VkUtils::BufferAlloc& cdfBuffer) {
    for (uint32_t i = 0; i < Constants::maxFramesInFlight; ++i) {
        vk::DescriptorBufferInfo triBufferInfo{
            .buffer = trianglesBuffer.buffer,
            .offset =  0,
            .range =  vk::WholeSize
        };

        vk::DescriptorBufferInfo cdfBufferInfo{
            .buffer = cdfBuffer.buffer,
            .offset =  0,
            .range =  vk::WholeSize
        };

        vk::WriteDescriptorSet triBufferWrite{
            .dstSet = getDescSetFrame(i),
            .dstBinding = 7,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &triBufferInfo
        };

        vk::WriteDescriptorSet cdfBufferWrite{
            .dstSet = getDescSetFrame(i),
            .dstBinding = 8,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &cdfBufferInfo
        };
        VkUtils::getDevice().updateDescriptorSets({triBufferWrite,cdfBufferWrite},{});
    }
}

void Renderer::initDescSetLayout() {

    //  Frame descriptor layout first
    descSetLayoutFrame_ = vk::raii::DescriptorSetLayout(VkUtils::getDevice(),frameLayoutInfo);

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

    // objDesc SSBO
    for (uint32_t i = 0; i < Constants::maxFramesInFlight; ++i) {

        vk::DeviceSize bufferSize = sizeof(Mesh::ObjDescription) * Constants::objDescLimit;
        VmaAllocationCreateFlags allocationCreateFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT ;
        auto buffer = VkUtils::createBufferVMA(bufferSize,vk::BufferUsageFlagBits::eStorageBuffer, allocationCreateFlags);

        objDescSSBOsMapped_.emplace_back(static_cast<unsigned char*>(buffer.allocationInfo.pMappedData));
        objDescSSBOs_.emplace_back(std::move(buffer));
    }

    std::vector<vk::DescriptorSetLayout> layouts(Constants::maxFramesInFlight,*descSetLayoutFrame_);
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = Engine::getInstance().getDescriptorPool(),
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()
    };

    descSetsFrame_ = VkUtils::getDevice().allocateDescriptorSets(allocInfo);

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

        vk::DescriptorBufferInfo objDescBufferInfo{
            .buffer = objDescSSBOs_[i].buffer,
            .offset = 0,
            .range = sizeof(Mesh::ObjDescription) * Constants::objDescLimit
        };

        vk::WriteDescriptorSet writeDescriptorSetObjDesc{
            .dstSet = descSetsFrame_[i], //  which descriptor set to update
            .dstBinding = 5, // which binding to update
            .dstArrayElement = 0, //  what element the update starts at
            .descriptorCount = 1, //  how many descriptors are affected
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &objDescBufferInfo,
        };

        VkUtils::getDevice().updateDescriptorSets({writeDescriptorSetCam, /*writeDescriptorSetDummy,*/ writeDescriptorSetMat, writeDescriptorSetObjDesc},{});
    }

    isDescSetLayoutInit_ = true;
}


void Renderer::registerTextureBindless(const Texture& texture) {
    for (uint32_t i = 0; i < Constants::maxFramesInFlight; ++i) {

        // TODO: handle other unsigned formats
        uint32_t dstBinding = texture.getVkFormat() == vk::Format::eR32Uint ? 6 : 2;

        vk::DescriptorImageInfo imageInfo{
            .sampler = texture.getVkSampler(),
            .imageView = texture.getVkImageView(),
            .imageLayout = texture.getSamplerLayout()
        };

        vk::WriteDescriptorSet writeDescriptorSetBindless{
            .dstSet = getDescSetFrame(i),
            .dstBinding = dstBinding,
            .dstArrayElement = texture.getCID(),
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &imageInfo
        };
        VkUtils::getDevice().updateDescriptorSets(writeDescriptorSetBindless,{});
    }
}

void Renderer::registerTextureStorage(const Texture& texture) {
    // TODO: move to non-uniform indexing

    for (uint32_t i = 0; i < Constants::maxFramesInFlight; ++i) {
        vk::DescriptorImageInfo imageInfo{
            .sampler = texture.getVkSampler(),
            .imageView = texture.getVkImageView(),
            .imageLayout = vk::ImageLayout::eGeneral
        };

        vk::WriteDescriptorSet writeDescriptorSetBindless{
            .dstSet = getDescSetFrame(i),
            .dstBinding = 4,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageImage,
            .pImageInfo = &imageInfo
        };
        VkUtils::getDevice().updateDescriptorSets(writeDescriptorSetBindless,{});
    }
}

void Renderer::uploadObjDescription(const Mesh& mesh) {
    for (uint32_t i = 0; i < Constants::maxFramesInFlight; ++i) {
        uint8_t* dst = objDescSSBOsMapped_[i] + mesh.getCID() * sizeof(Mesh::ObjDescription);
        memcpy(dst,&mesh.getDescription(), sizeof(Mesh::ObjDescription));
    }
}

void Renderer::flushExportBuffer() {

    if (!pendingExport_)
        return;

    pendingExport_ = false;
    uint32_t bufferSize = exportExtent_.width * exportExtent_.height * 4;

    std::vector<uint8_t> imageData(bufferSize);
    imageData.resize(bufferSize);

    memcpy(imageData.data(),exportBuffer_.allocationInfo.pMappedData,bufferSize);

    uint32_t exportScanWidth = exportExtent_.width * 4;

    FIBITMAP *image = FreeImage_ConvertFromRawBits(imageData.data(),
                                                exportExtent_.width,
                                                exportExtent_.height,
                                                exportScanWidth,
                                                4 * 8,
                                                FI_RGBA_RED_MASK,
                                                FI_RGBA_GREEN_MASK,
                                                FI_RGBA_BLUE_MASK,
                                                true);

    std::string fullPath {Paths::exportPrefix};
    fullPath.append(exportFileName_.size() > 0 ? exportFileName_ : "untitled.png");

    FreeImage_Save(FIF_PNG, image, fullPath.data(), PNG_DEFAULT);
    FreeImage_Unload(image);

    VkUtils::destroyBufferVMA(std::move(exportBuffer_));
}

void Renderer::recordSwapchainImageExport(const vk::Image& swapchainImage, vk::Extent2D extent, std::string_view fileName,
                                          const vk::raii::CommandBuffer& cmdBuf) {
    exportExtent_ = extent;
    vk::DeviceSize bufferSize = extent.width * extent.height * 4;
    exportBuffer_ = VkUtils::createBufferVMA(bufferSize,vk::BufferUsageFlagBits::eTransferDst,VkUtils::stagingAllocFlagsVMA);

    VkUtils::transitionImageLayout(swapchainImage,
                                  vk::ImageLayout::eTransferDstOptimal,
                                  vk::ImageLayout::eTransferSrcOptimal,
                                  vk::PipelineStageFlagBits2::eTransfer,
                                   vk::AccessFlagBits2::eTransferWrite,
                                   vk::PipelineStageFlagBits2::eTransfer,
                                   vk::AccessFlagBits2::eTransferRead,
                                  vk::ImageAspectFlagBits::eColor,
                                  cmdBuf);

    VkUtils::copyImageToBuffer(swapchainImage, exportBuffer_,0,extent.width,0,extent.height,cmdBuf);

    VkUtils::transitionImageLayout(swapchainImage,
                        vk::ImageLayout::eTransferSrcOptimal,
                                 vk::ImageLayout::eTransferDstOptimal,
                                 vk::PipelineStageFlagBits2::eTransfer,
                                  vk::AccessFlagBits2::eTransferRead,
                                  vk::PipelineStageFlagBits2::eTransfer,
                                  vk::AccessFlagBits2::eTransferWrite,
                                 vk::ImageAspectFlagBits::eColor,
                                 cmdBuf);

    pendingExport_ = true;
}
