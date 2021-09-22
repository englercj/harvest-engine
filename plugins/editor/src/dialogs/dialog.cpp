// Copyright Chad Engler

#include "dialog.h"

#include "fmt/format.h"

#include "imgui.h"

#include <cstdarg>

namespace he::editor
{
    uint32_t Dialog::s_nextCounter = 0;

    const char* Dialog::GetLabel() const
    {
        // TODO: We can cache this with some API changes if it is too slow.
        static std::string s_label;
        s_label.clear();
        fmt::format_to(std::back_inserter(s_label), "{} ##dialog-id-{}", m_title.c_str(), m_counter);
        return s_label.c_str();
    }
}
