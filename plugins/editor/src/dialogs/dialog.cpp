// Copyright Chad Engler

#include "dialog.h"

#include "he/core/appender.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"

#include "imgui.h"
#include "fmt/core.h"

namespace he::editor
{
    uint32_t Dialog::s_nextCounter = 0;

    const char* Dialog::GetLabel() const
    {
        // TODO: We can cache this with some API changes if it is too slow.
        static String s_label;
        s_label.Clear();
        fmt::format_to(Appender(s_label), "{} ##dialog-id-{}", m_title, m_counter);
        return s_label.Data();
    }
}
