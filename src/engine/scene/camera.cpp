//
// Created by Tonz on 04.08.2025.
//

#include "camera.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

Camera::Camera(const glm::vec3 &position, const glm::vec3 &viewAt, float verticalFov) : camForwardDir_(glm::normalize(viewAt - position)), positionWorld_(uboFormat_.positionWorld), verticalFov_(verticalFov){

    positionWorld_ = position;

    camRightDir_ = glm::normalize(glm::cross(camForwardDir_,worldUp_));
    camUpDir_ = glm::normalize(glm::cross(camRightDir_,camForwardDir_));

    glm::vec3 posNorm = glm::normalize(uboFormat_.positionWorld);

    //  init to reflect actual camera orientation set by position and viewAt
    yaw_ = glm::degrees(glm::atan(posNorm.z, posNorm.x)) - 180.0f;
    pitch_ = -glm::degrees(glm::asin(posNorm.y));
    recalculateMatrices();
}

float Camera::getVerticalFov(bool radians) const {
    if (radians)
        return glm::radians(verticalFov_);
    return verticalFov_;
}

const glm::mat4 &Camera::getViewMat() const {
    return uboFormat_.matView;
}

const glm::mat4 &Camera::getProjMat() const {
    return uboFormat_.matProj;
}

const glm::mat4 &Camera::getViewProjMat() const {
    return uboFormat_.matViewProj;
}

const glm::mat4 &Camera::getInvViewProjMat() const {
    return uboFormat_.matInvViewProj;
}

void Camera::updateOrientation(double dx, double dy) {
    yaw_ += dx * 0.03;
    pitch_ -= dy * 0.03;

    if (pitch_ > 89.0f)
        pitch_ = 89.0f;

    if (pitch_ < -89.0f)
        pitch_ = -89.0f;

    updateCameraVectors();
}

void Camera::updatePosition(const glm::vec3 &velocity) {
    glm::vec3 oldPos{positionWorld_};
    positionWorld_ += camRightDir_ * velocity.x;
    positionWorld_ += camUpDir_ * velocity.y;
    positionWorld_ += camForwardDir_ * velocity.z;

    if (positionWorld_ != oldPos) {
        recalculateViewMat();
        recalculateCompoundMatrices();
    }
}

void Camera::recalculateMatrices() {
    recalculateViewMat();
    recalculateProjMat();
    recalculateCompoundMatrices();
}

void Camera::recalculateViewMat() {
    uboFormat_.matView = glm::lookAt(positionWorld_,positionWorld_ + camForwardDir_,camUpDir_);
}

void Camera::recalculateProjMat() {
    uboFormat_.matProj = glm::perspective(getVerticalFov(true), aspectRatio_, zNear_, zFar_);
}

void Camera::recalculateCompoundMatrices() {
    uboFormat_.matViewProj = uboFormat_.matProj * uboFormat_.matView;
    uboFormat_.matInvViewProj = glm::inverse(uboFormat_.matViewProj);
}

void Camera::updateCameraVectors() {
    // calculate the new front vector
    camForwardDir_.x = glm::cos(glm::radians(yaw_)) * glm::cos(glm::radians(pitch_));
    camForwardDir_.y = glm::sin(glm::radians(pitch_));
    camForwardDir_.z = glm::sin(glm::radians(yaw_)) * glm::cos(glm::radians(pitch_));
    camForwardDir_ = glm::normalize(camForwardDir_);

    // also recalculate the right and up vector
    camRightDir_ = glm::normalize(glm::cross(camForwardDir_,worldUp_));
    camUpDir_ = glm::normalize(glm::cross(camRightDir_,camForwardDir_));

    // then recalculate affected matrices
    recalculateViewMat();
    recalculateCompoundMatrices();
}
