// Copyright Chad Engler

#include "he/assets/asset_database.h"

#include "migrations.h"

#include "he/assets/asset_models.h"
#include "he/assets/types.h"
#include "he/assets/types_fmt.h"
#include "he/core/appender.h"
#include "he/core/async_file.h"
#include "he/core/clock.h"
#include "he/core/clock_fmt.h"
#include "he/core/directory.h"
#include "he/core/file.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/string.h"
#include "he/core/string_builder.h"
#include "he/core/string_fmt.h"
#include "he/schema/toml.h"

#include "fmt/core.h"

#include <limits>

namespace he::assets
{
    bool AssetDatabase::Initialize(const char* cacheRoot, const char* assetRoot)
    {
        // Asset root needs to be an absolute path for us to utilize it correctly
        m_assetRoot = assetRoot;
        if (!HE_VERIFY(IsAbsolutePath(assetRoot), HE_KV(asset_root, assetRoot)))
            return false;

        // Ensure the cache root, and the resources directory both exist
        m_resourceRoot = cacheRoot;
        ConcatPath(m_resourceRoot, "resources");
        if (!HE_VERIFY(Directory::Create(m_resourceRoot.Data(), true), HE_KV(resource_root, m_resourceRoot)))
            return false;

        // Open the sqlite database and migrate the schema
        String dbPath = cacheRoot;
        ConcatPath(dbPath, "asset_cache.db");
        if (!m_db.Open(dbPath.Data()))
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

    bool AssetDatabase::UpdateAssetFileAsync(const char* path, LoadDelegate callback)
    {
        if (!HE_VERIFY(!String::IsEmpty(path)))
        {
            HE_LOG_WARN(he_assets,
                HE_MSG("UpdateAssetFileAsync called with an empty path, ignoring update request."),
                HE_KV(file_path, path));
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

        if (!HE_VERIFY(!String::IsEmpty(path)))
        {
            HE_LOG_WARN(he_assets,
                HE_MSG("LoadAssetFileAsync called with an empty path, ignoring load request."),
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

        AsyncFile file;
        Result r = file.Open(absPath.Data(), FileOpenMode::ReadExisting, FileOpenFlag::SequentialScan);
        if (!r)
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to open asset file."),
                HE_KV(file_path, absPath),
                HE_KV(result, r));

            callback({ r });
            return false;
        }

        const uint64_t size = file.GetSize();
        if (size > std::numeric_limits<uint32_t>::max())
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Asset file size is larger than UINT32_MAX"),
                HE_KV(file_size, size),
                HE_KV(file_path, absPath));

            callback({ Result::InvalidParameter });
            return false;
        }

        const uint32_t fileByteSize = static_cast<uint32_t>(size);

        LoadRequest* load = Allocator::GetDefault().New<LoadRequest>();
        load->path = absPath;
        load->content.Resize(fileByteSize, DefaultInit);
        load->db = this;
        load->callback = callback;

        file.ReadAsync(load->content.Data(), 0, fileByteSize, AsyncFile::CompleteDelegate::Make([](const void* l, AsyncFileOp token)
        {
            Result r = AsyncFile::GetResult(token);
            HandleFileReadComplete(static_cast<LoadRequest*>(const_cast<void*>(l)), r);
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
        if (!AssetFileModel::FindOne(*this, fileUuid, model))
        {
            callback({ Result::InvalidParameter });
            return false;
        }

        return LoadAssetFileAsync(model.file.path.Data(), Move(callback));
    }

    AssetDatabase::LoadResult AssetDatabase::LoadAssetFile(const char* path)
    {
        if (!HE_VERIFY(!String::IsEmpty(path)))
        {
            HE_LOG_WARN(he_assets,
                HE_MSG("LoadAssetFile called with an empty path, ignoring load request."),
                HE_KV(file_path, path));

            return { Result::InvalidParameter };
        }

        String absPath;
        if (!PrepareAbsolutePath(path, absPath))
        {
            return { Result::InvalidParameter };
        }

        LoadResult loadResult;
        String content;
        Result r = File::ReadAll(content, absPath.Data());
        if (!r)
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to load asset file."),
                HE_KV(file_path, absPath),
                HE_KV(result, r));

            return { r };
        }

        if (!schema::FromToml(loadResult.builder, content.Data()))
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to parse asset file. Is it valid TOML?"),
                HE_KV(file_path, absPath));

            return { Result::InvalidParameter };
        }

        if (loadResult.builder.Root().IsValid())
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to parse asset file. The root struct is not an AssetFile."),
                HE_KV(file_path, absPath));

            return { Result::InvalidParameter };
        }

        return loadResult;
    }

    AssetDatabase::LoadResult AssetDatabase::LoadAssetFile(const AssetFileUuid& fileUuid)
    {
        AssetFileModel model;
        if (!AssetFileModel::FindOne(*this, fileUuid, model))
        {
            return { Result::InvalidParameter };
        }

        return LoadAssetFile(model.file.path.Data());
    }

    bool AssetDatabase::SaveAssetFile(const char* path, AssetFile::Reader assetFile)
    {
        if (!HE_VERIFY(!String::IsEmpty(path)))
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("SaveAssetFile called with an empty path."),
                HE_KV(path, path));
            return false;
        }

        String absPath;
        if (!HE_VERIFY(PrepareAbsolutePath(path, absPath)))
            return false;

        StringBuilder stringBuilder;
        if (!HE_VERIFY(schema::ToToml<AssetFile>(stringBuilder, assetFile)))
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to serialize asset file. Is it valid?"),
                HE_KV(path, path));
            return false;
        }

        const String& str = stringBuilder.Str();
        const Result r = File::WriteAll(str.Data(), str.Size(), path);
        if (!r)
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to write asset file."),
                HE_KV(path, path),
                HE_KV(result, r));
            return false;
        }

        AssetFileUpdateInternal(path, assetFile);
        return true;
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

        if (IsFileUpToDate(path))
        {
            HE_LOG_TRACE(he_assets,
                HE_MSG("OnAssetFileUpdated called, but the file seems to be already up to date."),
                HE_KV(file_path, path));
            return;
        }

        UpdateAssetFileAsync(path, {});
    }

    bool AssetDatabase::PrepareRelativePath(const char* path, String& relPath) const
    {
        relPath = path;
        if (IsAbsolutePath(relPath))
        {
            if (!MakeRelative(relPath, m_assetRoot))
            {
                HE_LOG_WARN(he_assets,
                    HE_MSG("Absolute path does not refer to a file in the asset root directory, ignoring."),
                    HE_KV(file_path, relPath),
                    HE_KV(root_dir, m_assetRoot));
                return false;
            }
        }

        NormalizePath(relPath);
        return true;
    }

    bool AssetDatabase::PrepareAbsolutePath(const char* path, String& absPath) const
    {
        if (IsAbsolutePath(path) && !IsChildPath(path, m_assetRoot))
        {
            HE_LOG_WARN(he_assets,
                HE_MSG("Absolute path does not refer to a file in the asset root directory, ignoring."),
                HE_KV(file_path, path),
                HE_KV(root_dir, m_assetRoot));
            return false;
        }

        absPath = m_assetRoot;
        ConcatPath(absPath, path);
        NormalizePath(absPath);
        return true;
    }

    String AssetDatabase::MakeResourcePath(const AssetUuid& assetUuid, ResourceId resourceId) const
    {
        String path = m_resourceRoot;
        fmt::format_to(Appender(path), "/{}", assetUuid);
        Directory::Create(path.Data(), true);
        fmt::format_to(Appender(path), "/{}", resourceId);
        return path;
    }

    void AssetDatabase::AssetFileUpdateInternal(const char* path, AssetFile::Reader assetFile)
    {
        String relPath;
        if (!HE_VERIFY(PrepareRelativePath(path, relPath)))
            return;

        String absPath;
        if (!HE_VERIFY(PrepareAbsolutePath(path, absPath)))
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
        model.uuid = assetFile.GetUuid();
        model.file.path = relPath;
        model.file.writeTime = attrs.writeTime;
        model.file.size = static_cast<uint32_t>(attrs.size);
        model.source = {};

        if (assetFile.HasSource() && !assetFile.GetSource().IsEmpty())
        {
            const schema::String::Reader sourcePath = assetFile.GetSource();

            model.source.path = sourcePath;

            // TODO: Modify for when we support sparse checkout of source files.
            String sourceAbsPath;
            if (PrepareAbsolutePath(model.source.path.Data(), sourceAbsPath))
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

        if (!AssetFileModel::AddOrUpdate(*this, assetFile, model))
        {
            HE_LOG_WARN(he_assets,
                HE_MSG("Failed to update asset cached DB with asset file data. Check the logs above this for additional details."),
                HE_KV(file_uuid, AssetFileUuid(assetFile.GetUuid())),
                HE_KV(file_path, absPath));
        }
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

        // Root() of TypeBuilder will add it if it doesn't exist, so we check the underlying raw
        // builder if the root was properly set.
        schema::PointerBuilder root = loadResult.builder.builder.Root();
        if (root.IsNull() || !root.TryGetStruct<AssetFile>().IsValid())
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

        if (load.result)
            db.AssetFileUpdateInternal(path.Data(), load.builder.Root());

        if (callback)
            callback(Move(load));
    }
}
