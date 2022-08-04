// Copyright Chad Engler

#pragma once

#include "he/assets/types.h"
#include "he/core/async_file_loader.h"
#include "he/core/delegate.h"
#include "he/core/string.h"
#include "he/core/types.h"
#include "he/schema/layout.h"
#include "he/sqlite/database.h"

#include <functional>

namespace he::assets
{
    class AssetDatabase final
    {
    public:
        struct LoadResult
        {
            Result result{};
            he::schema::TypedBuilder<schema::AssetFile> builder{};
        };

        using LoadDelegate = Delegate<void(LoadResult)>;

    public:
        ~AssetDatabase() noexcept { Terminate(); }

        bool Initialize(const char* dbPath, const char* assetRoot, AsyncFileLoader& loader);
        bool Terminate();

        // TODO: Audit the path handling in these.
        // All of these should work with absolute paths, or asset root relative paths.
        bool IsFileUpToDate(const char* path);
        bool UpdateAssetFile(const char* path, LoadDelegate callback);
        bool LoadAssetFile(const char* path, LoadDelegate callback);
        bool LoadAssetFile(const AssetFileUuid& fileUuid, LoadDelegate callback);
        bool SaveAssetFile(const char* path, schema::AssetFile::Reader assetFile);

        void OnAssetFileDeleted(const char* path);
        void OnAssetFileUpdated(const char* path);

        const String& AssetRoot() const { return m_assetRoot; }

        sqlite::Transaction BeginTransaction() const { return m_db.BeginTransaction(); }
        const sqlite::Statement& StatementLiteral(const char* sql) { return m_db.StatementLiteral(sql); }

    private:
        bool PrepareRelativePath(const char* path, String& relPath) const;
        bool PrepareAbsolutePath(const char* path, String& absPath) const;

        void AssetFileUpdateInternal(const char* path, schema::AssetFile::Reader assetFile);

    private:
        struct LoadRequest
        {
            AsyncFileId fileId{};
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
        AsyncFileLoader* m_loader;
    };
}
