// Copyright Chad Engler

#include "asset_service.h"

#include "he/assets/asset_models.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/scope_guard.h"
#include "he/core/string_fmt.h"
#include "he/core/string_view_fmt.h"

namespace he::editor
{
    AssetService::AssetService(
        FileLoaderService& fileLoaderService,
        ProjectService& projectService,
        TaskService& taskService)
        : m_fileLoaderService(fileLoaderService)
        , m_projectService(projectService)
        , m_taskService(taskService)
        , m_db()
        , m_updater(m_db)
        , m_server(m_db, m_taskService)
    {}

    bool AssetService::Initialize()
    {
        m_projectService.OnLoad().Attach<&AssetService::OnProjectLoaded>(this);
        OnProjectLoaded();
        return true;
    }

    void AssetService::Terminate()
    {
        m_onDbReadyBinding.Detach();
        m_updater.Stop();

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

        String assetDbFile = m_projectService.DataDir();
        ConcatPath(assetDbFile, AssetDbFile);
        NormalizePath(assetDbFile);

        String assetRoot = GetDirectory(m_projectService.ProjectPath());
        ConcatPath(assetRoot, relativeAssetRoot);
        NormalizePath(assetRoot);

        auto failGuard = MakeScopeGuard([&]() { Terminate(); });

        if (!m_db.Initialize(assetDbFile.Data(), assetRoot.Data(), m_fileLoaderService.Loader()))
        {
            HE_LOG_ERROR(he_editor,
                HE_MSG("Failed to initialize asset cache DB."),
                HE_KV(asset_db_file, assetDbFile),
                HE_KV(asset_root, assetRoot));
            return;
        }

        m_onDbReadyBinding = m_updater.OnReady().Attach<&AssetService::OnDbReady>(this);

        if (!m_updater.Start())
        {
            HE_LOG_ERROR(he_editor,
                HE_MSG("Failed to start asset DB updater."),
                HE_KV(asset_db_file, assetDbFile),
                HE_KV(asset_root, assetRoot));
            return;
        }

        failGuard.Dismiss();
    }

    void AssetService::OnDbReady()
    {
        m_dbReady = true;

        TaskDelegate task = TaskDelegate::Make<&AssetService::ProcessPendingAssetsTask>(this);
        m_taskService.Add("Process pending assets", task);
    }

    void AssetService::ProcessPendingAssetsTask()
    {
        m_server.StartPendingImports();
        m_server.StartPendingCompiles();
    }
}
