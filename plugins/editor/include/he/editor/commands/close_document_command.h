// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/core/unique_ptr.h"
#include "he/editor/commands/command.h"
#include "he/editor/icons/icons_material_design.h"
#include "he/editor/services/dialog_service.h"

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
