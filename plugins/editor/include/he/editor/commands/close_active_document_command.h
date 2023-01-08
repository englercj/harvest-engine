// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/core/unique_ptr.h"
#include "he/editor/commands/command.h"
#include "he/editor/commands/close_document_command.h"
#include "he/editor/icons/icons_material_design.h"
#include "he/editor/services/document_service.h"

namespace he::editor
{
    class CloseActiveDocumentCommand : public Command
    {
    public:
        CloseActiveDocumentCommand(
            DocumentService& documentService,
            UniquePtr<CloseDocumentCommand> closeDocumentCommand) noexcept;

        bool CanRun() const override;
        void Run() override;

        const char* Label() const override { return "Close Active Document"; }
        const char* Icon() const override { return ICON_MDI_CLOSE; }

    private:
        DocumentService& m_documentService;

        UniquePtr<CloseDocumentCommand> m_closeDocumentCommand;
    };
}
