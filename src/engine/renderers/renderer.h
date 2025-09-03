//
// Created by Tonz on 03.09.2025.
//

#pragma once
#include "../../scene/scene.h"

class Renderer : public IDrawGui {
public:
    virtual void render(const Scene& scene) = 0;

    ~Renderer() override = default;

protected:
    Renderer() = default;

    virtual void recordCommandBuffer() = 0;
};