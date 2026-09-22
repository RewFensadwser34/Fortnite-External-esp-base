#pragma once

#include "imgui/imgui.h"

namespace menu {

inline bool open = true;

inline int page = 0;

namespace visuals {
inline bool enabled = false;

inline bool box_esp = true;
inline ImVec4 box_color = ImVec4(1.0f, 0.15f, 0.15f, 1.0f);
inline float box_thickness = 1.5f;
inline bool box_outline = true;
} 

inline void setup_style() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 8.0f;
    s.FrameRounding = 4.0f;
    s.WindowBorderSize = 1.0f;

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.09f, 0.95f);
    c[ImGuiCol_ChildBg] = ImVec4(0.09f, 0.09f, 0.12f, 1.0f);
    c[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.14f, 0.18f, 1.0f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.26f, 1.0f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.24f, 0.32f, 1.0f);
    c[ImGuiCol_Button] = ImVec4(0.55f, 0.15f, 0.15f, 1.0f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.70f, 0.20f, 0.20f, 1.0f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.45f, 0.12f, 0.12f, 1.0f);
    c[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.25f, 0.25f, 1.0f);
    c[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.25f, 0.25f, 1.0f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.85f, 0.18f, 0.18f, 1.0f);
    c[ImGuiCol_Tab] = ImVec4(0.12f, 0.12f, 0.15f, 1.0f);
    c[ImGuiCol_TabHovered] = ImVec4(0.55f, 0.15f, 0.15f, 1.0f);
    c[ImGuiCol_TabActive] = ImVec4(0.55f, 0.15f, 0.15f, 1.0f);
}


inline void render() {
    if (!open)
        return;

    ImGui::SetNextWindowSize(ImVec2(420, 320), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(120, 120), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("FORTNITE EXTRENAL BASE -- ENZO.DEV", &open, ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("pages")) {
        if (ImGui::BeginTabItem("Visuals")) {
            page = 0;

            ImGui::Checkbox("Enable", &visuals::enabled);

            ImGui::BeginDisabled(!visuals::enabled);

            ImGui::Checkbox("Box ESP", &visuals::box_esp);

            ImGui::BeginDisabled(!visuals::box_esp);
            ImGui::ColorEdit4("Box color", (float*)&visuals::box_color,
                              ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
            ImGui::SliderFloat("Thickness", &visuals::box_thickness, 1.0f, 5.0f, "%.1f");
            ImGui::Checkbox("Outline", &visuals::box_outline);
            ImGui::EndDisabled(); 

            ImGui::EndDisabled(); 

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

}
