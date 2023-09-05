// Copyright Chad Engler

#pragma once

#include "change_journal_watcher.h"

#include "he/assets/asset_database.h"
#include "he/core/signal.h"
#include "he/core/directory_watcher.h"
#include "he/core/vector.h"

#include <atomic>
#include <thread>

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
        void ScanThreadFunc();
        void FileWatchThreadFunc();

    private:
        AssetDatabase& m_db;

        uint32_t m_scanToken{ 0 };
        std::atomic<bool> m_running{ false };

        std::thread m_scanThread{};
        std::thread m_watchThread{};
        Vector<DirectoryWatcher> m_dirWatchers{};

        OnReadySignal m_onReadySignal{};
    };
}
