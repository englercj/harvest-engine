// Copyright Chad Engler

#include "choice_dialog.h"

#include "widgets/buttons.h"

#include "imgui.h"

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
        ImGui::TextUnformatted(m_message.c_str());
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

    const char* AsString(ChoiceDialog::Choice choice)
    {
        switch (choice)
        {
            case ChoiceDialog::Choice::Yes: return "Yes";
            case ChoiceDialog::Choice::No: return "No";
            case ChoiceDialog::Choice::YesAll: return "Yes All";
            case ChoiceDialog::Choice::NoAll: return "No All";
            case ChoiceDialog::Choice::OK: return "OK";
            case ChoiceDialog::Choice::Cancel: return "Cancel";
            case ChoiceDialog::Choice::Retry: return "Retry";
            case ChoiceDialog::Choice::Continue: return "Continue";
        }

        return "<Unknown Choice>";
    }
}
