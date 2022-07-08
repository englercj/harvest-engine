// Copyright Chad Engler

#include "stats_document.h"

#include "widgets/menu.h"

#include "imgui.h"

namespace he::editor
{
    StatsDocument::StatsDocument()
    {
        m_title = "App Stats";
        m_frameTime.SetDisplayUnits<Milliseconds>();
    }

    void StatsDocument::Show()
    {
        m_frameTime.Update(ImGui::GetIO().DeltaTime);
        m_frameTime.Show();
    }
}
