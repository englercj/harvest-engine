// Copyright Chad Engler

#pragma once

#include "change_journal_watcher.h"

#include "he/assets/asset_database.h"
#include "he/core/signal.h"
#include "he/core/directory_watcher.h"

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
        void HandleFileEntry(const DirectoryWatcher::Entry& entry);

        void JournalWatchThreadFunc();
        void HandleJournalEntry(const ChangeJournalWatcher::Entry& entry);

    private:
        AssetDatabase& m_db;

        ChangeJournalWatcher m_journalWatcher{};
        DirectoryWatcher m_dirWatcher{};

        int64_t m_startJournalMax{ 0 };
        bool m_useJournal{ false };
        std::atomic<bool> m_running{ false };

        std::thread m_watchThread{};
        std::thread m_scanThread{};

        OnReadySignal m_onReadySignal{};
    };
}
