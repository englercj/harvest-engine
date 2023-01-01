// Copyright Chad Engler

#include "he/assets/change_journal_watcher.h"

#include "he/core/assert.h"
#include "he/core/log.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/wstr.h"

#if defined(HE_PLATFORM_WINDOWS)

#include <Shlwapi.h>
#include <Windows.h>

namespace he::assets
{
    constexpr uint32_t BufferSize = 4096;
    constexpr DWORD ReasonMask = USN_REASON_DATA_EXTEND | USN_REASON_DATA_OVERWRITE | USN_REASON_DATA_TRUNCATION | USN_REASON_FILE_CREATE | USN_REASON_FILE_DELETE | USN_REASON_RENAME_NEW_NAME | USN_REASON_RENAME_OLD_NAME;

    ChangeJournalReasonFlag ToChangeJournalReasonFlags(DWORD reasons)
    {
        ChangeJournalReasonFlag flags = ChangeJournalReasonFlag::None;

        if (HasFlag(reasons, USN_REASON_FILE_CREATE))
            flags |= ChangeJournalReasonFlag::Added;

        if (HasFlag(reasons, USN_REASON_FILE_DELETE))
            flags |= ChangeJournalReasonFlag::Removed;

        if (HasFlag(reasons, USN_REASON_DATA_EXTEND) || HasFlag(reasons, USN_REASON_DATA_OVERWRITE) || HasFlag(reasons, USN_REASON_DATA_TRUNCATION))
            flags |= ChangeJournalReasonFlag::Modified;

        if (HasFlag(reasons, USN_REASON_RENAME_NEW_NAME))
            flags |= ChangeJournalReasonFlag::Renamed_OldName;

        if (HasFlag(reasons, USN_REASON_RENAME_OLD_NAME))
            flags |= ChangeJournalReasonFlag::Renamed_NewName;

        return flags;
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

    static bool ReadUsnRecords(HANDLE volume, USN startUsn, DWORDLONG journalId, uint8_t* buffer, DWORD& bytesReturned)
    {
        READ_USN_JOURNAL_DATA_V1 readData{};
        readData.StartUsn = startUsn;
        readData.ReasonMask = ReasonMask;
        readData.ReturnOnlyOnClose = false;
        readData.Timeout = 1;
        readData.BytesToWaitFor = BufferSize;
        readData.UsnJournalID = journalId;
        readData.MinMajorVersion = 2;
        readData.MaxMajorVersion = 3;

        const BOOL res = ::DeviceIoControl(
            volume,
            FSCTL_READ_USN_JOURNAL,
            &readData,
            sizeof(readData),
            buffer,
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

        return true;
    }

    static bool ParseUsnRecord(const USN_RECORD* record, ChangeJournalWatcher::Entry& entry)
    {
        switch (record->MajorVersion)
        {
            case 2:
            {
                const USN_RECORD_V2* recordV2 = reinterpret_cast<const USN_RECORD_V2*>(record);
                entry.usn = recordV2->Reason;
                entry.timestamp = Win32FileTimeToSystemTime(recordV2->TimeStamp.QuadPart);
                entry.attributeFlags = recordV2->FileAttributes;
                entry.reasonFlags = ToChangeJournalReasonFlags(recordV2->Reason);
                return ReadFileName(entry.path, recordV2);
            }
            case 3:
            {
                const USN_RECORD_V3* recordV3 = reinterpret_cast<const USN_RECORD_V3*>(record);
                entry.usn = recordV3->Reason;
                entry.timestamp = Win32FileTimeToSystemTime(recordV3->TimeStamp.QuadPart);
                entry.attributeFlags = recordV3->FileAttributes;
                entry.reasonFlags = ToChangeJournalReasonFlags(recordV3->Reason);
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

    bool ChangeJournalWatcher::Initialize(const char* path)
    {
        Terminate();

        auto guard = MakeScopeGuard([&]() { Terminate(); });

        const int num = ::PathGetDriveNumberW(HE_TO_WCSTR(path));
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
            FILE_GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr);

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

    void ChangeJournalWatcher::Terminate()
    {
        if (m_volume != INVALID_HANDLE_VALUE)
        {
            ::CloseHandle(m_volume);
            m_volume = INVALID_HANDLE_VALUE;
        }

        Allocator::GetDefault().Free(m_buffer);
        m_buffer = nullptr;
    }

    bool ChangeJournalWatcher::SetNextUsn(int64_t usn)
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

    bool ChangeJournalWatcher::GetEntries(EntryDelegate iterator)
    {
        HE_ASSERT(m_buffer);

        Entry entry;

        DWORD bytesReturned = 0;
        if (!ReadUsnRecords(m_volume, m_nextUsn, m_journalId, m_buffer, bytesReturned))
            return false;

        m_nextUsn = *reinterpret_cast<USN*>(m_buffer);

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

            if (ParseUsnRecord(record, entry))
            {
                iterator(entry);
            }
        }

        return true;
    }
}

#endif
