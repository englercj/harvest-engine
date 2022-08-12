// Copyright Chad Engler

#pragma once

#include "di.h"
#include "documents/document.h"

#include "he/core/utils.h"
#include "he/core/unique_ptr.h"
#include "he/core/vector.h"

#include <type_traits>

namespace he::editor
{
    class DocumentService
    {
    public:
        void DestroyClosedDocuments();

        void ShowDocuments();

        Document* ActiveDocument() const { return m_activeDocument; }

        template <typename T> requires(std::is_base_of_v<Document, T>)
        T& Open()
        {
            m_documents.PushBack(DICreateUnique<T>());
            return *static_cast<T*>(m_documents.Back().Get());
        }

    private:
        Document* m_activeDocument{ nullptr };
        Vector<UniquePtr<Document>> m_documents{};
    };
}
