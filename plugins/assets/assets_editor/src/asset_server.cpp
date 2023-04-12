// Copyright Chad Engler

#include "he/assets/asset_server.h"

#include "he/assets/asset_models.h"
#include "he/assets/asset_type_registry.h"
#include "he/assets/types_fmt.h"
#include "he/assets/types.h"
#include "he/assets/types_fmt.h"
#include "he/core/hash_table.h"
#include "he/core/log.h"
#include "he/core/module_registry.h"
#include "he/core/path.h"
#include "he/core/result_fmt.h"
#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/sqlite/orm.h"
#include "he/sqlite/transaction.h"

using namespace he::sqlite;

namespace he::assets
{
    extern Module* g_assetEditorModule;

    AssetServer::AssetServer(AssetDatabase& db, TaskExecutor& executor)
        : m_db(db)
        , m_executor(executor)
    {}

    void AssetServer::StartImport(const char* path)
    {
        ImportTaskData* data = Allocator::GetDefault().New<ImportTaskData>(this);
        data->path = path;
        data->ctx.file = data->path.Data();
        StartImport(data);
    }

    void AssetServer::StartImport(const char* path, schema::Builder&& importSettings)
    {
        ImportTaskData* data = Allocator::GetDefault().New<ImportTaskData>(this);
        data->path = path;
        data->importSettings = Move(importSettings);
        data->ctx.file = data->path.Data();
        data->ctx.settings = data->importSettings.Root().TryGetStruct();
        StartImport(data);
    }

    void AssetServer::StartImport(ImportTaskData* data)
    {
        HE_LOG_INFO(he_assets,
            HE_MSG("Starting asset import."),
            HE_KV(source_path, data->path),
            HE_KV(has_import_settings, data->ctx.settings.IsValid()));

        AssetFileModel model;
        if (m_db.Storage().FindOne(model, Where(Col(&AssetFileModel::sourcePath) == data->path)))
        {
            AssetDatabase::LoadDelegate cb = AssetDatabase::LoadDelegate::Make<&AssetServer::Import_OnAssetFileLoad>(data);
            m_db.LoadAssetFileAsync(model.filePath.Data(), cb);
        }
        else
        {
            Import_StartTask(data);
        }
    }

    void AssetServer::StartPendingImports()
    {
        Vector<AssetModel> pending;

        if (!m_db.Storage().FindAll(pending, Where(Col(&AssetModel::state) == AssetState::NeedsImport)))
        {
            HE_LOG_ERROR(he_editor, HE_MSG("Failed to query assets needing import."));
        }

        if (!m_db.Storage().FindAll(pending, Where(Col(&AssetModel::state) == AssetState::ImportFailed)))
        {
            HE_LOG_ERROR(he_editor, HE_MSG("Failed to query assets where importing had previously failed."));
        }

        HashSet<uint32_t> fileIds;

        for (const AssetModel& asset : pending)
        {
            const auto result = fileIds.Insert(asset.assetFileId);
            if (result.inserted)
            {
                AssetFileModel assetFile;
                if (m_db.Storage().FindOne(assetFile, Where(Col(&AssetFileModel::id) == asset.assetFileId)))
                {
                    StartImport(assetFile.sourcePath.Data());
                }
            }
        }
    }

    void AssetServer::StartCompile(const AssetUuid& assetUuid)
    {
        AssetModel asset;
        if (!m_db.Storage().FindOne(asset, Where(Col(&AssetModel::uuid) == assetUuid)))
        {
            HE_LOG_ERROR(he_editor, HE_MSG("Failed to find asset for compile request."), HE_KV(asset_uuid, assetUuid));
            return;
        }

        StartCompile(asset);
    }

    void AssetServer::StartCompile(const AssetModel& asset)
    {
        CompileTaskData* data = Allocator::GetDefault().New<CompileTaskData>(this);
        data->assetUuid = asset.uuid;
        data->server = this;

        String taskName = "Compiling asset '";
        taskName += asset.name;
        taskName += "'";

        TaskDelegate job = TaskDelegate::Make<&AssetServer::Compile_Task>(data);
        m_executor.Add(taskName.Data(), job);
    }

    void AssetServer::StartPendingCompiles()
    {
        Vector<AssetModel> pending;

        if (!m_db.Storage().FindAll(pending, Where(Col(&AssetModel::state) == AssetState::NeedsCompile)))
        {
            HE_LOG_ERROR(he_editor, HE_MSG("Failed to query assets needing compilation."));
        }

        if (!m_db.Storage().FindAll(pending, Where(Col(&AssetModel::state) == AssetState::CompileFailed)))
        {
            HE_LOG_ERROR(he_editor, HE_MSG("Failed to query assets where compilation had previously failed."));
        }

        for (const AssetModel& asset : pending)
        {
            StartCompile(asset.uuid);
        }
    }

    void AssetServer::Import_OnAssetFileLoad(ImportTaskData* data, AssetDatabase::LoadResult load)
    {
        if (load.result)
        {
            data->assetFile = Move(load.builder.builder);
            data->ctx.assetFile = data->assetFile.Root().TryGetStruct<AssetFile>();

            HE_LOG_INFO(he_assets,
                HE_MSG("Using existing asset file for import."),
                HE_KV(source_path, data->path),
                HE_KV(asset_file_uuid, AssetFileUuid(data->ctx.assetFile.GetUuid())));
        }
        else if (GetFileResult(load.result) != FileResult::NotFound)
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to load existing asset file during import."),
                HE_KV(source_path, data->path),
                HE_KV(result, load.result));

            data->server->m_onImportCompleteSignal.Dispatch(ImportError::Failure, data->ctx, data->result);
            Allocator::GetDefault().Delete(data);
            return;
        }

        Import_StartTask(data);
    }

    void AssetServer::Import_StartTask(ImportTaskData* data)
    {
        String taskName = "Importing file '";
        taskName += GetBaseName(data->path);
        taskName += "'";

        TaskDelegate job = TaskDelegate::Make<&AssetServer::Import_Task>(data);
        data->server->m_executor.Add(taskName.Data(), job);
    }

    void AssetServer::Import_Task(ImportTaskData* data)
    {
        const AssetTypeRegistry& registry = g_assetEditorModule->Registry().GetApi<AssetTypeRegistry>();

        if (data->assetFile.Root().IsNull())
        {
            AssetFile::Builder assetFile = data->assetFile.AddStruct<AssetFile>();
            FillUuidV4(assetFile.InitUuid());
            assetFile.InitSource(data->path);

            data->assetFile.SetRoot(assetFile);
            data->ctx.assetFile = assetFile;

            HE_LOG_INFO(he_assets,
                HE_MSG("Creating new asset file for import."),
                HE_KV(source_path, data->path),
                HE_KV(asset_file_uuid, AssetFileUuid(data->ctx.assetFile.GetUuid())));
        }

        bool found = false;
        ImportError error = ImportError::Success;

        // Run through each importer and attempt to import the file.
        for (const AssetTypeRegistry::ImporterEntry& entry : registry.Importers())
        {
            AssetImporter& importer = *entry.importer;

            if (importer.CanImport(data->ctx.file))
            {
                HE_LOG_DEBUG(he_assets,
                    HE_MSG("Importing asset source."),
                    HE_KV(importer_id, importer.Id()),
                    HE_KV(importer_version, importer.Version()),
                    HE_KV(asset_file_uuid, AssetFileUuid(data->ctx.assetFile.GetUuid())),
                    HE_KV(source_path, data->ctx.file));

                found = true;
                error = importer.Import(data->ctx, data->result);

                if (error == ImportError::Failure)
                {
                    HE_LOG_ERROR(he_assets,
                        HE_MSG("Failed to import asset source file."),
                        HE_KV(importer_id, importer.Id()),
                        HE_KV(importer_version, importer.Version()),
                        HE_KV(asset_file_uuid, AssetFileUuid(data->ctx.assetFile.GetUuid())),
                        HE_KV(source_path, data->path));
                }

                break;
            }
        }

        if (!found)
        {
            HE_LOG_WARN(he_assets,
                HE_MSG("No importer found that can handle this asset source."),
                HE_KV(asset_file_uuid, AssetFileUuid(data->ctx.assetFile.GetUuid())),
                HE_KV(source_path, data->path));
        }

        Vector<Asset::Reader> needsCompile;

        if (error == ImportError::Success)
        {
            String assetFilePath = data->path;
            assetFilePath += AssetFileExtension;

            AssetFile::Builder assetFile = data->assetFile.Root().TryGetStruct<AssetFile>();

            const uint32_t prevSize = assetFile.GetAssets().Size();
            const uint32_t newListSize = prevSize + data->result.m_new.Size();
            schema::List<Asset>::Builder assets = data->assetFile.AddList<Asset>(newListSize);

            // copy over existing and updated assets, preserving the order.
            for (uint32_t i = 0; i < prevSize; ++i)
            {
                Asset::Reader asset = assetFile.GetAssets()[i];

                bool foundUpdated = false;
                for (auto&& updated : data->result.m_updated)
                {
                    if (asset.GetUuid() == updated.GetUuid())
                    {
                        foundUpdated = true;
                        assets.Set(i, updated);
                    }
                }

                if (!foundUpdated)
                {
                    assets.Set(i, asset);
                }
            }

            // copy over new assets
            for (uint32_t i = prevSize; i < newListSize; ++i)
            {
                Asset::Reader asset = data->result.m_new[i - prevSize];
                assets.Set(i, asset);
            }

            assetFile.SetAssets(assets);

            bool saved = data->server->m_db.SaveAssetFile(assetFilePath.Data(), assetFile);
            if (!saved)
            {
                HE_LOG_ERROR(he_assets,
                    HE_MSG("Failed to save asset file after import."),
                    HE_KV(asset_file_path, assetFilePath),
                    HE_KV(asset_file_uuid, AssetFileUuid(assetFile.GetUuid())),
                    HE_KV(source_path, data->path));
                error = ImportError::Failure;
            }
        }

        Transaction t(data->server->m_db.Handle());
        for (const Asset::Reader asset : data->ctx.assetFile.GetAssets())
        {
            const AssetUuid assetUuid{ asset.GetUuid() };

            if (error == ImportError::Success)
            {
                const auto query = Update(
                    Where(Col(&AssetModel::uuid) == assetUuid),
                    Set(&AssetModel::state, AssetState::NeedsCompile));
                data->server->m_db.Storage().Query(query);
                data->server->StartCompile(assetUuid);
            }
            else
            {
                const auto query = Update(
                    Where(Col(&AssetModel::uuid) == assetUuid),
                    Set(&AssetModel::state, AssetState::ImportFailed));
                data->server->m_db.Storage().Query(query);
            }
        }
        t.Commit();

        data->server->m_onImportCompleteSignal.Dispatch(error, data->ctx, data->result);
        Allocator::GetDefault().Delete(data);
    }

    void AssetServer::Compile_Task(CompileTaskData* data)
    {
        bool success = false;

        AssetModel model;
        if (AssetModel::FindOne(data->server->m_db, data->assetUuid, model))
        {
            const AssetTypeId typeId{ model.assetTypeName };
            const AssetTypeRegistry& registry = g_assetEditorModule->Registry().GetApi<AssetTypeRegistry>();
            const AssetTypeRegistry::Entry* assetType = registry.FindAssetType(typeId);
            const AssetUuid assetUuid{ data->ctx.asset.GetUuid() };

            if (assetType)
            {
                success = assetType->compiler->Compile(data->ctx, data->result);

                if (success)
                {
                    HE_LOG_DEBUG(he_assets,
                        HE_MSG("Compiling asset."),
                        HE_KV(compiler_id, assetType->compiler->Id()),
                        HE_KV(compiler_version, assetType->compiler->Version()),
                        HE_KV(asset_uuid, assetUuid),
                        HE_KV(asset_type, model.assetTypeName));

                    const auto query = Update(
                        Where(Col(&AssetModel::uuid) == assetUuid),
                        Set(&AssetModel::state, AssetState::Ready));
                    data->server->m_db.Storage().Query(query);
                }
                else
                {
                    HE_LOG_ERROR(he_assets,
                        HE_MSG("Failed to compile asset."),
                        HE_KV(compiler_id, assetType->compiler->Id()),
                        HE_KV(compiler_version, assetType->compiler->Version()),
                        HE_KV(asset_uuid, assetUuid),
                        HE_KV(asset_type, model.assetTypeName));

                    const auto query = Update(
                        Where(Col(&AssetModel::uuid) == assetUuid),
                        Set(&AssetModel::state, AssetState::CompileFailed));
                    data->server->m_db.Storage().Query(query);
                }
            }
            else
            {
                HE_LOG_WARN(he_assets,
                    HE_MSG("Unknown asset type, no such type has been registered by any modules."),
                    HE_KV(asset_uuid, assetUuid),
                    HE_KV(asset_type, model.assetTypeName));
            }
        }
        else
        {
            HE_LOG_WARN(he_assets,
                HE_MSG("No asset found with that ID. Is the Asset cache database up-to-date?"),
                HE_KV(asset_uuid, AssetUuid(data->ctx.asset.GetUuid())));
        }

        data->server->m_onCompileCompleteSignal.Dispatch(success, data->ctx, data->result);
        Allocator::GetDefault().Delete(data);
    }
}
