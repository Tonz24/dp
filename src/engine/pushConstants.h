//
// Created by Tonz on 24.10.2025.
//

#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include "pushConstants.h"



struct PcsGBufferFill{
    struct Data{
        glm::mat4 modelMat{};
        glm::mat4 normalMat{};
        uint32_t materialId{};
        uint32_t meshId{};
    };
    Data data;

    static constexpr vk::ShaderStageFlags stageFlags{vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment};
    static constexpr vk::PushConstantRange getRange() {
        return {
            .stageFlags = stageFlags,
            .offset = 0,
            .size = sizeof(data)
        };
    }
};

struct PcsGBufferShade{
    struct Data{
        glm::vec3 lightPosWS{1.0f};
        int overlayIndex{0};

        glm::vec3 lightEmission{3.0f};
        int drawSkybox{true};

        int remapNormals{true};
        uint32_t albedoMapHandle;
        uint32_t normalMapHandle;
        uint32_t depthMapHandle;

        uint32_t materialMapHandle;
    };
    Data data;

    static constexpr vk::ShaderStageFlags stageFlags{vk::ShaderStageFlagBits::eFragment};
    static constexpr vk::PushConstantRange getRange() {
        return {
            .stageFlags = stageFlags,
            .offset = 0,
            .size = sizeof(data)
        };
    }
};

struct PcsSky{
    struct Data{
        uint32_t skyHandle;
    };
    Data data;

    static constexpr vk::ShaderStageFlags stageFlags{vk::ShaderStageFlagBits::eFragment};
    static constexpr vk::PushConstantRange getRange() {
        return {
            .stageFlags = stageFlags,
            .offset = 0,
            .size = sizeof(data)
        };
    }
};

struct PcsRtTonemap{
    struct Data{
        uint32_t accumulatorHandle;
        uint32_t normalTexIndex;
        uint32_t doTonemap;
    };
    Data data;


    static constexpr vk::ShaderStageFlags stageFlags{vk::ShaderStageFlagBits::eFragment};
    static constexpr vk::PushConstantRange getRange() {
        return {
            .stageFlags = stageFlags,
            .offset = 0,
            .size = sizeof(data)
        };
    }
};


struct PcsRaygen{
    struct Data{
        uint32_t albedoMapHandle;
        uint32_t normalMapHandle;
        uint32_t depthMapHandle;
        uint32_t materialMapHandle;

        uint32_t targetHandle;
        uint32_t skyHandle;
        uint32_t skyCdfHandle;

        uint32_t seed;
        uint32_t accumulate{0};
        uint32_t frameCtr{0};

        uint32_t maxRecursionDepth;

        uint32_t doRIS{0};
        uint32_t M_brdf{1};
        uint32_t M_area{1};
        uint32_t M_env{1};
    };
    Data data;

    static constexpr vk::ShaderStageFlags stageFlags{vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eMissKHR | vk::ShaderStageFlagBits::eClosestHitKHR};
    static constexpr vk::PushConstantRange getRange() {
        return {
            .stageFlags = stageFlags,
            .offset = 0,
            .size = sizeof(data)
        };
    }
};