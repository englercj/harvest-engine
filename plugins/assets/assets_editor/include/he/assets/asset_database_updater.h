// Copyright Chad Engler

#pragma once

#include "he/assets/asset_database.h"
#include "he/core/atomic.h"
#include "he/core/signal.h"
#include "he/core/directory_watcher.h"
#include "he/core/thread.h"
#include "he/core/vector.h"

namespace he::assets
{
    class AssetDatabaseUpdater final
    {
    public:
        AssetDatabaseUpdater(AssetDatabase& db) : m_db(db) {}

        bool Start();
        void Stop();

    public:
        using OnReadySignal = Signal<void()>;

        OnReadySignal& OnReady() { return m_onReadySignal; }

    private:
        static void ScanThreadFunc(void* instance);
        static void FileWatchThreadFunc(void* instance);

    private:
        AssetDatabase& m_db;

        uint32_t m_scanToken{ 0 };
        Atomic<bool> m_running{ false };

        Thread m_scanThread{};
        Thread m_watchThread{};
        Vector<DirectoryWatcher> m_dirWatchers{};

        OnReadySignal m_onReadySignal{};
    };
}
