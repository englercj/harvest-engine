// Copyright Chad Engler

#include "he/assets/asset_database_updater.h"

#include "he/assets/asset_file_scanner.h"
#include "he/assets/asset_models.h"
#include "he/core/assert.h"
#include "he/core/clock.h"
#include "he/core/delegate.h"
#include "he/core/file.h"
#include "he/core/directory_watcher.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/core/thread.h"
#include "he/core/types.h"
#include "he/core/vector.h"
#include "he/core/wstr.h"

#include <thread>

#if defined(HE_PLATFORM_API_WIN32)

#include "change_journal_watcher.win32.h"

#include <Windows.h>

namespace he::assets
{
    constexpr char NextUsnConfigKey[] = "he.assets.next_usn";

    class AssetDatabaseUpdaterImpl final : public AssetDatabaseUpdater
    {
    public:
        AssetDatabaseUpdaterImpl(AssetDatabase& db) : AssetDatabaseUpdater(db) {}

        bool Start() override
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
                if (ConfigModel::FindOne(m_db, NextUsnConfigKey, config) && config.value.Size() == sizeof(int64_t))
                {
                    int64_t startUsn;
                    MemCopy(&startUsn, config.value.Data(), sizeof(int64_t));

                    // If our starting USN is valid we can scan the journal instead of the file system
                    if (m_journalWatcher.SetNextUsn(startUsn))
                        needsFullScan = false;
                }
            }

            m_running = true;

            if (needsFullScan)
            {
                m_scanThread = std::thread(std::bind(&AssetDatabaseUpdaterImpl::ScanThreadFunc, this));
            }

            if (m_useJournal)
            {
                m_watchThread = std::thread(std::bind(&AssetDatabaseUpdaterImpl::JournalWatchThreadFunc, this));
            }
            else
            {
                m_watchThread = std::thread(std::bind(&AssetDatabaseUpdaterImpl::FileWatchThreadFunc, this));
            }
            return true;
        }

        void Stop() override
        {
            m_running = false;

            if (m_watchThread.joinable())
                m_watchThread.join();

            if (m_scanThread.joinable())
                m_scanThread.join();
        }

    private:
        void ScanThreadFunc()
        {
            SetCurrentThreadName("[HE] Asset File Scanner");
            AssetFileScanner scanner(m_db);
            scanner.Run(m_db.AssetRoot().Data());
        }

        void FileWatchThreadFunc()
        {
            SetCurrentThreadName("[HE] Asset File Watcher");

            while (m_running)
            {
                DirectoryWatcher::Entry entry;
                const Result r = m_dirWatcher.WaitForEntry(entry, FromPeriod<Seconds>(1));

                FileWatchResult wait = GetFileWatchResult(r);

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

        void HandleFileEntry(const DirectoryWatcher::Entry& entry)
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

        void JournalWatchThreadFunc()
        {
            SetCurrentThreadName("[HE] Asset File Watcher");

            auto journalEntryDelegate = ChangeJournalWatcher::EntryDelegate::Make<&AssetDatabaseUpdaterImpl::HandleJournalEntry>(this);

            while (m_running)
            {
                const bool r = m_journalWatcher.GetEntries(journalEntryDelegate);

                if (r)
                {
                    ConfigModel config;
                    config.key = NextUsnConfigKey;
                    config.SetValue(m_journalWatcher.NextUsn());

                    ConfigModel::AddOrUpdate(m_db, config);
                }
            }
        }

        void HandleJournalEntry(const ChangeJournalWatcher::Entry& entry)
        {
            if (!IsChildPath(entry.path, m_db.AssetRoot()))
                return;

            if (GetExtension(entry.path) != AssetFileExtension)
                return;

            if (HasAnyFlags(entry.reasonFlags, USN_REASON_FILE_DELETE | USN_REASON_RENAME_OLD_NAME))
            {
                m_db.OnAssetFileDeleted(entry.path.Data());
            }
            else if (!m_db.IsFileUpToDate(entry.path.Data()))
            {
                m_db.OnAssetFileUpdated(entry.path.Data());
            }
        }

    private:
        ChangeJournalWatcher m_journalWatcher{};
        DirectoryWatcher m_dirWatcher{};

        bool m_running{ false };
        bool m_useJournal{ false };

        std::thread m_watchThread{};
        std::thread m_scanThread{};
    };

    AssetDatabaseUpdater* AssetDatabaseUpdater::Create(AssetDatabase& db)
    {
        return Allocator::GetDefault().New<AssetDatabaseUpdaterImpl>(db);
    }

    void AssetDatabaseUpdater::Destroy(AssetDatabaseUpdater* updater)
    {
        Allocator::GetDefault().Delete(updater);
    }
}

#endif
