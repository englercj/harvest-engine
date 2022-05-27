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
        AssetFileScanner(AssetDatabase& db)
            : m_db(db)
        {}

        bool Run(const char* rootDir);

    private:
        struct PendingLoad
        {
            AsyncFile file;
            String path;
            String content;
            std::future<AsyncFileResult> load;
        };

        bool ScanDirectory(const char* dir);
        bool ReadFile(const char* fname);
        bool ProcessPending(uint32_t max, bool wait);
        bool ProcessFile(const FileAttributes& attributes, const String& path, const String& content);

        PendingLoad* FindAvailablePending();

    private:
        AssetDatabase& m_db;
        PendingLoad m_pending[32];
    };
}
