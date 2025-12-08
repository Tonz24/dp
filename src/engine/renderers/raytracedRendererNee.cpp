//
// Created by Tonz on 24.10.2025.
//

#include "raytracedRendererNee.h"

#include "imgui/imgui.h"

bool RaytracedRendererNEE::drawGUI() {
    bool changed{false};

    if (ImGui::CollapsingHeader("NEE path tracer")) {
        ImGui::Indent();

        ImGui::Checkbox("Tonemap",reinterpret_cast<bool*>(&tonemap_));
        ImGui::Checkbox("Accumulate",reinterpret_cast<bool*>(&pcs_.accumulate));
        changed |= ImGui::DragInt("Max bounce count",reinterpret_cast<int*>(&pcs_.maxRecursionDepth),0.25,1,16);

        changed |= ImGui::Checkbox("Resampled NEE",reinterpret_cast<bool*>(&pcsUnpacked_.doRIS));

        if (pcsUnpacked_.doRIS == 1) {
            changed |= ImGui::DragInt("BRDF sample count",reinterpret_cast<int*>(&pcsUnpacked_.M_brdf),0.25,0,PcsRaygen::maxSampleCount);
            changed |= ImGui::DragInt("Area sample count",reinterpret_cast<int*>(&pcsUnpacked_.M_area),0.25,0,PcsRaygen::maxSampleCount);
            changed |= ImGui::DragInt("Env sample count",reinterpret_cast<int*>(&pcsUnpacked_.M_env),0.25,0,PcsRaygen::maxSampleCount);
        }
        ImGui::Unindent();
    }
    PcsRaygen::packData(pcsUnpacked_,pcs_);

    return changed;
}
