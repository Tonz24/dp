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
    uint32_t materialType{0};
    float padding2;
};