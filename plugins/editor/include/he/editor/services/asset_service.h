// Copyright Chad Engler

#pragma once

#include "he/assets/asset_database.h"
#include "he/assets/asset_database_updater.h"
#include "he/assets/asset_server.h"
#include "he/assets/types.h"
#include "he/core/atomic.h"
#include "he/core/signal.h"
#include "he/core/types.h"
#include "he/editor/services/project_service.h"
#include "he/editor/services/task_service.h"

namespace he::editor
{
    class AssetService
    {
    public:
        struct ContentModule
        {
            editor::Plugin::Module::Reader mod;
            String rootPath;
        };

    public:
        AssetService(
            ProjectService& projectService,
            TaskService& taskService) noexcept;

        bool Initialize();
        void Terminate();

        void StartImport(const char* path) { m_server.StartImport(path); }
        void StartImport(const char* path, schema::Builder&& challengeBuilder) { m_server.StartImport(path, Move(challengeBuilder)); }

        void StartCompile(const assets::AssetUuid& assetUuid) { m_server.StartCompile(assetUuid); }

        assets::AssetDatabase& AssetDB() { return m_db; }
        const assets::AssetDatabase& AssetDB() const { return m_db; }

        bool IsAssetDBReady() const { return m_dbReady; }

        Span<const ContentModule> ContentModules() const { return m_contentModules; }

    public:
        using ImportCompleteSignal = assets::AssetServer::ImportCompleteSignal;
        using CompileCompleteSignal = assets::AssetServer::CompileCompleteSignal;
        using DbInitSignal = Signal<void(bool initialized)>;

        ImportCompleteSignal& OnImport() { return m_server.OnImport(); }
        CompileCompleteSignal& OnCompile() { return m_server.OnCompile(); }

    private:
        void OnDbReady();

        void ProcessPendingAssetsTask();

    private:

    private:
        ProjectService& m_projectService;
        TaskService& m_taskService;

        assets::AssetDatabase m_db;
        assets::AssetDatabaseUpdater m_updater;
        assets::AssetServer m_server;

        assets::AssetDatabaseUpdater::OnReadySignal::Binding m_onDbReadyBinding{};

        Vector<ContentModule> m_contentModules{};
        Atomic<bool> m_dbReady{ false };
    };
}
