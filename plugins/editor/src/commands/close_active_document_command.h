// Copyright Chad Engler

#pragma once

#include "command.h"
#include "close_document_command.h"
#include "fonts/icons_material_design.h"
#include "services/document_service.h"

#include "he/core/types.h"
#include "he/core/unique_ptr.h"

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
