// Copyright Chad Engler

#pragma once

#include "di.h"
#include "documents/document.h"

#include "he/core/type_traits.h"
#include "he/core/utils.h"
#include "he/core/vector.h"

#include <memory>

namespace he::editor
{
    class DocumentService
    {
    public:
        DocumentService(Allocator& allocator);

        void DestroyClosedDocuments();

        void ShowDocuments();

        template <typename T>
        T& Open()
        {
            static_assert(std::is_base_of_v<Document, T>, "Document Service can only open Document objects.");
            m_documents.PushBack(DICreateUnique<T>());
            return *static_cast<T*>(m_documents.Back().get());
        }

    private:
        Document* m_activeDocument{ nullptr };
        Vector<std::unique_ptr<Document>> m_documents;
    };
}
