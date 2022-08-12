// Copyright Chad Engler

#include "close_active_document_command.h"

namespace he::editor
{
    CloseActiveDocumentCommand::CloseActiveDocumentCommand(
        DocumentService& documentService,
        UniquePtr<CloseDocumentCommand> closeDocumentCommand) noexcept
        : m_documentService(documentService)
        , m_closeDocumentCommand(Move(closeDocumentCommand))
    {}

    bool CloseActiveDocumentCommand::CanRun() const
    {
        m_closeDocumentCommand->SetDocument(m_documentService.ActiveDocument());
        return m_closeDocumentCommand->CanRun();
    }

    void CloseActiveDocumentCommand::Run()
    {
        m_closeDocumentCommand->SetDocument(m_documentService.ActiveDocument());
        m_closeDocumentCommand->Run();
    }
}
