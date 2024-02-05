// Copyright Chad Engler

#include "he/editor/dialogs/choice_dialog.h"

#include "he/core/enum_ops.h"
#include "he/editor/widgets/buttons.h"

#include "imgui.h"

namespace he
{
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
            case editor::ChoiceDialog::Choice::Save: return "Save";
            case editor::ChoiceDialog::Choice::DontSave: return "Don't Save";
        }

        return "<unknown>";
    }
}

namespace he::editor
{
    void ChoiceDialog::Configure(const char* title, const char* msg, Button buttons, ResultDelegate callback)
    {
        m_title = title;
        m_message = msg;
        m_buttons = buttons;
        m_callback = callback;
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
        TryShowButton(Button::Retry, Choice::Retry);
        TryShowButton(Button::Continue, Choice::Continue);
        TryShowButton(Button::Save, Choice::Save);
        TryShowButton(Button::DontSave, Choice::DontSave);
        TryShowButton(Button::Cancel, Choice::Cancel);
    }

    void ChoiceDialog::TryShowButton(Button button, Choice choice)
    {
        if (HasFlag(m_buttons, button))
        {
            if (DialogButton(AsString(choice)))
            {
                if (m_callback)
                    m_callback(choice);
                Close();
            }
        }
    }
}
