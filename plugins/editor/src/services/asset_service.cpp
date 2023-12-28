// Copyright Chad Engler

#include "he/editor/services/asset_service.h"

#include "he/assets/asset_models.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/scope_guard.h"
#include "he/core/string_fmt.h"

namespace he::editor
{
    AssetService::AssetService(
        ProjectService& projectService,
        TaskService& taskService) noexcept
        : m_projectService(projectService)
        , m_taskService(taskService)
        , m_db()
        , m_updater(m_db)
        , m_server(m_db, m_taskService)
    {}

    bool AssetService::Initialize()
    {
        if (!HE_VERIFY(m_projectService.IsOpen(),
            HE_MSG("A project must be open for the asset service to initialize.")))
        {
            return false;
        }

        auto failGuard = MakeScopeGuard([&]() { Terminate(); });

        // Collect the content modules & root directories
        Vector<String> contentRoots;
        for (const ProjectService::PluginEntry& entry : m_projectService.Plugins())
        {
            const StringView pluginDir = GetDirectory(entry.filePath);
            for (const editor::Plugin::Module::Reader mod : entry.plugin.Root().GetModules())
            {
                if (mod.GetType() != editor::Plugin::ModuleType::Content || mod.GetContentDir().IsEmpty())
                    continue;

                ContentModule& content = m_contentModules.EmplaceBack();
                content.mod = mod;
                content.rootPath = pluginDir;
                ConcatPath(content.rootPath, mod.GetContentDir());
                NormalizePath(content.rootPath);

                contentRoots.PushBack(content.rootPath);
            }
        }

        // Initialize the database
        const String cacheRoot = m_projectService.DataDir();
        if (!m_db.Initialize(cacheRoot.Data(), contentRoots))
        {
            HE_LOG_ERROR(he_editor,
                HE_MSG("Failed to initialize asset cache DB."),
                HE_KV(cache_root, cacheRoot),
                HE_KV(content_root_count, contentRoots.Size()));
            return false;
        }

        // Startup the updater to ensure the DB is correct
        m_onDbReadyBinding = m_updater.OnReady().Attach<&AssetService::OnDbReady>(this);

        if (!m_updater.Start())
        {
            HE_LOG_ERROR(he_editor,
                HE_MSG("Failed to start asset DB updater."),
                HE_KV(cache_root, cacheRoot),
                HE_KV(content_root_count, contentRoots.Size()));
            return false;
        }

        failGuard.Dismiss();
        return true;
    }

    void AssetService::Terminate()
    {
        m_onDbReadyBinding.Detach();
        m_updater.Stop();

        m_db.Terminate();
        m_dbReady = false;
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
