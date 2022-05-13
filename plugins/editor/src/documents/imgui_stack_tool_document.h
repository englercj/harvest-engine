// Copyright Chad Engler

#pragma once

#include "document.h"

namespace he::editor
{
    class ImGuiStackToolDocument : public Document
    {
    public:
        ImGuiStackToolDocument();

        void Show() override;
    };
}
