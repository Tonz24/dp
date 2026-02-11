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

        uint32_t seed{};
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
        uint32_t motionMapHandle;
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


         /**
         * RIS information packed into 32 bits \n
         * MSB (1 << 31) -- do ris? \n
         * first 6 bits (63 << 0) -- brdf sample count [0, 63] \n
         * next 6 bits (63 << 6)  -- area sample count [0, 63] \n
         * next 6 bits (63 << 12) -- env sample count [0, 63] \n
         * next 6 bits (63 << 18) -- neighbor count [0, 63] \n
         * next bit    (63 << 24) -- reservoir buffer read/write index  [0 or 1] \n
         * 63 dec = 0x3F
         */
        uint32_t ris{0};
        float neighborSearchRadius{80.0f};
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

    static constexpr uint32_t risShift = 31;
    static constexpr uint32_t maxSampleCount = 63;

    static constexpr uint32_t brdfShift = 0 * 6;
    static constexpr uint32_t areaShift = 1 * 6;
    static constexpr uint32_t envShift =  2 * 6;
    static constexpr uint32_t neighborCountShift =  3 * 6;
    static constexpr uint32_t bufferIndexShift =  4 * 6;

    static constexpr uint32_t brdfMask = 0x3F << brdfShift;
    static constexpr uint32_t areaMask = 0x3F << areaShift;
    static constexpr uint32_t envMask = 0x3F << envShift;
    static constexpr uint32_t bufferIndexMask = 0x01 << bufferIndexShift;
    static constexpr uint32_t neighborMask = 0x3F << neighborCountShift;


    struct UnpackedData {
        uint32_t doRIS{0};
        uint32_t M_brdf{1};
        uint32_t M_area{1};
        uint32_t M_env{1};

        uint32_t doSpatialReuse{0};
        // spatial reuse neighbor count
        uint32_t M_neighbor{0};
        uint32_t neighborSearchRadius{0};

        bool bufferIndices{false};
    };

    static void packData(const UnpackedData& unpacked, Data& data) {
        data.ris = 0;

        data.ris |= unpacked.doRIS << risShift;
        data.ris |= unpacked.M_brdf << brdfShift;
        data.ris |= unpacked.M_area << areaShift;
        data.ris |= unpacked.M_env << envShift;
        data.ris |= unpacked.M_neighbor << neighborCountShift;
        data.ris |= static_cast<uint32_t>(unpacked.bufferIndices) << bufferIndexShift;
    }
};