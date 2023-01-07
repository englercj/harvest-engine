// Copyright Chad Engler

#pragma once

#include "di.h"
#include "documents/document.h"
#include "services/document_service.h"

#include "he/core/hash_table.h"
#include "he/core/type_info.h"
#include "he/core/utils.h"
#include "he/core/unique_ptr.h"
#include "he/core/vector.h"

#include <type_traits>

namespace he::editor
{
    class PanelService
    {
    public:
        PanelService(DocumentService& documentService) noexcept;

        void DestroyClosedPanels();

        void ShowPanels();

        template <typename T> requires(std::is_base_of_v<Document, T>)
        void Open()
        {
            constexpr TypeInfo Info = TypeInfo::Get<T>();

            Close();
            m_entry.type = Info;
            m_entry.openTime = 0;
            m_entry.panel = DICreateUnique<T>();
        }

        void Close()
        {
            if (m_entry.panel)
            {
                m_entry.panel->Close();
                m_pendingClose.PushBack(Move(m_entry));
            }
        }

        template <typename T> requires(std::is_base_of_v<Document, T>)
        bool IsOpen()
        {
            constexpr TypeInfo Info = TypeInfo::Get<T>();
            return m_entry.panel && m_entry.type == Info;
        }

    private:
        struct PanelEntry
        {
            TypeInfo type{};
            double openTime{ 0 };
            UniquePtr<Document> panel{};
        };

    private:
        DocumentService& m_documentService;

        PanelEntry m_entry{};
        Vector<PanelEntry> m_pendingClose{};
    };
}
