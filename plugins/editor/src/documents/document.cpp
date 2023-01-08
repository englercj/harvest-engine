// Copyright Chad Engler

#include "he/editor/documents/document.h"

#include "he/core/appender.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/editor/di.h"
#include "he/editor/icons/icons_material_design.h"
#include "he/editor/widgets/menu.h"

#include "imgui.h"
#include "fmt/core.h"

namespace he::editor
{
    std::atomic<uint32_t> Document::s_nextId = 1;

    Document::Document()
        // Using DICreate here so every document doesn't have to manually pass this through.
        : m_closeDocumentCommand(DICreateUnique<CloseDocumentCommand>())
    {
        m_closeDocumentCommand->SetDocument(this);
    }

    void Document::ShowContextMenu()
    {
        MenuItem(*m_closeDocumentCommand);
    }

    const char* Document::Label() const
    {
        // TODO: We can cache this with some API changes if it is too slow.
        static String s_label;
        s_label.Clear();
        fmt::format_to(Appender(s_label), "{} ##doc-id-{}", m_title, m_id);
        return s_label.Data();
    }

    void Document::RequestClose()
    {
        if (m_closeDocumentCommand->CanRun())
            m_closeDocumentCommand->Run();
    }
}
