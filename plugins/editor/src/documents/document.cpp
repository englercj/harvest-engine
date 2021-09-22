// Copyright Chad Engler

#include "document.h"

#include "fonts/IconsFontAwesome5Pro.h"
#include "widgets/menu.h"

#include "imgui.h"
#include "fmt/format.h"

#include <cstdarg>

namespace he::editor
{
    uint32_t Document::s_nextCounter = 0;

    const char* Document::GetLabel() const
    {
        // TODO: We can cache this with some API changes if it is too slow.
        static std::string s_label;
        s_label.clear();
        fmt::format_to(std::back_inserter(s_label), "{} ##doc-id-{}", m_title.c_str(), m_counter);
        return s_label.c_str();
    }

    void Document::ShowContextMenu()
    {
        if (MenuItem("Close", ICON_FA_TIMES, "Ctrl+W"))
        {
            RequestClose();
        }
    }
}
