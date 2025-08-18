//
// Created by Tonz on 23.07.2025.
//

#include "transform.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <imgui/imgui.h>

void Transform::translate(const glm::vec3 &translation) {
    this->translation_ += translation;
    applyTransform();
}

void Transform::setTranslation(const glm::vec3 &translation) {
    this->translation_ = translation;
    applyTransform();
}

void Transform::rotate(const glm::vec3 &rotation) {
    this->rotation_ += rotation;
    applyTransform();
}

void Transform::setRotation(const glm::vec3 &rotation) {
    this->rotation_ = rotation;
    applyTransform();
}

void Transform::scale(const glm::vec3 &scale) {
    this->scale_ += scale;
    applyTransform();
}

void Transform::setScale(const glm::vec3 &scale) {
    this->scale_ = scale;
    applyTransform();
}

void Transform::applyTransform() {
    glm::mat4 translation = glm::translate(glm::mat4{1},this->translation_);
    glm::mat4 rotation = glm::toMat4(glm::quat(glm::radians(this->rotation_))); //https://gamedev.stackexchange.com/questions/13436/glm-euler-angles-to-quaternion
    glm::mat4 scale = glm::scale(glm::mat4{1},this->scale_);

    modelMat_ = translation * rotation * scale;
    normalMat_ = glm::transpose(glm::inverse(glm::mat3(modelMat_)));
}

const glm::mat4 &Transform::getModelMat() const {
    return modelMat_;
}

const glm::mat3 & Transform::getNormalMat() const {
    return normalMat_;
}

bool Transform::drawGUI() {

    bool changed{false};

    if (ImGui::CollapsingHeader("Transform")) {
        ImGui::Indent();
        changed |= ImGui::DragFloat3("Translation",&translation_[0],0.01f);
        changed |= ImGui::DragFloat3("Rotation",&rotation_[0],0.01f);
        changed |= ImGui::DragFloat3("Scale",&scale_[0],0.01f);

        if (changed)
            applyTransform();

        ImGui::Unindent();
    }

    return changed;
}