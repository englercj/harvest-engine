// Copyright Chad Engler

#include "document.h"

#include "fonts/icons_material_design.h"
#include "widgets/menu.h"

#include "he/core/appender.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"

#include "imgui.h"
#include "fmt/core.h"

namespace he::editor
{
    uint32_t Document::s_nextCounter = 0;

    const char* Document::GetLabel() const
    {
        // TODO: We can cache this with some API changes if it is too slow.
        static String s_label;
        s_label.Clear();
        fmt::format_to(Appender(s_label), "{} ##doc-id-{}", m_title, m_counter);
        return s_label.Data();
    }

    void Document::ShowContextMenu()
    {
        if (MenuItem("Close", ICON_MDI_CLOSE, "Ctrl+W"))
        {
            RequestClose();
        }
    }
}
