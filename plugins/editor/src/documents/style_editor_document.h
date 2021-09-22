// Copyright Chad Engler

#pragma once

#include "document.h"

namespace he::editor
{
    class StyleEditorDocument : public Document
    {
    public:
        StyleEditorDocument();

        void Show() override;
    };
}
