//
// Created by Tonz on 12.08.2025.
//

#pragma once

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
    alignas(16) glm::vec3 positionWorld{};
};

struct MaterialUBOFormat {
    glm::vec3 diffuseAlbedo{};
    float shininess{};

    glm::vec3 specularAlbedo{};
    float ior{};

    glm::vec3 emission{};
    uint32_t diffuseAlbedoMapHandle{0};

    glm::vec3 attenuation;
    uint32_t specularALbedoMapHandle{0};

    uint32_t shininessMapHandle{0};
    uint32_t normalMapHandle{0};
    float padding;
    float padding2;
};