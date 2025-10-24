//
// Created by Tonz on 24.10.2025.
//

#include "raytracedRendererNee.h"

#include "imgui/imgui.h"

bool RaytracedRendererNEE::drawGUI() {
    if (ImGui::CollapsingHeader("NEE path tracer")) {
        ImGui::Indent();

        ImGui::Checkbox("Tonemap",reinterpret_cast<bool*>(&tonemap_));
        ImGui::Checkbox("Accumulate",reinterpret_cast<bool*>(&pcs_.accumulate));
        ImGui::DragInt("Max bounce count",reinterpret_cast<int*>(&pcs_.maxRecursionDepth),0.33,1,16);
        ImGui::Checkbox("Next event estimation",reinterpret_cast<bool*>(&pcs_.NEE));
        ImGui::Checkbox("Sample sky",reinterpret_cast<bool*>(&pcs_.sampleSky));

        ImGui::Unindent();
    }
    return false;
}

RaytracedRendererNEE::RaytracedRendererNEE(const std::shared_ptr<GBuffer>& gBuffer): RaytracedRenderer(gBuffer) {
    initGraphicsPipelines();
}

RaytracedRendererNEE::RaytracedRendererNEE(const std::string_view& gBufferName): RaytracedRenderer(gBufferName) {
    initGraphicsPipelines();
}

void RaytracedRendererNEE::initGraphicsPipelines() {

}
