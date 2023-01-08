// Copyright Chad Engler

#pragma once

#include "he/assets/types.h"
#include "he/core/async_file.h"
#include "he/core/delegate.h"
#include "he/core/string.h"
#include "he/core/types.h"
#include "he/schema/layout.h"
#include "he/sqlite/database.h"

namespace he::assets
{
    class AssetDatabase final
    {
    public:
        using AssetFileBuilder = schema::TypedBuilder<AssetFile>;

        struct LoadResult
        {
            Result result{};
            AssetFileBuilder builder{};
        };

        using LoadDelegate = Delegate<void(LoadResult)>;

    public:
        ~AssetDatabase() noexcept { Terminate(); }

        bool Initialize(const char* cacheRoot, const char* assetRoot);
        bool Terminate();

        bool IsInitialized() const { return m_db.IsOpen(); }

        // TODO: Audit the path handling in these.
        // All of these should work with absolute paths, or asset root relative paths.
        bool IsFileUpToDate(const char* path);
        bool UpdateAssetFileAsync(const char* path, LoadDelegate callback);
        bool LoadAssetFileAsync(const char* path, LoadDelegate callback);
        bool LoadAssetFileAsync(const AssetFileUuid& fileUuid, LoadDelegate callback);

        LoadResult LoadAssetFile(const char* path);
        LoadResult LoadAssetFile(const AssetFileUuid& fileUuid);
        bool SaveAssetFile(const char* path, AssetFile::Reader assetFile);

        void OnAssetFileDeleted(const char* path);
        void OnAssetFileUpdated(const char* path);

    public:
        const String& AssetRoot() const { return m_assetRoot; }

        sqlite::Transaction BeginTransaction() const { return m_db.BeginTransaction(); }
        const sqlite::Statement& StatementLiteral(const char* sql) { return m_db.StatementLiteral(sql); }

    public:
        Result AddResource(const AssetUuid& assetUuid, ResourceId resourceId, Span<const uint8_t> data) const
        {
            String path = MakeResourcePath(assetUuid, resourceId);
            return File::WriteAll(data.Data(), data.Size(), path.Data());
        }

        template <typename T>
        Result GetResource(T& data, const AssetUuid& assetUuid, ResourceId resourceId) const
        {
            String path = MakeResourcePath(assetUuid, resourceId);
            return File::ReadAll(data, path.Data());
        }

    private:
        bool PrepareRelativePath(const char* path, String& relPath) const;
        bool PrepareAbsolutePath(const char* path, String& absPath) const;

        String MakeResourcePath(const AssetUuid& assetUuid, ResourceId resourceId) const;

        void AssetFileUpdateInternal(const char* path, AssetFile::Reader assetFile);

    private:
        struct LoadRequest
        {
            AsyncFile file{};
            String path{};
            String content{};
            AssetDatabase* db{ nullptr };
            LoadDelegate callback{};
        };

        struct UpdateRequest
        {
            String path{};
            AssetDatabase* db{ nullptr };
            LoadDelegate callback{};
        };

        static void HandleFileReadComplete(LoadRequest* load, Result result);
        static void HandleLoadForUpdateComplete(UpdateRequest* req, LoadResult load);

    private:
        sqlite::Database m_db;
        String m_assetRoot;
        String m_resourceRoot;
    };
}
