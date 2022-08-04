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
        void StartImport(const char* path, he::schema::Builder&& moreInfoBuilder);
        void StartPendingImports();

        void StartCompile(const AssetUuid& assetUuid);
        void StartCompile(const AssetModel& asset);
        void StartPendingCompiles();

    public:
        using ImportCompleteSignal = Signal<void(bool success, const ImportContext& ctx, const ImportResult& result)>;
        using CompileCompleteSignal = Signal<void(bool success, const CompileContext& ctx, const CompileResult& result)>;

        ImportCompleteSignal& OnImport() { return m_onImportCompleteSignal; }
        CompileCompleteSignal& OnCompile() { return m_onCompileCompleteSignal; }

    private:
        struct ImportTaskData
        {
            String path;
            ImportContext ctx;
            ImportResult result;
            AssetServer* server;
        };
        static void Import_OnAssetFileLoad(ImportTaskData* data, AssetDatabase::LoadResult result);
        static void Import_Task(ImportTaskData* data);

        struct CompileTaskData
        {
            AssetUuid assetUuid;
            CompileContext ctx;
            CompileResult result;
            AssetServer* server;
        };
        static void Compile_Task(CompileTaskData* data);

    private:
        AssetDatabase& m_db;
        TaskExecutor& m_executor;

        ImportCompleteSignal m_onImportCompleteSignal{};
        CompileCompleteSignal m_onCompileCompleteSignal{};
    };
}
