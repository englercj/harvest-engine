// Copyright Chad Engler

#pragma once

#include "he/assets/asset_compiler.h"
#include "he/assets/asset_database.h"
#include "he/assets/asset_importer.h"
#include "he/assets/asset_models.h"
#include "he/assets/types.h"
#include "he/core/signal.h"
#include "he/core/task_executor.h"
#include "he/core/types.h"
#include "he/schema/layout.h"

namespace he::assets
{
    class AssetServer
    {
    public:
        AssetServer(AssetDatabase& db, TaskExecutor& executor);

        void StartImport(const char* path);
        void StartImport(const char* path, schema::Builder&& importSettings);
        void StartPendingImports();

        void StartCompile(const AssetUuid& assetUuid);
        void StartCompile(const AssetModel& asset);
        void StartPendingCompiles();

    public:
        using ImportCompleteSignal = Signal<void(ImportError error, const ImportContext& ctx, const ImportResult& result)>;
        using CompileCompleteSignal = Signal<void(bool success, const CompileContext& ctx, const CompileResult& result)>;

        ImportCompleteSignal& OnImport() { return m_onImportCompleteSignal; }
        CompileCompleteSignal& OnCompile() { return m_onCompileCompleteSignal; }

    private:
        struct ImportTaskData
        {
            ImportTaskData(AssetServer* server)
                : server(server)
                , ctx(server->m_db)
                , result(ctx, assetFile)
            {}

            String path{};
            schema::Builder importSettings{};
            schema::Builder assetFile{};

            AssetServer* server;
            ImportContext ctx;
            ImportResult result;
        };
        void StartImport(ImportTaskData* data);
        static void Import_OnAssetFileLoad(ImportTaskData* data, AssetDatabase::LoadResult result);
        static void Import_StartTask(ImportTaskData* data);
        static void Import_Task(ImportTaskData* data);

        struct CompileTaskData
        {
            CompileTaskData(AssetServer* server)
                : server(server)
                , ctx(server->m_db)
                , result(ctx)
            {}

            AssetUuid assetUuid{};

            AssetServer* server;
            CompileContext ctx;
            CompileResult result;
        };
        static void Compile_Task(CompileTaskData* data);

    private:
        AssetDatabase& m_db;
        TaskExecutor& m_executor;

        ImportCompleteSignal m_onImportCompleteSignal{};
        CompileCompleteSignal m_onCompileCompleteSignal{};
    };
}
