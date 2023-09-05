// Copyright Chad Engler

#pragma once

#include "he/assets/asset_models.h"
#include "he/assets/types.h"
#include "he/core/async_file.h"
#include "he/core/delegate.h"
#include "he/core/string.h"
#include "he/core/types.h"
#include "he/schema/layout.h"
#include "he/sqlite/database.h"
#include "he/sqlite/orm_storage.h"

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
        AssetDatabase();
        ~AssetDatabase() noexcept { Terminate(); }

        bool Initialize(StringView cacheRoot, Span<String> contentRoots);
        bool Terminate();

        bool IsInitialized() const { return m_storage.IsOpen(); }

        AssetDbStorage& Storage() { return m_storage; }
        sqlite::Database& Db() { return m_storage.Db(); }
        sqlite3* Handle() const { return m_storage.Handle(); }

        bool IsFileUpToDate(const char* path);
        bool UpdateAssetFileAsync(const char* path, LoadDelegate callback);
        bool LoadAssetFileAsync(const char* path, LoadDelegate callback);
        bool LoadAssetFileAsync(const AssetFileUuid& fileUuid, LoadDelegate callback);

        bool UpdateAssetFile(const char* path);
        bool UpdateAssetFile(const char* path, AssetFile::Reader assetFile);
        LoadResult LoadAssetFile(const char* path);
        LoadResult LoadAssetFile(const AssetFileUuid& fileUuid);
        bool SaveAssetFile(const char* path, AssetFile::Reader assetFile);

        void OnAssetFileDeleted(const char* path);
        void OnAssetFileUpdated(const char* path);

    public:
        Span<const String> ContentRoots() const { return m_contentRoots; }

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
        String MakeResourcePath(const AssetUuid& assetUuid, ResourceId resourceId) const;

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
        AssetDbStorage m_storage;
        String m_resourceRoot;
        Vector<String> m_contentRoots;
    };
}
