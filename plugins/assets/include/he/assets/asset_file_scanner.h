// Copyright Chad Engler

#pragma once

#include "he/assets/asset_database.h"
#include "he/core/async_file.h"
#include "he/core/span.h"
#include "he/core/types.h"
#include "he/core/vector.h"

#include "capnp/common.h"

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
            Vector<capnp::word> words;
            std::future<AsyncFileResult> load;
        };

        bool ScanDirectory(const char* dir);
        bool ReadFile(const char* fname);
        bool ProcessPending(uint32_t max, bool wait);
        bool ProcessFile(Span<const capnp::word> buffer);

        PendingLoad* FindAvailablePending();

    private:
        AssetDatabase& m_db;
        PendingLoad m_pending[32];
    };
}
