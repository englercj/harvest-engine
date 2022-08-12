// Copyright Chad Engler

#pragma once

#include "command.h"
#include "fonts/icons_material_design.h"
#include "services/dialog_service.h"

#include "he/core/types.h"
#include "he/core/unique_ptr.h"

namespace he::editor
{
    class Document;

    class CloseDocumentCommand : public Command
    {
    public:
        CloseDocumentCommand(
            DialogService& dialogService) noexcept;

        bool CanRun() const override;
        void Run() override;

        const char* Label() const override { return "Close Document"; }
        const char* Icon() const override { return ICON_MDI_CLOSE; }

        void SetDocument(Document* doc) { m_doc = doc; }

    private:
        void CloseDocument() const;

    private:
        DialogService& m_dialogService;

        Document* m_doc{ nullptr };
    };
}
