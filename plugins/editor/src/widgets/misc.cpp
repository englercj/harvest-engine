// Copyright Chad Engler

#include "misc.h"

#include "fonts/icons_material_design.h"

#include "imgui.h"

namespace he::editor
{
    void ShowHelpMarker(const char* helpText)
    {
        ImGui::TextDisabled(" " ICON_MDI_HELP_CIRCLE " ");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(450.0f);
            ImGui::TextUnformatted(helpText);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    void SetItemUnderLine(ImColor color)
    {
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        max.y -= 2;
        min.y = max.y;
        ImGui::GetWindowDrawList()->AddLine(min, max, color, 1.0f);
    }
}
