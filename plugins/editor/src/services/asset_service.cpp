// Copyright Chad Engler

#include "asset_service.h"

#include "he/core/log.h"
#include "he/core/path.h"

namespace he::editor
{
    AssetService::AssetService(
        FileLoaderService& fileLoaderService,
        ProjectService& projectService)
        : m_fileLoaderService(fileLoaderService)
        , m_projectService(projectService)
    {}

    bool AssetService::Initialize()
    {
        const char* assetRoot = m_projectService.GetProject().GetAssetRoot().Data();

        String resourcesDir = m_projectService.GetResourceDir();
        ConcatPath(resourcesDir, AssetDbFile);

        if (!m_db.Initialize(resourcesDir.Data(), assetRoot, m_fileLoaderService.Loader()))
        {
            HE_LOGF_ERROR(he_editor, "Failed to initialize asset cache DB.");
            return false;
        }

        m_updater = assets::AssetDatabaseUpdater::Create(m_db);
        if (!m_updater->Start())
        {
            HE_LOGF_ERROR(he_editor, "Failed to start asset DB updater.");
            return false;
        }

        return true;
    }

    void AssetService::Terminate()
    {
        if (m_updater)
        {
            m_updater->Stop();
            assets::AssetDatabaseUpdater::Destroy(m_updater);
        }
    }
}
