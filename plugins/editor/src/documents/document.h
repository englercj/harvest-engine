// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/string.h"

namespace he::editor
{
    class Document
    {
    public:
        virtual ~Document() {}

        virtual void Show() = 0;
        virtual void ShowPanels() {}
        virtual void ShowContextMenu();
        virtual void ResetDockLayout() {}

        const char* GetLabel() const;

        bool IsCloseRequested() const { return m_wantsClose; }
        void RequestClose() { m_wantsClose = true; }

        bool IsDirty() const { return m_dirty; }

    protected:
        String m_title{};

    private:
        static uint32_t s_nextCounter;

        const uint32_t m_counter{ s_nextCounter++ };
        bool m_dirty{ false };
        bool m_wantsClose{ false };
    };
}
