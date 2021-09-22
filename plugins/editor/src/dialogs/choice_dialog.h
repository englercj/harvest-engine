// Copyright Chad Engler

#pragma once

#include "dialog.h"

#include "he/core/enum_ops.h"

#include <string>

namespace he::editor
{
    class ChoiceDialog : public Dialog
    {
    public:
        enum class Choice : uint8_t
        {
            Yes,
            No,
            YesAll,
            NoAll,
            OK,
            Cancel,
            Retry,
            Continue,
        };

        enum class Button : uint32_t
        {
            None        = 0,
            Yes         = (1 << 0),
            No          = (1 << 1),
            YesAll      = (1 << 2),
            NoAll       = (1 << 3),
            OK          = (1 << 4),
            Cancel      = (1 << 5),
            Retry       = (1 << 6),
            Continue    = (1 << 7),
        };

    public:
        void Configure(const char* title, const char* msg, Button buttons = Button::OK);

        void ShowContent() override;
        void ShowButtons() override;

    private:
        void TryShowButton(Button button, Choice choice);

    private:
        std::string m_message{};
        Button m_buttons{ Button::OK };
        Choice m_result{ Choice::Cancel };
    };

    HE_ENUM_FLAGS(ChoiceDialog::Button);

    const char* AsString(ChoiceDialog::Choice);
}
