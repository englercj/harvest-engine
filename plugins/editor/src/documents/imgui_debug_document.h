// Copyright Chad Engler

#pragma once

#include "document.h"

namespace he::editor
{
    class ImGuiDebugDocument : public Document
    {
    public:
        ImGuiDebugDocument() noexcept;

        void Show() override;

    private:
        void ShowCustomWidgetsTab();
        void ShowPropertyGridTab();

    private:
        bool m_demoOpen{ false };
    };
}
