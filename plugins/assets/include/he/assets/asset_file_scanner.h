// Copyright Chad Engler

#pragma once

#include "he/assets/asset_database.h"
#include "he/core/async_file.h"
#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/core/vector.h"
#include "he/schema/types.h"

#include <future>

namespace he::assets
{
    class AssetFileScanner
    {
    public:
        AssetFileScanner(AssetDatabase& db);

        bool Run(const char* rootDir);

    private:
        struct PendingLoad
        {
            AsyncFile file{};
            String path{};
            String content{};
            std::future<AsyncFileResult> load{};
        };

        bool ScanDirectory(const char* dir);
        bool IsFileUpToDate(const char* path);
        bool StartFileUpdate(const char* path);
        bool ProcessPending(uint32_t max, bool wait);
        bool ProcessFile(const PendingLoad& pending);

        bool WriteScanHeader();
        bool ClearScanHeader();

        PendingLoad* FindAvailablePending();

    private:
        struct ScanHeader
        {
            uint32_t pid;
            uint32_t token;
        };

    private:
        AssetDatabase& m_db;
        PendingLoad m_pending[32]{};
        uint32_t m_token{ 0 };
    };
}
