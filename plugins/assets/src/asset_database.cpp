// Copyright Chad Engler

#include "he/assets/asset_database.h"

#include "he/assets/asset_models.h"
#include "he/assets/types.h"
#include "he/assets/types_fmt.h"
#include "he/core/clock.h"
#include "he/core/clock_fmt.h"
#include "he/core/file.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/string_fmt.h"
#include "he/schema/toml.h"

#include "migrations.h"

namespace he::assets
{
    bool AssetDatabase::Initialize(const char* dbPath, const char* rootDir, AsyncFileLoader& loader)
    {
        m_rootDir = rootDir;
        Result r = MakeAbsolute(m_rootDir);
        if (!r)
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to get absolute path for root directory."),
                HE_KV(root_dir, rootDir),
                HE_KV(result, r));
            return false;
        }
        NormalizePath(m_rootDir);

        m_loader = &loader;

        if (!m_db.Open(dbPath))
            return false;

        if (!m_db.MigrateSchema(AssetDatabase_Migrations))
            return false;

        return true;
    }

    bool AssetDatabase::Terminate()
    {
        return m_db.Close();
    }

    bool AssetDatabase::IsFileUpToDate(const char* path)
    {
        String relPath;
        if (!PrepareRelativePath(path, relPath))
            return false;

        AssetFileModel model;
        if (!AssetFileModel::FindOne(*this, path, model))
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
        if (model.file.size != fileSize)
        {
            HE_LOG_DEBUG(he_assets,
                HE_MSG("Asset file size has changed, update required."),
                HE_KV(path, path),
                HE_KV(last_file_size, model.file.size),
                HE_KV(file_size, fileSize));
            return false;
        }

        if (model.file.writeTime != attributes.writeTime)
        {
            HE_LOG_DEBUG(he_assets,
                HE_MSG("Asset file write time has changed, update required."),
                HE_KV(path, path),
                HE_KV(last_write_time, model.file.writeTime),
                HE_KV(write_time, attributes.writeTime));
            return false;
        }

        // Check the asset file's source to see if it has changed since last time we've scanned it
        // If this is true, we probably want to re-run the importer to ensure import resources exist.
        if (!model.source.path.IsEmpty())
        {
            // TODO: Modify for when we support sparse checkout of source files.
            res = File::GetAttributes(model.source.path.Data(), attributes);
            if (!res)
            {
                HE_LOG_ERROR(he_assets,
                    HE_MSG("Failed to read source file attributes. Assuming update is not required."),
                    HE_KV(path, path),
                    HE_KV(result, res));
                return true;
            }

            const uint32_t sourceFileSize = static_cast<uint32_t>(attributes.size);
            if (model.source.size != sourceFileSize)
            {
                HE_LOG_DEBUG(he_assets,
                    HE_MSG("Source file size has changed, update required."),
                    HE_KV(path, path),
                    HE_KV(last_file_size, model.source.size),
                    HE_KV(file_size, sourceFileSize));
                return false;
            }

            if (model.source.writeTime != attributes.writeTime)
            {
                HE_LOG_DEBUG(he_assets,
                    HE_MSG("Source file write time has changed, update required."),
                    HE_KV(path, path),
                    HE_KV(last_write_time, model.source.writeTime),
                    HE_KV(write_time, attributes.writeTime));
                return false;
            }
        }

        // Nothing has changed, good to go.
        return true;
    }

    bool AssetDatabase::UpdateAssetFile(const char* path, LoadDelegate callback)
    {
        if (!HE_VERIFY(!String::IsEmpty(path)))
        {
            HE_LOG_WARN(he_assets,
                HE_MSG("UpdateAssetFile called with an empty path, ignoring update request."),
                HE_KV(file_path, path));
            return false;
        }

        UpdateRequest* req = Allocator::GetDefault().New<UpdateRequest>();
        req->db = this;
        req->path = path;
        req->callback = callback;

        return LoadAssetFile(path, LoadDelegate::Make<&HandleLoadForUpdateComplete>(req));
    }

    bool AssetDatabase::LoadAssetFile(const char* path, LoadDelegate callback)
    {
        if (!HE_VERIFY(callback))
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("LoadAssetFile called without a callback, ignoring load request."),
                HE_KV(file_path, path));
            return false;
        }

        if (!HE_VERIFY(!String::IsEmpty(path)))
        {
            HE_LOG_WARN(he_assets,
                HE_MSG("LoadAssetFile called with an empty path, ignoring load request."),
                HE_KV(file_path, path));

            callback({ Result::InvalidParameter });
            return false;
        }

        String absPath;
        if (!PrepareAbsolutePath(path, absPath))
        {
            callback({ Result::InvalidParameter });
            return false;
        }

        AsyncFileId fileId;
        Result r = m_loader->OpenFile(absPath.Data(), fileId);
        if (!r)
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to open asset file."),
                HE_KV(file_path, absPath),
                HE_KV(result, r));

            callback({ r });
            return false;
        }

        FileAttributes attrs;
        r = m_loader->GetAttributes(fileId, attrs);
        if (!r)
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to read attributes of asset file. Does it exist?"),
                HE_KV(file_path, absPath));

            callback({ r });
            return false;
        }

        if (attrs.size > std::numeric_limits<uint32_t>::max())
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Asset file size is larger than UINT32_MAX"),
                HE_KV(file_size, attrs.size),
                HE_KV(file_path, absPath));

            callback({ Result::InvalidParameter });
            return false;
        }

        const uint32_t fileByteSize = static_cast<uint32_t>(attrs.size);

        // TODO: Use an object pool
        LoadRequest* load = Allocator::GetDefault().New<LoadRequest>();
        load->path = absPath;
        load->content.Resize(fileByteSize, DefaultInit);
        load->db = this;
        load->callback = callback;

        AsyncFileQueue* queue = m_loader->DefaultQueue();

        AsyncFileRequest req;
        req.file = fileId;
        req.offset = 0;
        req.size = fileByteSize;
        req.dst = load->content.Data();
        req.dstSize = fileByteSize;
        req.name = load->path.Data();

        queue->EnqueueRequest(req);
        queue->EnqueueDelegate(AsyncFileQueue::LoadDelegate::Make<&HandleFileReadComplete>(load));
        queue->Submit();

        load->db->m_loader->CloseFile(fileId);

        return true;
    }

    bool AssetDatabase::LoadAssetFile(const AssetFileUuid& fileUuid, LoadDelegate callback)
    {
        if (!HE_VERIFY(callback))
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("LoadAssetFile called without a callback, ignoring load request."),
                HE_KV(file_uuid, fileUuid));
            return false;
        }

        AssetFileModel model;
        if (!AssetFileModel::FindOne(*this, fileUuid, model))
        {
            callback({ Result::InvalidParameter });
            return false;
        }

        return LoadAssetFile(model.file.path.Data(), Move(callback));
    }

    void AssetDatabase::OnAssetFileDeleted(const char* path)
    {
        if (!HE_VERIFY(!String::IsEmpty(path)))
        {
            HE_LOG_WARN(he_assets,
                HE_MSG("OnAssetFileDeleted called with an empty path, ignoring delete notification."),
                HE_KV(file_path, path));
            return;
        }

        String relPath;
        if (!PrepareRelativePath(path, relPath))
            return;

        NormalizePath(relPath);
        AssetFileModel::RemoveOne(*this, relPath.Data());
    }

    void AssetDatabase::OnAssetFileUpdated(const char* path)
    {
        if (!HE_VERIFY(!String::IsEmpty(path)))
        {
            HE_LOG_WARN(he_assets,
                HE_MSG("OnAssetFileUpdated called with an empty path, ignoring update notification."),
                HE_KV(file_path, path));
            return;
        }

        String relPath;
        if (!PrepareRelativePath(path, relPath))
            return;

        UpdateAssetFile(relPath.Data());
    }

    bool AssetDatabase::PrepareRelativePath(const char* path, String& relPath) const
    {
        relPath = path;
        if (IsAbsolutePath(relPath))
        {
            if (!MakeRelative(relPath, m_rootDir))
            {
                HE_LOG_WARN(he_assets,
                    HE_MSG("Absolute path does not refer to a file in the asset root directory, ignoring."),
                    HE_KV(file_path, relPath),
                    HE_KV(root_dir, m_rootDir));
                return false;
            }
        }

        NormalizePath(relPath);
        return true;
    }

    bool AssetDatabase::PrepareAbsolutePath(const char* path, String& absPath) const
    {
        if (IsAbsolutePath(path) && !IsChildPath(path, m_rootDir))
        {
            HE_LOG_WARN(he_assets,
                HE_MSG("Absolute path does not refer to a file in the asset root directory, ignoring."),
                HE_KV(file_path, path),
                HE_KV(root_dir, m_rootDir));
            return false;
        }

        absPath = m_rootDir;
        ConcatPath(absPath, path);
        return true;
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

        if (!schema::FromToml<AssetFile>(loadResult.builder, load->content.Data()))
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to parse asset file. Is it valid TOML?"),
                HE_KV(file_path, load->path));

            load->callback({ Result::InvalidParameter });
            return;
        }

        loadResult.assetFile = loadResult.builder.Root().TryGetStruct<AssetFile>();
        if (!loadResult.assetFile.IsValid())
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to parse asset file. The root struct is not an AssetFile."),
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

        // Early out of the file failed to load
        if (!load.result)
        {
            if (callback)
                callback(Move(load));
            return;
        }

        String relPath;
        if (!HE_VERIFY(db.PrepareRelativePath(path.Data(), relPath)))
            return;

        String absPath;
        if (!HE_VERIFY(db.PrepareAbsolutePath(path.Data(), absPath)))
            return;

        // Otherwise lets prepare the file model and update the DB
        FileAttributes attrs{};
        Result res = File::GetAttributes(absPath.Data(), attrs);
        if (!res)
        {
            HE_LOG_WARN(he_assets,
                HE_MSG("Failed to read asset file attributes from disk, write time and size will be incorrect."),
                HE_KV(result, res),
                HE_KV(file_path, absPath));
        }

        AssetFileModel model;
        model.uuid = load.assetFile.GetUuid();
        model.file.path = relPath;
        model.file.writeTime = attrs.writeTime;
        model.file.size = static_cast<uint32_t>(attrs.size);
        model.source = {};

        if (load.assetFile.HasSource() && !load.assetFile.GetSource().IsEmpty())
        {
            const schema::String::Reader sourcePath = load.assetFile.GetSource();

            model.source.path = sourcePath;

            // TODO: Modify for when we support sparse checkout of source files.
            String sourceAbsPath;
            if (db.PrepareAbsolutePath(model.source.path.Data(), sourceAbsPath))
            {
                res = File::GetAttributes(sourceAbsPath.Data(), attrs);
                if (res)
                {
                    model.source.writeTime = attrs.writeTime;
                    model.source.size = static_cast<uint32_t>(attrs.size);
                }
                else
                {
                    HE_LOG_WARN(he_assets,
                        HE_MSG("Failed to read source file attributes from disk, write time and size will be incorrect."),
                        HE_KV(result, res),
                        HE_KV(file_path, absPath),
                        HE_KV(source_path, sourceAbsPath));
                }
            }
        }

        if (!AssetFileModel::AddOrUpdate(db, load.assetFile, model))
        {
            HE_LOG_WARN(he_assets,
                HE_MSG("Failed to update asset cached DB with asset file data. Check the logs above this for additional details."),
                HE_KV(file_uuid, AssetFileUuid(load.assetFile.GetUuid())),
                HE_KV(file_path, absPath));
        }

        if (callback)
            callback(Move(load));
    }

}
