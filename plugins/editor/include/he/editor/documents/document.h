// Copyright Chad Engler

#pragma once

#include "he/core/atomic.h"
#include "he/core/string.h"
#include "he/core/types.h"
#include "he/core/unique_ptr.h"
#include "he/editor/commands/close_document_command.h"

namespace he::editor
{
    class Document
    {
    public:
        Document();
        virtual ~Document() = default;

        virtual void Show() = 0;
        virtual void ShowContextMenu();
        virtual void ShowMainMenu() {}
        virtual void ResetDockLayout() {}

        const char* Label() const;
        const String& Title() const { return m_title; }

        bool IsClosing() const { return m_pendingClose; }
        void Close() { m_pendingClose = true; }
        void RequestClose();

        bool IsDirty() const { return m_dirty; }

    protected:
        String m_title{};
        bool m_dirty{ false };

    private:
        static Atomic<uint32_t> s_nextId;

        UniquePtr<CloseDocumentCommand> m_closeDocumentCommand;

        const uint32_t m_id{ s_nextId++ };
        bool m_pendingClose{ false };
    };
}
