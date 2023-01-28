// Copyright Chad Engler

#pragma once

#include "he/core/type_traits.h"
#include "he/core/utils.h"
#include "he/core/unique_ptr.h"
#include "he/core/vector.h"
#include "he/editor/di.h"
#include "he/editor/documents/document.h"

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

        template <typename T> requires(IsBaseOf<Document, T>)
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
