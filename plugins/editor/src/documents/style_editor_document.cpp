// Copyright Chad Engler

#include "style_editor_document.h"

#include "fonts/IconsFontAwesome5Pro.h"
#include "widgets/menu.h"

#include "imgui.h"

namespace he::editor
{
    StyleEditorDocument::StyleEditorDocument()
    {
        m_title = "Style Editor";
    }

    void StyleEditorDocument::Show()
    {
        ImGui::ShowStyleEditor();
    }
}
