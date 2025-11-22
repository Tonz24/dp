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
        ImGui::DragInt("Max bounce count",reinterpret_cast<int*>(&pcs_.maxRecursionDepth),0.25,1,16);
        ImGui::Checkbox("Resampled NEE",reinterpret_cast<bool*>(&pcs_.doRIS));

        if (pcs_.doRIS == 1) {
            ImGui::DragInt("BRDF sample count",reinterpret_cast<int*>(&pcs_.M_brdf),0.25,0,16);
            ImGui::DragInt("Area sample count",reinterpret_cast<int*>(&pcs_.M_area),0.25,0,16);
            ImGui::DragInt("Env sample count",reinterpret_cast<int*>(&pcs_.M_env),0.25,0,16);
        }

        ImGui::Unindent();
    }
    return false;
}
