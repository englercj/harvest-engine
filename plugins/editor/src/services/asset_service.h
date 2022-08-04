// Copyright Chad Engler

#pragma once

#include "file_loader_service.h"
#include "project_service.h"
#include "task_service.h"

#include "he/assets/asset_database.h"
#include "he/assets/asset_database_updater.h"
#include "he/assets/asset_server.h"
#include "he/assets/types.h"
#include "he/core/signal.h"
#include "he/core/types.h"

#include <atomic>

namespace he::editor
{
    constexpr const char AssetDbFile[] = "asset_cache.db";

    class AssetService
    {
    public:
        AssetService(
            FileLoaderService& fileLoaderService,
            ProjectService& projectService,
            TaskService& taskService);

        bool Initialize();
        void Terminate();

        void StartImport(const char* path) { m_server.StartImport(path); }
        void StartImport(const char* path, he::schema::Builder&& moreInfoBuilder) { m_server.StartImport(path, Move(moreInfoBuilder)); }

        void StartCompile(const assets::AssetUuid& assetUuid) { m_server.StartCompile(assetUuid); }

        assets::AssetDatabase& AssetDB() { return m_db; }
        const assets::AssetDatabase& AssetDB() const { return m_db; }

        bool IsAssetDBReady() const { return m_dbReady; }

    public:
        using ImportCompleteSignal = assets::AssetServer::ImportCompleteSignal;
        using CompileCompleteSignal = assets::AssetServer::CompileCompleteSignal;

        ImportCompleteSignal& OnImport() { return m_server.OnImport(); }
        CompileCompleteSignal& OnCompile() { return m_server.OnCompile(); }

    private:
        void OnProjectLoaded();
        void OnDbReady();

        void ProcessPendingAssetsTask();

    private:
        FileLoaderService& m_fileLoaderService;
        ProjectService& m_projectService;
        TaskService& m_taskService;

        assets::AssetDatabase m_db;
        assets::AssetDatabaseUpdater m_updater;
        assets::AssetServer m_server;

        assets::AssetDatabaseUpdater::OnReadySignal::Binding m_onDbReadyBinding{};

        std::atomic<bool> m_dbReady{ false };
    };
}
