// Copyright Chad Engler

#pragma once

#include "he/editor/documents/document.h"
#include "he/editor/services/type_edit_ui_service.h"

namespace he::editor
{
    class ImGuiDebugDocument : public Document
    {
    public:
        ImGuiDebugDocument(TypeEditUIService& s) noexcept;

        void Show() override;

    private:
        void ShowCustomWidgetsTab();
        void ShowPropertyGridTab();

    private:
        TypeEditUIService& m_editUIService;

        bool m_demoOpen{ false };
    };
}
