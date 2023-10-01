// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/editor/dialogs/choice_dialog.h"

namespace he::editor
{
    class AboutDialog : public ChoiceDialog
    {
    public:
        AboutDialog() noexcept;
    };
}
