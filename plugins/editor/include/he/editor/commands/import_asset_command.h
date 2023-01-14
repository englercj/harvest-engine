// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/core/utils.h"
#include "he/editor/commands/command.h"
#include "he/editor/icons/icons_material_design.h"
#include "he/editor/services/dialog_service.h"
#include "he/editor/services/platform_service.h"
#include "he/editor/services/project_service.h"

namespace he::editor
{
    class Document;
    class ProgressDialog;

    class ImportAssetCommand : public Command
    {
    public:
        ImportAssetCommand(
            DialogService& dialogService,
            PlatformService& platformService,
            ProjectService& projectService) noexcept;

        bool CanRun() const override;
        void Run() override;

        const char* Label() const override { return "Import Asset..."; }
        const char* Icon() const override { return ICON_MDI_FILE_IMPORT; }

    private:
        DialogService& m_dialogService;
        PlatformService& m_platformService;
        ProjectService& m_projectService;
    };
}
