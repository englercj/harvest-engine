// Copyright Chad Engler

#pragma once

#include "document.h"

namespace he::editor
{
    class ImGuiStyleEditorDocument : public Document
    {
    public:
        ImGuiStyleEditorDocument();

        void Show() override;
    };
}
