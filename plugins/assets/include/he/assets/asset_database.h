// Copyright Chad Engler

#pragma once

#include "he/assets/types.h"
#include "he/core/async_file.h"
#include "he/core/string.h"
#include "he/core/types.h"
#include "he/schema/layout.h"
#include "he/sqlite/database.h"

#include <future>

namespace he::assets
{
    class AssetDatabase final
    {
    public:
        struct LoadResult
        {
            Result result{};
            schema::Builder builder{};
            AssetFile::Builder assetFile{};
        };

    public:
        ~AssetDatabase() { Terminate(); }

        bool Initialize(const char* dbPath);
        bool Terminate();

        bool IsFileUpToDate(const char* path);
        bool MaybeUpdateFile(const char* path);

        std::future<LoadResult> LoadAssetFile(const char* path);
        std::future<LoadResult> LoadAssetFile(const AssetFileUuid& fileUuid);

        void PollPendingLoads();

        sqlite::Transaction BeginTransaction() const { return m_db.BeginTransaction(); }
        const sqlite::Statement& StatementLiteral(const char* sql) { return m_db.StatementLiteral(sql); }

    private:
        struct PendingLoad
        {
            AsyncFile file{};
            String path{};
            String content{};
            std::future<AsyncFileResult> load{};
        };

        struct LoadReq
        {
            int32_t pendingIndex{ -1 };
            std::promise<LoadResult> promise{};
        };

        PendingLoad* FindAvailablePending();

    private:
        sqlite::Database m_db;
        PendingLoad m_pending[32]{};
        Vector<LoadReq> m_requestedLoads{};
    };
}
