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

        ImGui::Unindent();
    }
    return false;
}
