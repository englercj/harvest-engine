// Copyright Chad Engler

#include "he/editor/documents/dev_console_document.h"

#include "imgui.h"

namespace he::editor
{
    DevConsoleDocument::DevConsoleDocument() noexcept
    {
        m_title = "Developer Console";
    }

    void DevConsoleDocument::Show()
    {
        ImGui::TextUnformatted("DEV Console");
    }
}
