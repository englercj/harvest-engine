// Copyright Chad Engler

#include "he/editor/dialogs/about_dialog.h"

namespace he::editor
{
    AboutDialog::AboutDialog() noexcept
    {
        Configure(
            "About Harvest",
            "Harvest Engine v1.0.0\n\nCopyright© 2023 Chad Engler, All rights reserved.",
            ChoiceDialog::Button::OK);
    }
}
