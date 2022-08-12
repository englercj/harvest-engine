// Copyright Chad Engler

#pragma once

#include "command.h"
#include "fonts/icons_material_design.h"
#include "services/asset_service.h"
#include "services/dialog_service.h"
#include "services/platform_service.h"

#include "he/core/types.h"
#include "he/core/utils.h"

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
