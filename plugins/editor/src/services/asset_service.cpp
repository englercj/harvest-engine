// Copyright Chad Engler

#include "asset_service.h"

#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/scope_guard.h"
#include "he/core/string_fmt.h"
#include "he/core/string_view_fmt.h"

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
        m_projectService.OnLoad().Attach<&AssetService::OnProjectLoaded>(this);
        return true;
    }

    void AssetService::Terminate()
    {
        if (m_updater)
        {
            m_updater->Stop();
            assets::AssetDatabaseUpdater::Destroy(m_updater);
            m_updater = nullptr;
        }

        m_db.Terminate();
    }

    void AssetService::OnProjectLoaded()
    {
        Terminate();

        if (!m_projectService.IsOpen())
            return;

        const he::schema::String::Reader relativeAssetRoot = m_projectService.Project().GetAssetRoot();

        if (!relativeAssetRoot.IsValid() || relativeAssetRoot.IsEmpty())
            return;

        String resourcesDir = m_projectService.ResourceDir();
        ConcatPath(resourcesDir, AssetDbFile);
        NormalizePath(resourcesDir);

        String assetRoot = m_projectService.ProjectPath();
        ConcatPath(assetRoot, relativeAssetRoot);
        NormalizePath(assetRoot);

        auto failGuard = MakeScopeGuard([&]() { Terminate(); });

        if (!m_db.Initialize(resourcesDir.Data(), assetRoot.Data(), m_fileLoaderService.Loader()))
        {
            HE_LOG_ERROR(he_editor,
                HE_MSG("Failed to initialize asset cache DB."),
                HE_KV(resources_dir, resourcesDir),
                HE_KV(asset_root, assetRoot));
            return;
        }

        m_updater = assets::AssetDatabaseUpdater::Create(m_db);
        if (!m_updater->Start())
        {
            HE_LOG_ERROR(he_editor,
                HE_MSG("Failed to start asset DB updater."),
                HE_KV(resources_dir, resourcesDir),
                HE_KV(asset_root, assetRoot));
            return;
        }

        failGuard.Dismiss();
    }
}
