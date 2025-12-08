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
    glm::vec4 albedoRoughness{0.0f,0.0f,0.0f,0.5f};
    glm::vec4 emissionMetallic{0.0f,0.0f,0.0f,0.0f};
    glm::vec4 attenuationIor{0.0f,0.0f,0.0f,1.46f};

    uint32_t albedoMapHandle{0};
    uint32_t roughnessMapHandle{0};
    uint32_t metallicMapHandle{0};
    uint32_t normalMapHandle{0};

    uint32_t materialType{2};
    float padding;
    float padding1;
    float padding2;
};

struct alignas(16) CandidateSample{
    // sample direction OR sample hit point
    // for any sample that is not an env map sample, the omega_i variable represents the hit point that the sample hit
    glm::vec3 omega_i;

    // 1.0 / pdf
    float W;

    // emission of hit surface
    glm::vec3 L_i;

    // mis weight for this sample
    // if the first bit is positive (the number is negative), the omega_i variable represents hit position
    float misWeight;

    // normal at sample hit point
    // is valid only when omega_i represents position
    glm::vec3 normal;
};

struct alignas(16) Reservoir{
    CandidateSample bestSample;
    float wSum;
    float W;
};