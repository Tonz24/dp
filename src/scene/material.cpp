//
// Created by Tonz on 28.07.2025.
//

#include "material.h"

#include <imgui/imgui.h>

#include <utility>

#include "../engine/engine.h"

std::shared_ptr<Texture> Material::getTexture(TextureMapSlot slot) {
    if (slot == TextureMapSlot::invalidMapSlot){
        std::cerr << "ERROR: trying to get texture at invalid slot offset!" << std::endl;
        exit(EXIT_FAILURE);
    }
    return textures_[static_cast<uint8_t>(slot)];
}

void Material::setTexture(std::shared_ptr<Texture> texture, TextureMapSlot slot) {
    if (slot == TextureMapSlot::invalidMapSlot){
        std::cerr << "ERROR: trying to set texture at invalid slot offset!" << std::endl;
        exit(EXIT_FAILURE);
    }
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

    for (uint32_t i = 0; i < textures_.size(); ++i) {

        imageInfos.emplace_back(vk::DescriptorImageInfo{
            .sampler = textures_[i] ? textures_[i]->getVkSampler() : Texture::getDummy().getVkSampler(),
            .imageView = textures_[i] ? textures_[i]->getVkImageView() : Texture::getDummy().getVkImageView(),
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
    auto uboMapped = Engine::getInstance().getMaterialUBO();
    memcpy(uboMapped + getCID() * sizeof(MaterialUBOFormat), &uboFormat_,sizeof(uboFormat_));
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
