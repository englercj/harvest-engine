// Copyright Chad Engler

#pragma once

#include "he/editor/documents/document.h"

namespace he::editor
{
    class DevConsoleDocument : public Document
    {
    public:
        DevConsoleDocument() noexcept;

        void Show() override;
    };
}
