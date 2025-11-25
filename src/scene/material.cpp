//
// Created by Tonz on 28.07.2025.
//

#include "material.h"

#include <imgui/imgui.h>
#include <utility>
#include <iostream>

#include "../engine/engine.h"
#include "../engine/managers/resourceManager.h"

std::shared_ptr<Texture> Material::getTexture(TextureMapSlot slot) {
    if (slot == TextureMapSlot::invalid)
        throw std::runtime_error("ERROR: trying to get texture at invalid slot offset!");

    return textures_[static_cast<uint8_t>(slot)];
}

void Material::setTexture(std::shared_ptr<Texture> texture, TextureMapSlot slot) {
    if (slot == TextureMapSlot::invalid)
        throw std::runtime_error("ERROR: trying to set texture at invalid slot offset!");

    textures_[static_cast<uint8_t>(slot)] = std::move(texture);

    if (slot == TextureMapSlot::albedo)
        uboFormat_.albedoMapHandle = textures_[static_cast<uint8_t>(TextureMapSlot::albedo)]->getCID();
    else if (slot == TextureMapSlot::roughness)
        uboFormat_.roughnessMapHandle = textures_[static_cast<uint8_t>(TextureMapSlot::roughness)]->getCID();
    else if (slot == TextureMapSlot::normal)
        uboFormat_.normalMapHandle = textures_[static_cast<uint8_t>(TextureMapSlot::normal)]->getCID();
    else if (slot == TextureMapSlot::metallic)
        uboFormat_.metallicMapHandle = textures_[static_cast<uint8_t>(TextureMapSlot::metallic)]->getCID();
}

void Material::updateUBO() const {
    Engine::getInstance().setMaterialUBOStorage(getCID(), uboFormat_);
}

void Material::updateUBONow() const {
    for (const auto & materialUBO : Renderer::getMatUBOsMapped()) {
        uint8_t* dst = materialUBO + getCID() * sizeof(uboFormat_);
        memcpy(dst,&uboFormat_,sizeof(uboFormat_));
    }
}

bool Material::drawGUI() {

    // changed signals that changes to material variables should be propagated outside (to mesh for BLAS instance recreation)
    // changedUBO tracks whether the material UBO needs to be changed (from this function)
    bool changed{false};
    if (ImGui::CollapsingHeader("Material")) {
        ImGui::Indent();

        static constexpr std::array materialType{"diffuse","mirror","pbr"};

        changed |= ImGui::Combo("Material type", reinterpret_cast<int*>(&uboFormat_.materialType), materialType.data(), materialType.size());

        changed |= ImGui::ColorEdit3("Albedo",&uboFormat_.albedoRoughness[0]);
        changed |= ImGui::DragFloat("Roughness",&uboFormat_.albedoRoughness[3],0.001f,0.05f,1.0f);
        changed |= ImGui::DragFloat("Metallic",&uboFormat_.emissionMetallic[3],0.001f,0.0f,1.0f);
        changed |= ImGui::DragFloat("Index of refraction",&uboFormat_.attenuationIor[3],0.01,1.0f,5.0f);
        changed |= ImGui::DragFloat3("Transmission attenuation",&uboFormat_.attenuationIor[0],0.01,0.0f,9999.0f);

        changed |= ImGui::ColorEdit3("Emission",&uboFormat_.emissionMetallic[0],ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);

        ImGui::Unindent();
    }

    if (changed)
        updateUBO();

    return changed;
}

bool Material::isEmissive() const {
    return glm::any(glm::notEqual(glm::vec3{uboFormat_.emissionMetallic},glm::vec3{0.0f}));
}

Material::~Material() {
    for (auto texture : textures_)
        texture.reset();
}