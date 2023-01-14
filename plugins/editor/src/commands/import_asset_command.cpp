// Copyright Chad Engler

#include "he/editor/commands/import_asset_command.h"

#include "he/editor/dialogs/import_asset_dialog.h"

namespace he::editor
{
    ImportAssetCommand::ImportAssetCommand(
        DialogService& dialogService,
        PlatformService& platformService,
        ProjectService& projectService) noexcept
        : m_dialogService(dialogService)
        , m_platformService(platformService)
        , m_projectService(projectService)
    {}

    bool ImportAssetCommand::CanRun() const
    {
        return m_projectService.IsOpen();
    }

    void ImportAssetCommand::Run()
    {
        FileDialogConfig config{};

        String path;
        if (m_platformService.OpenFileDialog(path, config))
        {
            m_dialogService.Open<ImportAssetDialog>().Configure(path.Data());
        }
    }
}
