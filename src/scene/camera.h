//
// Created by Tonz on 04.08.2025.
//

#pragma once


#include <glm/glm.hpp>

#include "../engine/iDrawGui.h"
#include "../engine/uboFormat.h"


class Camera : public UBOFormat<CameraUBOFormat>, public IDrawGui {
public:

    explicit Camera(const glm::vec3& position, const glm::vec3& viewAt, float verticalFov = 45.0f);

    [[nodiscard]] float getVerticalFov(bool radians) const;

    [[nodiscard]] const glm::mat4 &getViewMat() const;

    [[nodiscard]] const glm::mat4 &getProjMat() const;

    [[nodiscard]] const glm::mat4 &getViewProjMat() const;

    [[nodiscard]] const glm::mat4 &getInvViewProjMat() const;

    [[nodiscard]] const glm::vec3 & getPositionWorld() const { return uboFormat_.positionWorld; }

    void resetMovementFlag() {isMoving_ = false;}
    [[nodiscard]] bool getIsMoving() const { return isMoving_; }

    void updateOrientation(double dx, double dy);
    void updatePosition(const glm::vec3& velocity);

    bool drawGUI() override;

private:

    void recalculateMatrices();

    void recalculateViewMat();

    void recalculateProjMat();

    void recalculateCompoundMatrices();

    void updateCameraVectors();

    glm::vec3 camForwardDir_{}, camRightDir_{}, camUpDir_{};
    glm::vec3& positionWorld_;

    float verticalFov_{45.0f};
    float aspectRatio_{16.0f / 9.0f};

    float yaw_{0.0f}, pitch_{0.0f};
    float movementSpeed_{0.5f};
    bool isMoving_{false};

    static constexpr glm::vec3 worldUp_{0,1,0};

    bool dirty_{true};
};
