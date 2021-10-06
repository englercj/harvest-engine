// Copyright Chad Engler

#pragma once

#include "di.h"
#include "documents/document.h"

#include "he/core/utils.h"
#include "he/core/vector.h"

#include <memory>
#include <type_traits>

namespace he::editor
{
    class DocumentService
    {
    public:
        void DestroyClosedDocuments();

        void ShowDocuments();

        template <typename T> requires(std::is_base_of_v<Document, T>)
        T& Open()
        {
            m_documents.PushBack(DICreateUnique<T>());
            return *static_cast<T*>(m_documents.Back().get());
        }

    private:
        Document* m_activeDocument{ nullptr };
        Vector<std::unique_ptr<Document>> m_documents{};
    };
}
