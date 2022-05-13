// Copyright Chad Engler

#include "imgui_style_editor_document.h"

#include "imgui.h"

namespace he::editor
{
    ImGuiStyleEditorDocument::ImGuiStyleEditorDocument()
    {
        m_title = "ImGui Style Editor";
    }

    void ImGuiStyleEditorDocument::Show()
    {
        ImGui::ShowStyleEditor();
    }
}
