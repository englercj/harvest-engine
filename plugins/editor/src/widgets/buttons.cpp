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
        const bool pressed = ImGui::InvisibleButton("###behavior", ImGui::GetItemRectSize());

        if (ImGui::IsItemHovered())
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            SetItemUnderLine(LinkColor);
        }

        ImGui::PopID();

        return pressed;
    }

    bool ConditionButton(const char* label, bool enabled, const ImVec2& size)
    {
        ImGuiStyle& style = ImGui::GetStyle();

        if (!enabled)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, style.Colors[ImGuiCol_Button]);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, style.Colors[ImGuiCol_Button]);
        }

        const bool pressed = ImGui::Button(label, size);

        if (!enabled)
        {
            ImGui::PopStyleColor(3);
            return false; // always false if disabled
        }

        return pressed;
    }

    bool DialogButton(const char* label, bool enabled)
    {
        const float dpiScale = ImGui::GetWindowDpiScale();
        return ConditionButton(label, enabled, ImVec2(UnscaledDialogButtonWidth * dpiScale, 0));
    }
}
