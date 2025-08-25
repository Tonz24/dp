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
        uboFormat_.diffuseAlbedoMapHandle = 1;
    else if (slot == TextureMapSlot::specularMapSlot)
        uboFormat_.specularALbedoMapHandle = 1;
    else if (slot == TextureMapSlot::normalMapSlot)
        uboFormat_.normalMapHandle = 1;
    else if (slot == TextureMapSlot::shininessMapSlot)
        uboFormat_.shininessMapHandle = 1;
}

void Material::recordDescriptorSet() const {

    std::vector<vk::WriteDescriptorSet> descriptorWrites{};
    std::vector<vk::DescriptorImageInfo> imageInfos;
    imageInfos.reserve(textures_.size());
    descriptorWrites.reserve(textures_.size());

    auto dummy = TextureManager::getInstance()->getResource("dummy");

    for (uint32_t i = 0; i < textures_.size(); ++i) {

        imageInfos.emplace_back(vk::DescriptorImageInfo{
            .sampler = textures_[i] ? textures_[i]->getVkSampler() : dummy->getVkSampler(),
            .imageView = textures_[i] ? textures_[i]->getVkImageView() : dummy->getVkImageView(),
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        });

        vk::WriteDescriptorSet writeDescriptorSet{
            .dstSet = descriptorSet_,
            .dstBinding = i,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &imageInfos.back()
        };

        descriptorWrites.emplace_back(writeDescriptorSet);
    }

    Engine::getInstance().getDevice().updateDescriptorSets(descriptorWrites,{});
}

void Material::updateUBO() const {
    Engine::getInstance().setMaterialUBOStorage(getCID(), uboFormat_);
}

void Material::updateUBONow() const {
    for (const auto & materialUBO : Engine::getInstance().getMaterialUBOs()) {
        uint8_t* dst = materialUBO + getCID() * sizeof(uboFormat_);
        memcpy(dst,&uboFormat_,sizeof(uboFormat_));
    }
}

bool Material::drawGUI() {

    bool changed{false};
    if (ImGui::CollapsingHeader("Material")) {
        ImGui::Indent();
        changed |= ImGui::ColorEdit3("Diffuse albedo",&uboFormat_.diffuseAlbedo[0]);
        changed |= ImGui::ColorEdit3("Specular albedo",&uboFormat_.specularAlbedo[0]);
        changed |= ImGui::DragFloat("Shininess",&uboFormat_.shininess,1,0.0f,10000.0f);
        changed |= ImGui::DragFloat("Index of refraction",&uboFormat_.ior,0.01,1.0f,5.0f);
        changed |= ImGui::ColorEdit3("Emission",&uboFormat_.emission[0]);
        changed |= ImGui::ColorEdit3("Attenuation",&uboFormat_.attenuation[0]);
        ImGui::Unindent();
    }

    if (changed)
        updateUBO();

    return changed;
}

void Material::allocateDescriptorSet() {
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = Engine::getInstance().getDescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &*Engine::getInstance().getDescriptorSetLayoutMaterial(),
    };

    auto h = Engine::getInstance().getDevice().allocateDescriptorSets(allocInfo);
    descriptorSet_ = std::move(h.front());
}
