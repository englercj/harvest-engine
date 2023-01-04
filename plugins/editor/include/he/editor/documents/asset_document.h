// Copyright Chad Engler

#pragma once

#include "he/editor/documents/document.h"
#include "framework/schema_edit.h"

#include "he/core/assert.h"
#include "he/core/types.h"

namespace he::editor
{
    class AssetDocument : public Document
    {
    public:
        void SetContext(SchemaEditContext* ctx) { HE_ASSERT(!m_ctx); m_ctx = ctx; }

    private:
        SchemaEditContext* m_ctx{ nullptr };
    };
}
