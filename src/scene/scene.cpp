//
// Created by Tonz on 04.08.2025.
//

#include "scene.h"

#include <imgui/imgui.h>



bool Scene::drawGUI() {

    if (ImGui::CollapsingHeader("Scene")) {
        ImGui::Indent();
        ImGui::Text("Selected mesh: ");
        ImGui::SameLine();

        if (selectedObject_ != nullptr) {
            ImGui::Text(selectedObject_->getResourceName().c_str());
            selectedObject_->drawGUI();
        }

        ImGui::Unindent();
    }

    return false;
}

