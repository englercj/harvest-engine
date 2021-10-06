// Copyright Chad Engler

#include "choice_dialog.h"

#include "widgets/buttons.h"

#include "he/core/enum_ops.h"

#include "imgui.h"

namespace he
{
    template <>
    const char* AsString(editor::ChoiceDialog::Choice x)
    {
        switch (x)
        {
            case editor::ChoiceDialog::Choice::Yes: return "Yes";
            case editor::ChoiceDialog::Choice::No: return "No";
            case editor::ChoiceDialog::Choice::YesAll: return "Yes All";
            case editor::ChoiceDialog::Choice::NoAll: return "No All";
            case editor::ChoiceDialog::Choice::OK: return "OK";
            case editor::ChoiceDialog::Choice::Cancel: return "Cancel";
            case editor::ChoiceDialog::Choice::Retry: return "Retry";
            case editor::ChoiceDialog::Choice::Continue: return "Continue";
        }

        return "<unknown>";
    }
}

namespace he::editor
{
    void ChoiceDialog::Configure(const char* title, const char* msg, Button buttons)
    {
        m_title = title;
        m_message = msg;
        m_buttons = buttons;
    }

    void ChoiceDialog::ShowContent()
    {
        ImGui::TextUnformatted(m_message.Data());
    }

    void ChoiceDialog::ShowButtons()
    {
        TryShowButton(Button::Yes, Choice::Yes);
        TryShowButton(Button::No, Choice::No);
        TryShowButton(Button::YesAll, Choice::YesAll);
        TryShowButton(Button::NoAll, Choice::NoAll);
        TryShowButton(Button::OK, Choice::OK);
        TryShowButton(Button::Cancel, Choice::Cancel);
        TryShowButton(Button::Retry, Choice::Retry);
        TryShowButton(Button::Continue, Choice::Continue);
    }

    void ChoiceDialog::TryShowButton(Button button, Choice choice)
    {
        if (HasFlag(m_buttons, button))
        {
            if (DialogButton(AsString(choice)))
            {
                m_result = choice;
                RequestClose();
            }
        }
    }
}
