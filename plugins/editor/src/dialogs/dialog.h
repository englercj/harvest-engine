// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/string.h"

namespace he::editor
{
    class Dialog
    {
    public:
        virtual ~Dialog() {}

        virtual void ShowContent() = 0;
        virtual void ShowButtons() = 0;

        const char* GetLabel() const;

        bool IsCloseRequested() const { return m_wantsClose; }
        void RequestClose() { m_wantsClose = true; }

    protected:
        String m_title{};

    private:
        static uint32_t s_nextCounter;

        const uint32_t m_counter{ s_nextCounter++ };
        bool m_wantsClose{ false };
    };
}
