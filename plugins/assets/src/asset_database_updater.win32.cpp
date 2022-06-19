// Copyright Chad Engler

#include "he/assets/asset_database_updater.h"

#include "he/assets/asset_file_scanner.h"
#include "he/assets/asset_models.h"
#include "he/core/assert.h"
#include "he/core/clock.h"
#include "he/core/delegate.h"
#include "he/core/file.h"
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

#include <Shlwapi.h>
#include <Windows.h>

namespace he::assets
{
    constexpr char NextUsnConfigKey[] = "he.assets.next_usn";

    struct JournalEntry
    {
        String path;
        int64_t usn;
        SystemTime timestamp;
        uint32_t attributeFlags;
        uint32_t reasonFlags;
    };

    using JournalEntryDelegate = Delegate<void(const JournalEntry&)>;

    class JournalWatcher
    {
    public:
        static constexpr uint32_t BufferSize = 4096;
        static constexpr DWORD ReasonMask = USN_REASON_DATA_EXTEND | USN_REASON_DATA_OVERWRITE | USN_REASON_DATA_TRUNCATION | USN_REASON_FILE_CREATE | USN_REASON_FILE_DELETE | USN_REASON_RENAME_NEW_NAME | USN_REASON_RENAME_OLD_NAME;

    public:
        JournalWatcher() = default;
        ~JournalWatcher() { Terminate(); }

        bool Initialize(const char* path)
        {
            Terminate();

            auto guard = MakeScopeGuard([&]() { Terminate(); });

            const int num = ::PathGetDriveNumberW(HE_TO_WSTR(path));
            if (num == -1)
            {
                HE_LOG_ERROR(he_assets,
                    HE_MSG("Failed to get volume number for path."),
                    HE_KV(path, path),
                    HE_KV(result, Result::FromLastError()));
                return false;
            }

            wchar_t root[4];
            ::PathBuildRootW(root, num);

            wchar_t vol[] = L"\\\\.\\X:";
            vol[4] = root[0];
            m_volume = ::CreateFileW(
                vol,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL);

            if (m_volume == INVALID_HANDLE_VALUE)
            {
                String temp;
                WCToMBStr(temp, vol);
                HE_LOG_ERROR(he_assets,
                    HE_MSG("Failed to open volume handle for USN journal."),
                    HE_KV(path, path),
                    HE_KV(volume, temp),
                    HE_KV(result, Result::FromLastError()));
                return false;
            }

            USN_JOURNAL_DATA_V2 journalData{};
            DWORD bytesReturned = 0;
            BOOL res = ::DeviceIoControl(
                m_volume,
                FSCTL_QUERY_USN_JOURNAL,
                nullptr,
                0,
                &journalData,
                sizeof(journalData),
                &bytesReturned,
                nullptr);

            if (!res || bytesReturned != sizeof(journalData))
            {
                String temp;
                WCToMBStr(temp, vol);
                HE_LOG_ERROR(he_assets,
                    HE_MSG("Failed to query initial USN journal data."),
                    HE_KV(path, path),
                    HE_KV(volume, temp),
                    HE_KV(bytes_returned, bytesReturned),
                    HE_KV(result, Result::FromLastError()));
                return false;
            }

            // Records are written to 64-bit aligned locations, so we align to 8-bytes.
            // This ensures that `m_buffer + sizeof(USN)` will be 8-byte aligned as well.
            static_assert(sizeof(USN) == 8);
            m_buffer = static_cast<uint8_t*>(Allocator::GetDefault().Malloc(BufferSize, 8));

            m_journalId = journalData.UsnJournalID;
            m_lowestUsn = journalData.LowestValidUsn;
            m_nextUsn = journalData.NextUsn;

            guard.Dismiss();
            return true;
        }

        void Terminate()
        {
            if (m_volume != INVALID_HANDLE_VALUE)
            {
                ::CloseHandle(m_volume);
                m_volume = INVALID_HANDLE_VALUE;
            }

            Allocator::GetDefault().Free(m_buffer);
            m_buffer = nullptr;
        }

        int64_t NextUsn() const { return m_nextUsn; }

        bool SetNextUsn(int64_t usn)
        {
            if (usn < m_lowestUsn)
            {
                HE_LOG_ERROR(he_assets,
                    HE_MSG("Failed to set journal next USN. Requested USN is lower than the lowest valid USN."),
                    HE_KV(usn, usn),
                    HE_KV(lowest_valid_usn, m_lowestUsn));
                return false;
            }

            m_nextUsn = usn;
            return true;
        }

        bool GetEntries(JournalEntryDelegate iterator)
        {
            HE_ASSERT(m_buffer);

            JournalEntry entry;

            DWORD bytesReturned = 0;
            if (!ReadRecords(bytesReturned))
                return false;

            const uint8_t* end = m_buffer + bytesReturned;
            const uint8_t* p = m_buffer + sizeof(USN);

            while (p < end)
            {
                const USN_RECORD* record = reinterpret_cast<const USN_RECORD*>(p);
                p += record->RecordLength;

                if (p > end || !IsAligned(p, 8) || record->RecordLength <= 0)
                {
                    HE_LOG_ERROR(he_assets, HE_MSG("Got a malformed USN record."));
                    return false;
                }

                if (ParseRecord(record, entry))
                {
                    iterator(entry);
                }
            }

            return true;
        }

    private:
        bool ReadRecords(DWORD& bytesReturned)
        {
            READ_USN_JOURNAL_DATA_V1 readData{};
            readData.StartUsn = m_nextUsn;
            readData.ReasonMask = ReasonMask;
            readData.ReturnOnlyOnClose = false;
            readData.Timeout = 1;
            readData.BytesToWaitFor = BufferSize;
            readData.UsnJournalID = m_journalId;
            readData.MinMajorVersion = 2;
            readData.MaxMajorVersion = 3;

            const BOOL res = ::DeviceIoControl(
                m_volume,
                FSCTL_READ_USN_JOURNAL,
                &readData,
                sizeof(readData),
                m_buffer,
                BufferSize,
                &bytesReturned,
                nullptr);

            if (res == 0 || bytesReturned < sizeof(USN))
            {
                HE_LOG_ERROR(he_assets,
                    HE_MSG("Failed to read USN journal records."),
                    HE_KV(bytes_returned, bytesReturned),
                    HE_KV(result, Result::FromLastError()));
                return false;
            }

            m_nextUsn = *reinterpret_cast<USN*>(m_buffer);
            return true;
        }

        template <typename T>
        static bool ReadFileName(String& dst, const T* record)
        {
            const uint8_t* recordBegin = reinterpret_cast<const uint8_t*>(record);
            const uint8_t* recordEnd = recordBegin + record->RecordLength;

            const uint8_t* fnameBegin = recordBegin + record->FileNameOffset;
            const uint8_t* fnameEnd = fnameBegin + record->FileNameLength;

            if (fnameEnd > recordEnd)
            {
                HE_LOG_ERROR(he_assets,
                    HE_MSG("Got a malformed USN record. File name length is invalid, ignoring."),
                    HE_KV(file_name_length, record->FileNameLength),
                    HE_KV(record_length, record->RecordLength),
                    HE_KV(record_struct_size, sizeof(T)));
                return false;
            }

            const wchar_t* fname = reinterpret_cast<const wchar_t*>(fnameBegin);
            const uint32_t fnameLen = record->FileNameLength / sizeof(wchar_t);
            WCToMBStr(dst, fname, fnameLen);
            return true;
        }

        bool ParseRecord(const USN_RECORD* record, JournalEntry& entry)
        {
            switch (record->MajorVersion)
            {
                case 2:
                {
                    const USN_RECORD_V2* recordV2 = reinterpret_cast<const USN_RECORD_V2*>(record);
                    entry.usn = recordV2->Reason;
                    entry.timestamp = Win32FileTimeToSystemTime(recordV2->TimeStamp.QuadPart);
                    entry.attributeFlags = recordV2->FileAttributes;
                    entry.reasonFlags = recordV2->Reason;
                    return ReadFileName(entry.path, recordV2);
                }
                case 3:
                {
                    const USN_RECORD_V3* recordV3 = reinterpret_cast<const USN_RECORD_V3*>(record);
                    entry.usn = recordV3->Reason;
                    entry.timestamp = Win32FileTimeToSystemTime(recordV3->TimeStamp.QuadPart);
                    entry.attributeFlags = recordV3->FileAttributes;
                    entry.reasonFlags = recordV3->Reason;
                    return ReadFileName(entry.path, recordV3);
                }
                case 4:
                {
                    const USN_RECORD_V4* recordV4 = reinterpret_cast<const USN_RECORD_V4*>(record);
                    HE_LOG_TRACE(he_assets,
                        HE_MSG("Encountered v4 USN record (range tracking), ignoring."),
                        HE_KV(major_version, recordV4->Header.MajorVersion),
                        HE_KV(minor_version, recordV4->Header.MinorVersion),
                        HE_KV(usn, recordV4->Usn),
                        HE_KV(reason, recordV4->Reason));
                    // v4 records are valid, but aren't interesting for us. Return false to skip it.
                    return false;
                }
            }

            HE_LOG_WARN(he_assets,
                HE_MSG("Encountered unknown USN record version, ignoring."),
                HE_KV(major_version, record->MajorVersion),
                HE_KV(minor_version, record->MinorVersion));
            return false;
        }

    private:
        uint8_t* m_buffer{ nullptr };

        HANDLE m_volume{ INVALID_HANDLE_VALUE };
        DWORDLONG m_journalId{ 0 };
        USN m_lowestUsn{ 0 };
        USN m_nextUsn{ 0 };
    };

    class AssetDatabaseUpdaterImpl final : public AssetDatabaseUpdater
    {
    public:
        AssetDatabaseUpdaterImpl(AssetDatabase& db) : AssetDatabaseUpdater(db) {}

        bool Start() override
        {
            if (!m_watcher.Initialize(m_db.AssetRoot().Data()))
                return false;

            bool needsFullScan = true;

            ConfigModel config;
            if (ConfigModel::FindOne(m_db, NextUsnConfigKey, config) && config.value.Size() == sizeof(int64_t))
            {
                int64_t startUsn;
                MemCopy(&startUsn, config.value.Data(), sizeof(int64_t));

                // If our starting USN is valid we can scan the journal instead of the file system
                if (m_watcher.SetNextUsn(startUsn))
                    needsFullScan = false;
            }

            m_running = true;

            if (needsFullScan)
            {
                m_scanThread = std::thread(std::bind(&AssetDatabaseUpdaterImpl::ScanThreadFunc, this));
            }

            m_watchThread = std::thread(std::bind(&AssetDatabaseUpdaterImpl::WatchThreadFunc, this));
            return true;
        }

        void Stop() override
        {
            m_watchThread.join();

            if (m_scanThread.joinable())
                m_scanThread.join();
        }

    private:
        void ScanThreadFunc()
        {
            SetCurrentThreadName("Asset File Scanner");
            AssetFileScanner scanner(m_db);
            scanner.Run(m_rootDir.Data());
        }

        void WatchThreadFunc()
        {
            SetCurrentThreadName("Asset File Watcher");

            while (m_running)
            {
                auto delegate = JournalEntryDelegate::Make<&AssetDatabaseUpdaterImpl::HandleEntry>(this);
                const bool r = m_watcher.GetEntries(delegate);

                if (r)
                {
                    ConfigModel config;
                    config.key = NextUsnConfigKey;
                    config.SetValue(m_watcher.NextUsn());

                    ConfigModel::AddOrUpdate(m_db, config);
                }
            }
        }

        void HandleEntry(const JournalEntry& entry)
        {
            if (!IsChildPath(entry.path, m_rootDir))
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
        String m_rootDir{};
        JournalWatcher m_watcher{};

        bool m_running{ false };

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
