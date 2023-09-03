// Copyright Chad Engler

#include "he/assets/asset_database_updater.h"

#include "he/assets/asset_file_scanner.h"
#include "he/assets/asset_models.h"
#include "he/core/assert.h"
#include "he/core/delegate.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/core/thread.h"
#include "he/core/types.h"
#include "he/sqlite/orm.h"
#include "he/sqlite/orm_storage.h"

#include <functional>

using namespace he::sqlite;

namespace he::assets
{
    constexpr StringView NextUsnConfigKey = "he.assets.next_usn";

    bool AssetDatabaseUpdater::Start()
    {
        if (!HE_VERIFY(!m_running))
            return true;

        const char* path = m_db.AssetRoot().Data();

        // We want to use the Change Journal if possible because it is much more efficient than
        // scanning the filesystem and watching for changes. We are also much less likely to
        // miss change events when using the journal vs the directory watcher.
        //
        // Unfortunately, access to the Change Journal requires Admin privileges. The directory
        // watcher is used as a fallback in the cases where we fail to access the journal.
        m_useJournal = true;
        if (!m_journalWatcher.Initialize(path))
        {
            m_useJournal = false;
            const Result r = m_dirWatcher.Open(path);
            if (!r)
            {
                HE_LOG_ERROR(he_assets,
                    HE_MSG("Failed to open path for watching."),
                    HE_KV(path, path),
                    HE_KV(result, r));
                return false;
            }
        }

        bool needsFullScan = true;

        if (m_useJournal)
        {
            ConfigModel config;
            const bool found = m_db.Storage().FindOne(config, Where(Col(&ConfigModel::key) == NextUsnConfigKey));
            if (found && config.value.Size() == sizeof(int64_t))
            {
                int64_t startUsn;
                MemCopy(&startUsn, config.value.Data(), sizeof(int64_t));

                m_startJournalMax = m_journalWatcher.NextUsn();

                // If our starting USN is valid we can scan the journal instead of the file system
                if (m_journalWatcher.SetNextUsn(startUsn))
                    needsFullScan = false;
            }
        }

        m_running = true;

        if (needsFullScan)
        {
            m_scanThread = std::thread(std::bind(&AssetDatabaseUpdater::ScanThreadFunc, this));
        }

        if (m_useJournal)
        {
            m_watchThread = std::thread(std::bind(&AssetDatabaseUpdater::JournalWatchThreadFunc, this));
        }
        else
        {
            m_watchThread = std::thread(std::bind(&AssetDatabaseUpdater::FileWatchThreadFunc, this));
        }
        return true;
    }

    void AssetDatabaseUpdater::Stop()
    {
        m_running = false;

        if (m_watchThread.joinable())
            m_watchThread.join();

        if (m_scanThread.joinable())
            m_scanThread.join();
    }

    void AssetDatabaseUpdater::ScanThreadFunc()
    {
        SetCurrentThreadName("[HE] Asset File Scanner");

        const String& assetRoot = m_db.AssetRoot();

        AssetFileScanner scanner(m_db);
        if (!scanner.Run(assetRoot.Data()))
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Asset file scan failed, check the log for additional info."),
                HE_KV(asset_root, assetRoot));
        }

        m_onReadySignal.Dispatch();
    }

    void AssetDatabaseUpdater::FileWatchThreadFunc()
    {
        SetCurrentThreadName("[HE] Asset File Watcher");

        while (m_running)
        {
            DirectoryWatcher::Entry entry;
            const Result r = m_dirWatcher.WaitForEntry(entry, FromPeriod<Seconds>(1));

            const FileWatchResult wait = GetFileWatchResult(r);

            switch (wait)
            {
                case FileWatchResult::Success: HandleFileEntry(entry); break;
                case FileWatchResult::Timeout: break;
                case FileWatchResult::Failure:
                    HE_LOG_ERROR(he_assets, HE_MSG("Failed to wait for file watch entry."), HE_KV(result, r));
                    break;
            }
        }
    }

    void AssetDatabaseUpdater::HandleFileEntry(const DirectoryWatcher::Entry& entry)
    {
        if (GetExtension(entry.path) != AssetFileExtension)
            return;

        if (entry.reason == FileChangeReason::Removed || entry.reason == FileChangeReason::Renamed_OldName)
        {
            m_db.OnAssetFileDeleted(entry.path.Data());
        }
        else if (!m_db.IsFileUpToDate(entry.path.Data()))
        {
            m_db.OnAssetFileUpdated(entry.path.Data());
        }
    }

    void AssetDatabaseUpdater::JournalWatchThreadFunc()
    {
        SetCurrentThreadName("[HE] Asset File Watcher");

        auto journalEntryDelegate = ChangeJournalWatcher::EntryDelegate::Make<&AssetDatabaseUpdater::HandleJournalEntry>(this);

        while (m_running)
        {
            const bool r = m_journalWatcher.GetEntries(journalEntryDelegate);

            if (r)
            {
                ConfigModel config;
                config.key = NextUsnConfigKey;
                config.SetValue(m_journalWatcher.NextUsn());

                const auto query = Insert<ConfigModel>(
                    Cols(&ConfigModel::key, &ConfigModel::value),
                    Values(config.key, config.value),
                    OnConflict(&ConfigModel::key).DoUpdate(Set(&ConfigModel::value, Excluded(&ConfigModel::value))));

                m_db.Storage().Execute(query);

                if (m_startJournalMax != 0 && m_journalWatcher.NextUsn() >= m_startJournalMax)
                {
                    m_startJournalMax = 0;
                    m_onReadySignal.Dispatch();
                }
            }
        }
    }

    void AssetDatabaseUpdater::HandleJournalEntry(const ChangeJournalWatcher::Entry& entry)
    {
        if (!IsChildPath(entry.path, m_db.AssetRoot()))
            return;

        if (GetExtension(entry.path) != AssetFileExtension)
            return;

        if (HasAnyFlags(entry.reasonFlags, ChangeJournalReasonFlag::Removed | ChangeJournalReasonFlag::Renamed_OldName))
        {
            m_db.OnAssetFileDeleted(entry.path.Data());
        }
        else if (!m_db.IsFileUpToDate(entry.path.Data()))
        {
            m_db.OnAssetFileUpdated(entry.path.Data());
        }
    }
}
