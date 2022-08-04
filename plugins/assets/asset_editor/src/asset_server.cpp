// Copyright Chad Engler

#include "he/assets/asset_server.h"

#include "he/assets/asset_models.h"
#include "he/assets/asset_type_registry.h"
#include "he/assets/types_fmt.h"
#include "he/assets/types.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/result_fmt.h"
#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"

#include <unordered_set>

namespace he::assets
{
    AssetServer::AssetServer(AssetDatabase& db, TaskExecutor& executor)
        : m_db(db)
        , m_executor(executor)
    {}

    void AssetServer::StartImport(const char* path)
    {
        ImportTaskData* data = Allocator::GetDefault().New<ImportTaskData>();
        data->path = path;
        data->server = this;

        AssetDatabase::LoadDelegate cb = AssetDatabase::LoadDelegate::Make<&AssetServer::Import_OnAssetFileLoad>(data);
        m_db.LoadAssetFile(path, cb);
    }

    void AssetServer::StartImport(const char* path, he::schema::Builder&& moreInfoBuilder)
    {
        ImportTaskData* data = Allocator::GetDefault().New<ImportTaskData>();
        data->path = path;
        data->ctx.moreInfoBuilder = Move(moreInfoBuilder);
        data->server = this;

        AssetDatabase::LoadDelegate cb = AssetDatabase::LoadDelegate::Make<&AssetServer::Import_OnAssetFileLoad>(data);
        m_db.LoadAssetFile(path, cb);
    }

    void AssetServer::StartPendingImports()
    {
        Vector<AssetModel> pending;

        if (!AssetModel::FindAll(m_db, AssetState::NeedsImport, pending))
        {
            HE_LOG_ERROR(he_editor, HE_MSG("Failed to query assets needing import."));
        }

        if (!AssetModel::FindAll(m_db, AssetState::ImportFailed, pending))
        {
            HE_LOG_ERROR(he_editor, HE_MSG("Failed to query assets needing import."));
        }

        std::unordered_set<AssetFileUuid> fileUuids;

        for (const AssetModel& asset : pending)
        {
            const auto pair = fileUuids.insert(asset.fileUuid);
            if (pair.second)
            {
                AssetFileModel assetFile;
                if (AssetFileModel::FindOne(m_db, asset.fileUuid, assetFile))
                {
                    StartImport(assetFile.file.path.Data());
                }
            }
        }
    }

    void AssetServer::StartCompile(const AssetUuid& assetUuid)
    {
        AssetModel asset;
        if (!AssetModel::FindOne(m_db, assetUuid, asset))
        {
            HE_LOG_ERROR(he_editor, HE_MSG("Failed to query assets needing import."));
            return;
        }

        StartCompile(asset);
    }

    void AssetServer::StartCompile(const AssetModel& asset)
    {
        CompileTaskData* data = Allocator::GetDefault().New<CompileTaskData>();
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

        if (!AssetModel::FindAll(m_db, AssetState::NeedsImport, pending))
        {
            HE_LOG_ERROR(he_editor, HE_MSG("Failed to query assets needing import."));
        }

        if (!AssetModel::FindAll(m_db, AssetState::ImportFailed, pending))
        {
            HE_LOG_ERROR(he_editor, HE_MSG("Failed to query assets needing import."));
        }

        for (const AssetModel& asset : pending)
        {
            StartCompile(asset.uuid);
        }
    }

    void AssetServer::Import_OnAssetFileLoad(ImportTaskData* data, AssetDatabase::LoadResult load)
    {
        if (!load.result)
        {
            if (GetFileResult(load.result) != FileResult::NotFound)
            {
                HE_LOG_ERROR(he_assets,
                    HE_MSG("Failed to load existing asset file during import."),
                    HE_KV(path, data->path),
                    HE_KV(result, load.result));

                data->server->m_onImportCompleteSignal.Dispatch(false, data->ctx, data->result);
                Allocator::GetDefault().Delete(data);
                return;
            }

            schema::AssetFile::Builder assetFile = data->ctx.assetFile.Root();
            FillUuidV4(assetFile.InitUuid());
            assetFile.InitSource(data->path);
        }

        data->ctx.file = data->path.Data();
        data->ctx.assetFile = Move(load.builder);

        String taskName = "Importing file '";
        taskName += GetBaseName(data->path);
        taskName += "'";

        TaskDelegate job = TaskDelegate::Make<&AssetServer::Import_Task>(data);
        data->server->m_executor.Add(taskName.Data(), job);
    }

    void AssetServer::Import_Task(ImportTaskData* data)
    {
        AssetTypeRegistry& registry = AssetTypeRegistry::Get();

        bool found = false;
        bool success = false;

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
                    HE_KV(asset_file_uuid, AssetFileUuid(data->ctx.assetFile.Root().GetUuid())),
                    HE_KV(path, data->ctx.file));

                found = true;
                success = importer.Import(data->ctx, data->result);

                if (!success)
                {
                    HE_LOG_ERROR(he_assets,
                        HE_MSG("Failed to import asset source."),
                        HE_KV(importer_id, importer.Id()),
                        HE_KV(importer_version, importer.Version()),
                        HE_KV(asset_file_uuid, AssetFileUuid(data->ctx.assetFile.Root().GetUuid())),
                        HE_KV(path, data->path));
                    break;
                }
            }
        }

        if (!found)
        {
            HE_LOG_WARN(he_assets, HE_MSG("No importer found that can handle this asset source."), HE_KV(path, data->path));
        }

        // TODO: Only compile the touched stuff instead of everything.
        // Maybe only if there are new resources or data has changed?
        if (success)
        {
            String assetFilePath = data->path;
            assetFilePath += AssetFileExtension;

            success = data->server->m_db.SaveAssetFile(assetFilePath.Data(), data->ctx.assetFile.Root());
            if (!success)
            {
                HE_LOG_ERROR(he_assets,
                    HE_MSG("Failed to save asset file after import."),
                    HE_KV(asset_file_path, assetFilePath),
                    HE_KV(asset_file_uuid, AssetFileUuid(data->ctx.assetFile.Root().GetUuid())),
                    HE_KV(source_path, data->path));
            }

        }

        sqlite::Transaction t = data->server->m_db.BeginTransaction();
        for (const schema::Asset::Builder asset : data->ctx.assetFile.Root().GetAssets())
        {
            AssetUuid assetUuid{ asset.GetUuid() };

            if (success)
            {
                AssetModel::UpdateState(data->server->m_db, assetUuid, AssetState::NeedsCompile);
                data->server->StartCompile(assetUuid);
            }
            else
            {
                AssetModel::UpdateState(data->server->m_db, assetUuid, AssetState::ImportFailed);
            }
        }
        t.Commit();

        data->server->m_onImportCompleteSignal.Dispatch(success, data->ctx, data->result);
        Allocator::GetDefault().Delete(data);
    }


    void AssetServer::Compile_Task(CompileTaskData* data)
    {
        bool success = false;

        AssetModel model;
        if (AssetModel::FindOne(data->server->m_db, data->assetUuid, model))
        {
            const AssetTypeId typeId{ model.type };
            AssetCompiler* compiler = AssetTypeRegistry::Get().FindCompiler(typeId);
            if (compiler)
            {
                success = compiler->Compile(data->ctx, data->result);

                if (success)
                {
                    HE_LOG_DEBUG(he_assets,
                        HE_MSG("Compiling asset."),
                        HE_KV(compiler_id, compiler->Id()),
                        HE_KV(compiler_version, compiler->Version()),
                        HE_KV(asset_uuid, AssetUuid(data->ctx.asset.GetUuid())),
                        HE_KV(asset_type, model.type));
                    AssetModel::UpdateState(data->server->m_db, data->ctx.asset.GetUuid(), AssetState::Ready);
                }
                else
                {
                    HE_LOG_ERROR(he_assets,
                        HE_MSG("Failed to compile asset."),
                        HE_KV(compiler_id, compiler->Id()),
                        HE_KV(compiler_version, compiler->Version()),
                        HE_KV(asset_uuid, AssetUuid(data->ctx.asset.GetUuid())),
                        HE_KV(asset_type, model.type));
                    AssetModel::UpdateState(data->server->m_db, data->ctx.asset.GetUuid(), AssetState::CompileFailed);
                }
            }
            else
            {
                HE_LOG_WARN(he_assets,
                    HE_MSG("No compiler found that can handle this asset type."),
                    HE_KV(asset_uuid, AssetUuid(data->ctx.asset.GetUuid())),
                    HE_KV(asset_type, model.type));
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
