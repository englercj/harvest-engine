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

#include <atomic>

namespace he::assets
{
    class AssetFileScanner
    {
    public:
        AssetFileScanner(AssetDatabase& db) noexcept;

        bool Run(const char* rootDir);

    private:
        bool ScanDirectory(const char* dir);

        bool WriteScanHeader();
        bool ClearScanHeader();

        void OnUpdateComplete(AssetDatabase::LoadResult result);

    private:
        struct ScanHeader
        {
            uint32_t pid;
            uint32_t token;
        };

    private:
        AssetDatabase& m_db;
        uint32_t m_token{ 0 };
        std::atomic<uint32_t> m_pendingOps{ 0 };
    };
}
