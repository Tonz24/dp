//
// Created by Tonz on 23.07.2025.
//

#pragma once
#include <glm/glm.hpp>

#include "../engine/iDrawGui.h"

class Transform : public IDrawGui {
public:

    void translate(const glm::vec3& translation);
    void setTranslation(const glm::vec3& translation);

    void rotate(const glm::vec3& rotation);
    void setRotation(const glm::vec3& rotation);

    void scale(const glm::vec3& scale);
    void setScale(const glm::vec3& scale);

    [[nodiscard]] const glm::mat4& getModelMat() const;
    [[nodiscard]] const glm::mat3& getNormalMat() const;

    bool drawGUI() override;

private:

    glm::vec3 translation_{};
    glm::vec3 rotation_{};
    glm::vec3 scale_{1.0};

    glm::mat4 modelMat_{1.0f};
    glm::mat3 normalMat_{1.0f};

    void applyTransform();

};
