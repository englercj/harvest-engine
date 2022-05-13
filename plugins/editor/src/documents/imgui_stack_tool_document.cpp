// Copyright Chad Engler

#include "imgui_stack_tool_document.h"

#include "imgui.h"

namespace he::editor
{
    ImGuiStackToolDocument::ImGuiStackToolDocument()
    {
        m_title = "ImGui Stack Tool";
    }

    void ImGuiStackToolDocument::Show()
    {
        ImGui::ShowStackToolWindow();
    }
}
