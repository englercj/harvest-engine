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

        Document& Open(UniquePtr<Document> document)
        {
            m_documents.PushBack(Move(document));
            return *m_documents.Back();
        }

        template <typename T> requires(std::is_base_of_v<Document, T>)
        T& Open()
        {
            Document& doc = Open(DICreateUnique<T>());
            return static_cast<T&>(doc);
        }

    private:
        Document* m_activeDocument{ nullptr };
        Vector<UniquePtr<Document>> m_documents{};
    };
}
