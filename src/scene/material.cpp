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
    if (slot == TextureMapSlot::invalidMapSlot)
        throw std::runtime_error("ERROR: trying to get texture at invalid slot offset!");

    return textures_[static_cast<uint8_t>(slot)];
}

void Material::setTexture(std::shared_ptr<Texture> texture, TextureMapSlot slot) {
    if (slot == TextureMapSlot::invalidMapSlot)
        throw std::runtime_error("ERROR: trying to set texture at invalid slot offset!");

    textures_[static_cast<uint8_t>(slot)] = std::move(texture);

    if (slot == TextureMapSlot::diffuseMapSlot)
        uboFormat_.diffuseAlbedoMapHandle = textures_[static_cast<uint8_t>(TextureMapSlot::diffuseMapSlot)]->getCID();
    else if (slot == TextureMapSlot::specularMapSlot)
        uboFormat_.specularALbedoMapHandle = textures_[static_cast<uint8_t>(TextureMapSlot::specularMapSlot)]->getCID();
    else if (slot == TextureMapSlot::normalMapSlot)
        uboFormat_.normalMapHandle = textures_[static_cast<uint8_t>(TextureMapSlot::normalMapSlot)]->getCID();
    else if (slot == TextureMapSlot::shininessMapSlot)
        uboFormat_.shininessMapHandle = textures_[static_cast<uint8_t>(TextureMapSlot::shininessMapSlot)]->getCID();
}

void Material::recordDescriptorSet() const {
    auto dummy = TextureManager::getInstance()->getResource("dummy");

    for (uint32_t i = 0; i < textures_.size(); ++i) {
        const auto& texture = textures_[i] ? textures_[i] : dummy;
        Renderer::registerTextureBindless(*texture);
    }
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
    bool changed{false}, changedUBO{false};
    if (ImGui::CollapsingHeader("Material")) {
        ImGui::Indent();

        static constexpr std::array materialType{"diffuse","mirror"};

        if (ImGui::Combo("Material type", reinterpret_cast<int*>(&uboFormat_.materialType), materialType.data(), materialType.size())) {
            changed = true;
            changedUBO = true;
        }
        changedUBO |= ImGui::ColorEdit3("Diffuse albedo",&uboFormat_.diffuseAlbedo[0]);
        changedUBO |= ImGui::ColorEdit3("Specular albedo",&uboFormat_.specularAlbedo[0]);
        changedUBO |= ImGui::DragFloat("Shininess",&uboFormat_.shininess,1,1.0f,10000.0f);
        changedUBO |= ImGui::DragFloat("Index of refraction",&uboFormat_.ior,0.01,1.0f,5.0f);
        if (ImGui::ColorEdit3("Emission",&uboFormat_.emission[0],ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float)) {
            changedUBO = true;
            changed = true;
        }
        changedUBO |= ImGui::ColorEdit3("Attenuation",&uboFormat_.attenuation[0]);
        ImGui::Unindent();
    }

    if (changedUBO)
        updateUBO();

    return changed;
}

bool Material::isEmissive() const {
    return glm::any(glm::notEqual(uboFormat_.emission,glm::vec3{0.0f}));
}