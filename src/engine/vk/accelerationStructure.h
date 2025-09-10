//
// Created by Tonz on 10.09.2025.
//

#pragma once
#include "vkUtils.h"


class AccelerationStructure {
public:
    AccelerationStructure() = default;
    AccelerationStructure(vk::AccelerationStructureTypeKHR type,const vk::AccelerationStructureGeometryKHR& geometry, uint32_t primitiveCount);

    ~AccelerationStructure();

    AccelerationStructure(const AccelerationStructure& other) = delete;
    AccelerationStructure& operator=(const AccelerationStructure& other) = delete;

    AccelerationStructure(AccelerationStructure&& other) noexcept
        : accelStruct_(std::move(other.accelStruct_)),
          storageBuffer_(std::move(other.storageBuffer_)) {}

    AccelerationStructure& operator=(AccelerationStructure&& other) noexcept {
        if (this == &other) return *this;
        accelStruct_ = std::move(other.accelStruct_);
        storageBuffer_ = std::move(other.storageBuffer_);
        return *this;
    }

    [[nodiscard]] const VkUtils::BufferAlloc& getStorageBuffer() const {return storageBuffer_;}

private:
    vk::raii::AccelerationStructureKHR accelStruct_{nullptr};
    VkUtils::BufferAlloc storageBuffer_{};
};
