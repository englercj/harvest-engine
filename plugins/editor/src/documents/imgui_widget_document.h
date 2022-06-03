// Copyright Chad Engler

#pragma once

#include "document.h"

namespace he::editor
{
    class ImGuiWidgetDocument : public Document
    {
    public:
        ImGuiWidgetDocument();

        void Show() override;
    };
}
