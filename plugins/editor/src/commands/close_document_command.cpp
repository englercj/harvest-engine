// Copyright Chad Engler

#include "he/editor/commands/close_document_command.h"

#include "he/editor/dialogs/choice_dialog.h"
#include "he/editor/documents/document.h"

namespace he::editor
{
    CloseDocumentCommand::CloseDocumentCommand(
            DialogService& dialogService) noexcept
        : m_dialogService(dialogService)
    {}

    bool CloseDocumentCommand::CanRun() const
    {
        return m_doc != nullptr;
    }

    void CloseDocumentCommand::Run()
    {
        if (m_doc->IsDirty())
        {
            constexpr const char Title[] = "Unsaved Changes";

            String msg = "Do you want to save the changes you made to ";
            msg += m_doc->Title();
            msg += "?\n\nYour changes will be lost if you don't save them.";

            ChoiceDialog::Button buttons = ChoiceDialog::Button::Save | ChoiceDialog::Button::DontSave | ChoiceDialog::Button::Cancel;
            ChoiceDialog::ResultDelegate cb = ChoiceDialog::ResultDelegate::Make<&CloseDocumentCommand::CloseDocument>(this);

            m_dialogService.Open<ChoiceDialog>()
                .Configure(Title, msg.Data(), buttons, cb);
        }
        else
        {
            CloseDocument();
        }
    }

    void CloseDocumentCommand::CloseDocument() const
    {
        m_doc->Close();
    }
}
