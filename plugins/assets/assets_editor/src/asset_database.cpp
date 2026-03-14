// Copyright Chad Engler

#include "he/assets/asset_database.h"

#include "he/assets/asset_models.h"
#include "he/assets/types.h"
#include "he/assets/types_fmt.h"
#include "he/core/async_file.h"
#include "he/core/clock.h"
#include "he/core/clock_fmt.h"
#include "he/core/directory.h"
#include "he/core/fmt.h"
#include "he/core/file.h"
#include "he/core/limits.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/stopwatch.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/core/string_ops.h"
#include "he/schema/toml.h"

using namespace he::sqlite;

namespace he::assets
{
    static const char AssetDbStartupSql[] = R"(
        PRAGMA automatic_index = true;
        PRAGMA encoding = 'UTF-8';
        PRAGMA foreign_keys = true;
        PRAGMA journal_mode = WAL;
        PRAGMA page_size = 4096;
        PRAGMA recursive_triggers = true;
        PRAGMA synchronous = normal;
        PRAGMA temp_store = memory;
        VACUUM;
    )";

    AssetDatabase::AssetDatabase()
        : m_storage(AssetDbSchema)
    {}

    bool AssetDatabase::Initialize(StringView cacheRoot, Span<String> contentRoots)
    {
        // Collect the content root paths
        for (const String& root : contentRoots)
        {
            if (!HE_VERIFY(IsAbsolutePath(root),
                HE_MSG("Content root path must be absolute."),
                HE_KV(content_root, root)))
            {
                return false;
            }

            m_contentRoots.EmplaceBack(root);
        }

        // Ensure the cache root, and the resources directory both exist
        m_resourceRoot = cacheRoot;
        ConcatPath(m_resourceRoot, "resources");
        if (!HE_VERIFY(Directory::Create(m_resourceRoot.Data(), true),
            HE_MSG("Failed to create resources directory."),
            HE_KV(resource_root, m_resourceRoot)))
        {
            return false;
        }

        HE_LOG_INFO(he_assets,
            HE_MSG("Initializing asset database"),
            HE_KV(cache_root, cacheRoot),
            HE_KV(content_root_count, m_contentRoots.Size()));

        Stopwatch timer;

        // Open the sqlite database and sync the schema
        String dbPath = cacheRoot;
        ConcatPath(dbPath, "asset_cache.db");
        if (!m_storage.Open(dbPath.Data()))
            return false;

        if (!m_storage.Db().Execute(AssetDbStartupSql))
            return false;

        if (!m_storage.Sync())
            return false;

        HE_LOG_INFO(he_assets,
            HE_MSG("Asset database initialization complete"),
            HE_KV(time, timer.Elapsed()));

        return true;
    }

    bool AssetDatabase::Terminate()
    {
        return m_storage.Close();
    }

    bool AssetDatabase::IsFileUpToDate(const char* path)
    {
        if (!HE_VERIFY(IsAbsolutePath(path),
            HE_MSG("The AssetDatabase can only handle absolute paths."),
            HE_KV(file_path, path)))
        {
            return false;
        }

        AssetFileModel model;
        if (!m_storage.FindOne(model, Where(Col(&AssetFileModel::filePath) == path)))
            return false;

        // Check the asset file's attributes to see if it has changed since last time we've scanned it
        FileAttributes attributes;
        Result res = File::GetAttributes(path, attributes);
        if (!res)
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to read asset file attributes. Assuming update is required."),
                HE_KV(path, path),
                HE_KV(result, res));
            return false;
        }

        const uint32_t fileSize = static_cast<uint32_t>(attributes.size);
        if (model.fileSize != fileSize)
        {
            HE_LOG_DEBUG(he_assets,
                HE_MSG("Asset file size has changed, update required."),
                HE_KV(path, path),
                HE_KV(last_file_size, model.fileSize),
                HE_KV(file_size, fileSize));
            return false;
        }

        if (model.fileWriteTime != attributes.writeTime)
        {
            HE_LOG_DEBUG(he_assets,
                HE_MSG("Asset file write time has changed, update required."),
                HE_KV(path, path),
                HE_KV(last_write_time, model.fileWriteTime),
                HE_KV(write_time, attributes.writeTime));
            return false;
        }

        // Check the asset file's source to see if it has changed since last time we've scanned it
        // If this is true, we probably want to re-run the importer to ensure import resources exist.
        if (!model.sourcePath.IsEmpty())
        {
            // TODO: Modify for when we support sparse checkout of source files.
            res = File::GetAttributes(model.sourcePath.Data(), attributes);
            if (!res)
            {
                HE_LOG_ERROR(he_assets,
                    HE_MSG("Failed to read source file attributes. Assuming update is not required."),
                    HE_KV(path, path),
                    HE_KV(result, res));
                return true;
            }

            const uint32_t sourceFileSize = static_cast<uint32_t>(attributes.size);
            if (model.sourceSize != sourceFileSize)
            {
                HE_LOG_DEBUG(he_assets,
                    HE_MSG("Source file size has changed, update required."),
                    HE_KV(path, path),
                    HE_KV(last_file_size, model.sourceSize),
                    HE_KV(file_size, sourceFileSize));
                return false;
            }

            if (model.sourceWriteTime != attributes.writeTime)
            {
                HE_LOG_DEBUG(he_assets,
                    HE_MSG("Source file write time has changed, update required."),
                    HE_KV(path, path),
                    HE_KV(last_write_time, model.sourceWriteTime),
                    HE_KV(write_time, attributes.writeTime));
                return false;
            }
        }

        // Nothing has changed, good to go.
        return true;
    }

    bool AssetDatabase::UpdateAssetFileAsync(const char* path, LoadDelegate callback)
    {
        if (!HE_VERIFY(IsAbsolutePath(path),
            HE_MSG("The AssetDatabase can only handle absolute paths."),
            HE_KV(file_path, path)))
        {
            return false;
        }

        UpdateRequest* req = Allocator::GetDefault().New<UpdateRequest>();
        req->db = this;
        req->path = path;
        req->callback = callback;

        return LoadAssetFileAsync(path, LoadDelegate::Make<&HandleLoadForUpdateComplete>(req));
    }

    bool AssetDatabase::LoadAssetFileAsync(const char* path, LoadDelegate callback)
    {
        if (!HE_VERIFY(callback))
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("LoadAssetFileAsync called without a callback, ignoring load request."),
                HE_KV(file_path, path));
            return false;
        }

        if (!HE_VERIFY(IsAbsolutePath(path),
            HE_MSG("The AssetDatabase can only handle absolute paths."),
            HE_KV(file_path, path)))
        {
            return false;
        }

        AsyncFile file;
        const Result r = file.Open(path, FileAccessMode::Read, FileCreateMode::OpenExisting, FileOpenFlag::SequentialScan);
        if (!r)
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to open asset file."),
                HE_KV(file_path, path),
                HE_KV(result, r));

            callback({ r });
            return false;
        }

        const uint64_t size = file.GetSize();
        if (size > Limits<uint32_t>::Max)
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Asset file size is larger than UINT32_MAX"),
                HE_KV(file_size, size),
                HE_KV(file_path, path));

            callback({ Result::InvalidParameter });
            return false;
        }

        const uint32_t fileByteSize = static_cast<uint32_t>(size);

        LoadRequest* load = Allocator::GetDefault().New<LoadRequest>();
        load->path = path;
        load->content.Resize(fileByteSize, DefaultInit);
        load->db = this;
        load->callback = callback;

        file.ReadAsync(load->content.Data(), 0, fileByteSize, AsyncFile::CompleteDelegate::Make([](const void* l, AsyncFileOp token)
        {
            const Result rc = AsyncFile::GetResult(token);
            HandleFileReadComplete(static_cast<LoadRequest*>(const_cast<void*>(l)), rc);
        }, load));

        file.Close();
        return true;
    }

    bool AssetDatabase::LoadAssetFileAsync(const AssetFileUuid& fileUuid, LoadDelegate callback)
    {
        if (!HE_VERIFY(callback))
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("LoadAssetFile called without a callback, ignoring load request."),
                HE_KV(file_uuid, fileUuid));
            return false;
        }

        AssetFileModel model;
        if (!m_storage.FindOne(model, Where(Col(&AssetFileModel::uuid) == fileUuid)))
        {
            callback({ Result::InvalidParameter });
            return false;
        }

        return LoadAssetFileAsync(model.filePath.Data(), Move(callback));
    }

    bool AssetDatabase::UpdateAssetFile(const char* path)
    {
        if (!HE_VERIFY(IsAbsolutePath(path),
            HE_MSG("The AssetDatabase can only handle absolute paths."),
            HE_KV(file_path, path)))
        {
            return false;
        }

        const AssetDatabase::LoadResult load = LoadAssetFile(path);
        if (!load.result)
        {
            HE_LOG_WARN(he_assets,
                HE_MSG("Failed to read asset file data from disk, cannot update database."),
                HE_KV(file_path, path),
                HE_KV(result, load.result));
            return false;
        }

        return UpdateAssetFile(path, load.builder.Root());
    }

    bool AssetDatabase::UpdateAssetFile(const char* path, AssetFile::Reader assetFile)
    {
        if (!HE_VERIFY(IsAbsolutePath(path),
            HE_MSG("The AssetDatabase can only handle absolute paths."),
            HE_KV(file_path, path)))
        {
            return false;
        }

        // Otherwise lets prepare the file model and update the DB
        FileAttributes attrs{};
        const Result r = File::GetAttributes(path, attrs);
        if (!r)
        {
            HE_LOG_WARN(he_assets,
                HE_MSG("Failed to read asset file attributes from disk, write time and size will be incorrect."),
                HE_KV(file_path, path),
                HE_KV(result, r));
            return false;
        }

        AssetFileModel model;
        model.uuid = assetFile.GetUuid();
        model.filePath = path;
        model.fileWriteTime = attrs.writeTime;
        model.fileSize = static_cast<uint32_t>(attrs.size);

        if (assetFile.HasSource() && !assetFile.GetSource().IsEmpty())
        {
            const schema::String::Reader sourcePath = assetFile.GetSource();
            model.sourcePath = sourcePath;

            const Result rc = File::GetAttributes(model.sourcePath.Data(), attrs);
            if (rc)
            {
                model.sourceWriteTime = attrs.writeTime;
                model.sourceSize = static_cast<uint32_t>(attrs.size);
            }
            else
            {
                HE_LOG_WARN(he_assets,
                    HE_MSG("Failed to read source file attributes from disk, write time and size will be incorrect."),
                    HE_KV(asset_file_path, model.filePath),
                    HE_KV(asset_source_path, model.sourcePath),
                    HE_KV(result, rc));
            }
        }

        if (!AssetFileModel::AddOrUpdate(*this, assetFile, model))
        {
            HE_LOG_WARN(he_assets,
                HE_MSG("Failed to update asset cached DB with asset file data. Check the logs above this for additional details."),
                HE_KV(file_uuid, AssetFileUuid(assetFile.GetUuid())),
                HE_KV(file_path, model.filePath));
            return false;
        }

        return true;
    }

    AssetDatabase::LoadResult AssetDatabase::LoadAssetFile(const char* path)
    {
        if (!HE_VERIFY(IsAbsolutePath(path),
            HE_MSG("The AssetDatabase can only handle absolute paths."),
            HE_KV(file_path, path)))
        {
            return { Result::InvalidParameter };
        }

        LoadResult load;
        String content;
        load.result = File::ReadAll(content, path);
        if (!load.result)
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to load asset file."),
                HE_KV(file_path, path),
                HE_KV(result, load.result));

            return load;
        }

        if (!schema::FromToml(load.builder, content.Data()))
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to parse asset file. Is it valid TOML?"),
                HE_KV(file_path, path));

            return { Result::InvalidParameter };
        }

        return load;
    }

    AssetDatabase::LoadResult AssetDatabase::LoadAssetFile(const AssetFileUuid& fileUuid)
    {
        AssetFileModel model;
        if (!m_storage.FindOne(model, Where(Col(&AssetFileModel::uuid) == fileUuid)))
        {
            return { Result::InvalidParameter };
        }

        return LoadAssetFile(model.filePath.Data());
    }

    bool AssetDatabase::SaveAssetFile(const char* path, AssetFile::Reader assetFile)
    {
        if (!HE_VERIFY(IsAbsolutePath(path),
            HE_MSG("The AssetDatabase can only handle absolute paths."),
            HE_KV(file_path, path)))
        {
            return false;
        }

        String str;
        schema::ToToml<AssetFile>(str, assetFile);

        const Result r = File::WriteAll(str.Data(), str.Size(), path);
        if (!r)
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to write asset file."),
                HE_KV(path, path),
                HE_KV(result, r));
            return false;
        }

        UpdateAssetFile(path, assetFile);
        return true;
    }

    void AssetDatabase::OnAssetFileDeleted(const char* path)
    {
        if (!HE_VERIFY(IsAbsolutePath(path),
            HE_MSG("The AssetDatabase can only handle absolute paths."),
            HE_KV(file_path, path)))
        {
            return;
        }

        m_storage.Destroy<AssetFileModel>(Where(Col(&AssetFileModel::filePath) == path));
    }

    void AssetDatabase::OnAssetFileUpdated(const char* path)
    {
        if (!HE_VERIFY(IsAbsolutePath(path),
            HE_MSG("The AssetDatabase can only handle absolute paths."),
            HE_KV(file_path, path)))
        {
            return;
        }

        if (IsFileUpToDate(path))
        {
            HE_LOG_TRACE(he_assets,
                HE_MSG("OnAssetFileUpdated called, but the file seems to be already up to date."),
                HE_KV(file_path, path));
            return;
        }

        UpdateAssetFile(path);
    }

    String AssetDatabase::MakeResourcePath(const AssetUuid& assetUuid, ResourceId resourceId) const
    {
        String path = m_resourceRoot;
        FormatTo(path, "/{}", assetUuid);
        Directory::Create(path.Data(), true);
        FormatTo(path, "/{}", resourceId);
        return path;
    }

    void AssetDatabase::HandleFileReadComplete(LoadRequest* load, Result result)
    {
        HE_AT_SCOPE_EXIT([&]() { Allocator::GetDefault().Delete(load); });

        if (!result)
        {
            load->callback({ result });
            return;
        }

        LoadResult loadResult;

        if (!schema::FromToml(loadResult.builder, load->content.Data()))
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to parse asset file. Is it valid TOML?"),
                HE_KV(file_path, load->path));

            load->callback({ Result::InvalidParameter });
            return;
        }

        load->callback(Move(loadResult));
    }

    void AssetDatabase::HandleLoadForUpdateComplete(UpdateRequest* req, LoadResult load)
    {
        AssetDatabase& db = *req->db;
        const String& path = req->path;
        const LoadDelegate& callback = req->callback;

        HE_AT_SCOPE_EXIT([&]() { Allocator::GetDefault().Delete(req); });

        if (load.result)
            db.UpdateAssetFile(path.Data(), load.builder.Root());

        if (callback)
            callback(Move(load));
    }
}
