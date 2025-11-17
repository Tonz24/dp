//
// Created by Tonz on 28.07.2025.
//

#pragma once

#include <array>
#include <memory>

#include <glm/glm.hpp>

#include "../engine/iDrawGui.h"
#include "texture.h"
#include "../engine/uboFormat.h"


class Material : public ManagedResource, public UBOFormat<MaterialUBOFormat>, public IDrawGui {
public:

    enum class TextureMapSlot : uint8_t {
        invalid = 255,
        albedo = 0,
        roughness = 1,
        metallic = 2,
        normal = 3,
    };

    enum class MaterialType : uint32_t {
        diffuse = 0,
        mirror = 1,
        pbr = 2,
    };

    Material() : ManagedResource(){}

    ~Material() override;


    std::shared_ptr<Texture> getTexture(TextureMapSlot slot);
    void setTexture(std::shared_ptr<Texture> texture, TextureMapSlot slot);

    [[nodiscard]] const glm::vec3& getAlbedo() const { return reinterpret_cast<const glm::vec3&>(uboFormat_.albedoRoughness); }
    [[nodiscard]] const glm::vec3& getEmission() const { return reinterpret_cast<const glm::vec3&>(uboFormat_.emissionMetallic); }
    [[nodiscard]] float getRoughness() const { return uboFormat_.albedoRoughness.w;}
    [[nodiscard]] float getMetallic() const { return uboFormat_.emissionMetallic.w; }
    [[nodiscard]] float getIor() const { return uboFormat_.attenuationIor.w; }
    [[nodiscard]] const glm::vec3 &getAttenuation() const { return reinterpret_cast<const glm::vec3&>(uboFormat_.attenuationIor); }
    MaterialType getMaterialType() const {return static_cast<MaterialType>(uboFormat_.materialType);}

    [[nodiscard]] std::string getResourceType() const override { return "Material"; }

    void setAlbedo(const glm::vec3 &diffuseAlbedo) {
        uboFormat_.albedoRoughness.x  = diffuseAlbedo.x;
        uboFormat_.albedoRoughness.y  = diffuseAlbedo.y;
        uboFormat_.albedoRoughness.z  = diffuseAlbedo.z;
    }
    void setEmission(const glm::vec3 &emission) {
        uboFormat_.emissionMetallic.x  = emission.x;
        uboFormat_.emissionMetallic.y  = emission.y;
        uboFormat_.emissionMetallic.z  = emission.z;
    }
    void setAttenuation(const glm::vec3 &attenuation) {
        uboFormat_.attenuationIor.x  = attenuation.x;
        uboFormat_.attenuationIor.y  = attenuation.y;
        uboFormat_.attenuationIor.z  = attenuation.z;
    }

    void setRoughness(float roughness) {uboFormat_.albedoRoughness.w = roughness; }
    void setMetallic(float metallic) { uboFormat_.emissionMetallic.w = metallic; }
    void setIor(float ior) { uboFormat_.attenuationIor.w = ior; }

    void updateUBO() const;
    void updateUBONow() const;

    bool drawGUI() override;

    bool isEmissive() const;


    friend class MaterialManager;
private:

    std::array<std::shared_ptr<Texture>,4> textures_{};
};




