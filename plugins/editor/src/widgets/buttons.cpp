// Copyright Chad Engler

#include "buttons.h"

#include "misc.h"

#include "imgui.h"
#include "imgui_internal.h"

namespace he::editor
{
    bool LinkButton(const char* label)
    {
        ImGui::PushID(label);

        ImVec2 pos = ImGui::GetCursorPos();

        const ImVec4 LinkColor{ 0.41f, 0.65f, 0.88f, 1.0f };
        //const ImVec4 LinkColor = ImGui::GetStyle().Colors[ImGuiCol_HeaderHovered];

        ImGui::PushStyleColor(ImGuiCol_Text, LinkColor);
        ImGui::TextUnformatted(label);
        ImGui::PopStyleColor();

        ImGui::SetCursorPos(pos);
        ImGui::SetItemAllowOverlap();
        const bool pressed = ImGui::InvisibleButton("##link_behavior", ImGui::GetItemRectSize());

        if (ImGui::IsItemHovered())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            SetItemUnderLine(LinkColor);
        }

        ImGui::PopID();

        return pressed;
    }

    bool DialogButton(const char* label)
    {
        const float dpiScale = ImGui::GetWindowDpiScale();
        return ImGui::Button(label, ImVec2(UnscaledDialogButtonWidth * dpiScale, 0));
    }
}
