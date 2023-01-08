// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/core/utils.h"
#include "he/editor/commands/command.h"
#include "he/editor/icons/icons_material_design.h"
#include "he/editor/services/asset_service.h"
#include "he/editor/services/dialog_service.h"
#include "he/editor/services/platform_service.h"

namespace he::editor
{
    class Document;

    class ImportAssetCommand : public Command
    {
    public:
        ImportAssetCommand(
            AssetService& assetService,
            DialogService& dialogService,
            PlatformService& platformService) noexcept;

        bool CanRun() const override;
        void Run() override;

        const char* Label() const override { return "Import Asset..."; }
        const char* Icon() const override { return ICON_MDI_FILE_IMPORT; }

    private:
        AssetService& m_assetService;
        DialogService& m_dialogService;
        PlatformService& m_platformService;
    };
}
