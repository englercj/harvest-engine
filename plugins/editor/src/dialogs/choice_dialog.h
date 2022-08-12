// Copyright Chad Engler

#pragma once

#include "dialog.h"

#include "he/core/delegate.h"
#include "he/core/enum_ops.h"
#include "he/core/string.h"

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
            Save,
            DontSave,
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
            Save        = (1 << 8),
            DontSave    = (1 << 9),
        };

        using ResultDelegate = Delegate<void(Choice)>;

    public:
        void Configure(const char* title, const char* msg, Button buttons = Button::OK, ResultDelegate callback = {});

        void ShowContent() override;
        void ShowButtons() override;

    private:
        void TryShowButton(Button button, Choice choice);

    private:
        String m_message{};
        Button m_buttons{ Button::OK };
        ResultDelegate m_callback{};
    };

    HE_ENUM_FLAGS(ChoiceDialog::Button);
}
