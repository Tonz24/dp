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
        invalidMapSlot = 255,
        diffuseMapSlot = 0,
        specularMapSlot = 1,
        normalMapSlot = 2,
        shininessMapSlot = 3,
    };

    Material() : ManagedResource(){
        allocateDescriptorSet();
    }


    std::shared_ptr<Texture> getTexture(TextureMapSlot slot);

    void setTexture(std::shared_ptr<Texture> texture, TextureMapSlot slot);

    [[nodiscard]] const glm::vec3 &getDiffuseAlbedo() const { return uboFormat_.diffuseAlbedo; }


    [[nodiscard]] const glm::vec3 &getSpecularAlbedo() const { return uboFormat_.specularAlbedo; }

    [[nodiscard]] const glm::vec3 &getEmission() const { return uboFormat_.emission; }

    // float getRoughness() const {
    //     return roughness_;
    // }
    //
    // float getMetallic() const {
    //     return metallic_;
    // }

    void setDiffuseAlbedo(const glm::vec3 &diffuseAlbedo) { uboFormat_.diffuseAlbedo = diffuseAlbedo; }

    void setSpecularAlbedo(const glm::vec3 &specularAlbedo) { uboFormat_.specularAlbedo = specularAlbedo; }

    void setEmission(const glm::vec3 &emission) { uboFormat_.emission = emission;}

    // void setRoughness(float roughness) {
    //     roughness_ = roughness;
    // }
    //
    // void setMetallic(float metallic) {
    //     metallic_ = metallic;
    // }

    float getShininess() const { return uboFormat_.shininess; }

    float getIor() const { return uboFormat_.ior; }

    const glm::vec3 &getAttenuation() const { return uboFormat_.attenuation; }

    void setShininess(float shininess) { uboFormat_.shininess = shininess; }

    void setIor(float ior) { uboFormat_.ior = ior; }

    void setAttenuation(const glm::vec3 &attenuation) { uboFormat_.attenuation = attenuation; }

    std::string getResourceType() const override { return "Material"; }

    void recordDescriptorSet() const;
    const vk::raii::DescriptorSet& getDescriptorSet() const {return descriptorSet_;}

    void updateUBO() const;
    void updateUBONow() const;

    bool drawGUI() override;


    friend class MaterialManager;
private:

    void allocateDescriptorSet();
    // glm::vec3 diffuseAlbedo_{};
    // glm::vec3 specularAlbedo_{};
    // glm::vec3 emission_{};
    //
    // float shininess_{};
    // float ior_{};
    // glm::vec3 attenuation_{};
    //
    //
    // //unused
    // float roughness_{};
    // float metallic_{};


    // [0 - diffuse albedo map
    //  1 - specular albedo map
    //  2 - normal map
    //  3 - shininnes map]
    std::array<std::shared_ptr<Texture>,4> textures_{};

    vk::raii::DescriptorSet descriptorSet_{nullptr};
};




