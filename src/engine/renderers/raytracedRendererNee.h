//
// Created by Tonz on 24.10.2025.
//

#pragma once
#include "raytracedRenderer.h"


class RaytracedRendererNEE : public RaytracedRenderer {
public:

    bool drawGUI() override;

    explicit RaytracedRendererNEE(const std::shared_ptr<GBuffer>& gBuffer);
    explicit RaytracedRendererNEE(const std::string_view& gBufferName);
private:
    void initGraphicsPipelines();
};
