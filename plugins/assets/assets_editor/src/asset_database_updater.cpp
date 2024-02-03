// Copyright Chad Engler

#include "he/assets/asset_database_updater.h"

#include "he/assets/asset_models.h"
#include "he/assets/types.h"
#include "he/core/assert.h"
#include "he/core/delegate.h"
#include "he/core/directory.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/random.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/core/thread.h"
#include "he/core/types.h"
#include "he/sqlite/orm.h"
#include "he/sqlite/orm_storage.h"

using namespace he::sqlite;

namespace he::assets
{
    constexpr StringView NextUsnConfigKey = "he.assets.next_usn";

    bool AssetDatabaseUpdater::Start()
    {
        if (!HE_VERIFY(!m_running))
            return true;

        GetSecureRandomBytes(reinterpret_cast<uint8_t*>(&m_scanToken), sizeof(m_scanToken));

        const Span<const String> paths = m_db.ContentRoots();
        for (const String& path : paths)
        {
            DirectoryWatcher& watcher = m_dirWatchers.EmplaceBack();
            const Result rc = watcher.Open(path.Data());
            if (!HE_VERIFY(rc,
                HE_MSG("Failed to open path for watching."),
                HE_KV(path, path),
                HE_KV(result, r)))
            {
                Stop();
                return false;
            }
        }

        m_running = true;

        Result rc = m_scanThread.Start({ &AssetDatabaseUpdater::ScanThreadFunc, this });
        if (!HE_VERIFY(rc,
            HE_MSG("Failed to create asset scan thread."),
            HE_KV(result, rc)))
        {
            Stop();
            return false;
        }

        rc = m_watchThread.Start({ &AssetDatabaseUpdater::FileWatchThreadFunc, this });
        if (!HE_VERIFY(rc,
            HE_MSG("Failed to create asset watch thread."),
            HE_KV(result, rc)))
        {
            Stop();
            return false;
        }

        return true;
    }

    void AssetDatabaseUpdater::Stop()
    {
        m_running = false;

        if (m_watchThread.IsJoinable())
            m_watchThread.Join();

        if (m_scanThread.IsJoinable())
            m_scanThread.Join();

        m_dirWatchers.Clear();
    }

    void AssetDatabaseUpdater::ScanThreadFunc(void* instance)
    {
        Thread::SetName("[HE] Asset File Scanner");

        AssetDatabaseUpdater* updater = static_cast<AssetDatabaseUpdater*>(instance);

        const Span<const String> paths = updater->m_db.ContentRoots();

        Vector<String> directoriesToScan;
        directoriesToScan = paths;

        String fileData;
        fileData.Reserve(8192);

        String fullPath;
        fullPath.Reserve(1024);

        while (!directoriesToScan.IsEmpty())
        {
            String dirPath = Move(directoriesToScan.Back());
            directoriesToScan.PopBack();

            DirectoryScanner scanner;

            const Result r = scanner.Open(dirPath.Data());
            if (!r)
            {
                HE_LOG_ERROR(he_assets,
                    HE_MSG("Failed to open directory for asset scan."),
                    HE_KV(directory_name, dirPath),
                    HE_KV(result, r));
                continue;
            }

            DirectoryScanner::Entry entry;
            while (scanner.NextEntry(entry))
            {
                fullPath = dirPath;
                ConcatPath(fullPath, entry.name);

                if (entry.isDirectory)
                {
                    directoriesToScan.PushBack(fullPath);
                    continue;
                }

                const StringView ext = GetExtension(fullPath);
                if (ext != AssetFileExtension)
                    continue;

                if (!updater->m_db.UpdateAssetFile(fullPath.Data()))
                {
                    HE_LOG_ERROR(he_assets,
                        HE_MSG("Failed to update database entries for asset file."),
                        HE_KV(asset_file_path, fullPath.Data()));
                    continue;
                }

                const auto query = sqlite::Update(
                    sqlite::Where(sqlite::Col(&AssetFileModel::filePath) == fullPath),
                    sqlite::Set(&AssetFileModel::scanToken, m_scanToken));

                if (!updater->m_db.Storage().Execute(query))
                {
                    HE_LOG_ERROR(he_assets,
                        HE_MSG("Failed to update asset in DB to latest scan token. Asset metadata may be removed from DB."),
                        HE_KV(asset_file_path, fullPath),
                        HE_KV(result, r));
                    continue;
                }
            }
        }

        m_onReadySignal.Dispatch();
    }

    void AssetDatabaseUpdater::FileWatchThreadFunc(void* instance)
    {
        Thread::SetName("[HE] Asset File Watcher");

        AssetDatabaseUpdater* updater = static_cast<AssetDatabaseUpdater*>(instance);

        while (updater->m_running)
        {
            for (DirectoryWatcher& watcher : updater->m_dirWatchers)
            {
                DirectoryWatcher::Event evt;
                const Result r = watcher.WaitForEvent(evt, Duration_Zero);
                const DirectoryWatchResult waitResult = GetDirectoryWatchResult(r);

                switch (waitResult)
                {
                    case DirectoryWatchResult::Success:
                    {
                        if (GetExtension(evt.path) != AssetFileExtension)
                            return;

                        if (evt.reason == FileChangeReason::Removed || evt.reason == FileChangeReason::Renamed_OldName)
                        {
                            updater->m_db.OnAssetFileDeleted(evt.path.Data());
                        }
                        else
                        {
                            updater->m_db.OnAssetFileUpdated(evt.path.Data());
                        }
                        break;
                    }
                    case DirectoryWatchResult::Timeout:
                    {
                        // Timeout is fine, just means we didn't get any events
                        break;
                    }
                    case DirectoryWatchResult::Failure:
                    {
                        HE_LOG_ERROR(he_assets, HE_MSG("Failed to get file watch event status."), HE_KV(result, r));
                        break;
                    }
                }
            }

            if (m_running)
            {
                Thread::Sleep(FromPeriod<Seconds>(1));
            }
        }
    }
}
