//
// Created by Tonz on 04.08.2025.
//

#include "scene.h"

#include <imgui/imgui.h>



bool Scene::drawGUI() {

    if (ImGui::CollapsingHeader("Scene")) {
        ImGui::Indent();
        ImGui::Text("Selected mesh ");
        ImGui::SameLine();

        if (ImGui::ArrowButton("left",ImGuiDir_Left) && selectedObjectIndex_ != 0 )
           selectedObjectIndex_--;

        ImGui::SameLine();
        ImGui::Text(std::to_string(selectedObjectIndex_).c_str());
        ImGui::SameLine();
        if (ImGui::ArrowButton("right",ImGuiDir_Right) && selectedObjectIndex_ < meshes_.size() - 1)
           selectedObjectIndex_++;
        ImGui::SameLine();

        ImGui::Text(meshes_[selectedObjectIndex_]->getResourceName().c_str());
        meshes_[selectedObjectIndex_]->drawGUI();

        ImGui::Unindent();
    }

    return false;
}

