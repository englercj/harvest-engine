// Copyright Chad Engler

#include "buttons.h"

#include "menu.h"
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
        return ImGui::Button(label, ImVec2(DialogButtonWidth(), 0));
    }

    float DialogButtonWidth()
    {
        constexpr float DialogButtonWidth = 80.0f;
        return DialogButtonWidth * ImGui::GetWindowDpiScale();
    }

    bool CommandButton(const char* label, Command& cmd)
    {
        const bool disabled = !cmd.CanRun();
        ImGui::BeginDisabled(disabled);

        const bool active = ImGui::Button(label);
        if (active)
            cmd.Run();

        ImGui::EndDisabled();
        return active;
    }

    bool CommandLinkButton(const char* label, Command& cmd)
    {
        const bool disabled = !cmd.CanRun();
        ImGui::BeginDisabled(disabled);

        const bool active = LinkButton(label);
        if (active)
            cmd.Run();

        ImGui::EndDisabled();
        return active;
    }

    bool BeginPopupMenuButton(const char* label, const char* popupId, const ImVec2& size)
    {
        const bool clicked = ImGui::Button(label, size);
        const ImVec2 min = ImGui::GetItemRectMin();
        const ImVec2 max = ImGui::GetItemRectMax();

        if (clicked)
        {
            ImGui::OpenPopup(popupId);
        }

        ImGui::SetNextWindowPos(ImVec2(min.x, max.y), ImGuiCond_Always);
        return BeginPopupMenu(popupId);
    }

    void EndPopupMenuButton()
    {
        EndPopupMenu();
    }
}
