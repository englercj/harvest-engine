// Copyright Chad Engler

#include "import_asset_command.h"

namespace he::editor
{
    ImportAssetCommand::ImportAssetCommand(
        AssetService& assetService,
        DialogService& dialogService,
        PlatformService& platformService) noexcept
        : m_assetService(assetService)
        , m_dialogService(dialogService)
        , m_platformService(platformService)
    {}

    bool ImportAssetCommand::CanRun() const
    {
        return true;
    }

    void ImportAssetCommand::Run()
    {
        FileDialogConfig config{};

        String path;
        if (m_platformService.OpenFileDialog(path, config))
        {
            m_assetService.StartImport(path.Data());
        }
    }
}
