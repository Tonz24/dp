//
// Created by Tonz on 12.08.2025.
//

#pragma once
#include <glm/glm.hpp>

template <typename T>
class UBOFormat {
public:
    const T& getUBOFormat() { return uboFormat_;}

protected:
    T uboFormat_;
};

struct CameraUBOFormat {
    glm::mat4 matView{};
    glm::mat4 matProj{};
    glm::mat4 matViewProj{};
    glm::mat4 matInvViewProj{};

    glm::vec3 positionWorld{};
    float zNear{0.1f};

    float zFar{700.0f};
};

struct alignas(16) MaterialUBOFormat {
    glm::vec3 diffuseAlbedo{0};
    float shininess{32};

    glm::vec3 specularAlbedo{0};
    float ior{};

    glm::vec3 emission{0};
    uint32_t diffuseAlbedoMapHandle{0};

    glm::vec3 attenuation{0};
    uint32_t specularALbedoMapHandle{0};

    uint32_t shininessMapHandle{0};
    uint32_t normalMapHandle{0};
    float padding;
    float padding2;
};

static_assert(sizeof(MaterialUBOFormat) % 16 == 0, "MaterialUBOFormat must be 16B aligned (std140).");

struct PcsGBufferFill {
    glm::mat4 modelMat{};
    glm::mat4 normalMat{};
    uint32_t materialId{};
    uint32_t meshId{};
};

struct PcsGBufferShade {
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

struct PcsSky {
    uint32_t skyHandle;
};

struct PcsRaygen {
    uint32_t albedoMapHandle;
    uint32_t normalMapHandle;
    uint32_t depthMapHandle;
    uint32_t materialMapHandle;

    uint32_t skyHandle;

    uint32_t seed;
    uint32_t accumulate{0};
    uint32_t frameCtr{0};
};