// Copyright Chad Engler

#pragma once

#include "he/core/clock.h"
#include "he/core/delegate.h"
#include "he/core/string.h"
#include "he/core/path.h"

#if defined(HE_PLATFORM_API_WIN32)

#include <Shlwapi.h>
#include <Windows.h>

namespace he::assets
{
    class ChangeJournalWatcher
    {
    public:
        static constexpr uint32_t BufferSize = 4096;
        static constexpr DWORD ReasonMask = USN_REASON_DATA_EXTEND | USN_REASON_DATA_OVERWRITE | USN_REASON_DATA_TRUNCATION | USN_REASON_FILE_CREATE | USN_REASON_FILE_DELETE | USN_REASON_RENAME_NEW_NAME | USN_REASON_RENAME_OLD_NAME;

        struct Entry
        {
            String path{};
            int64_t usn{ 0 };
            SystemTime timestamp{ 0 };
            uint32_t attributeFlags{ 0 };
            uint32_t reasonFlags{ 0 };
        };

        using EntryDelegate = Delegate<void(const Entry&)>;

    public:
        ChangeJournalWatcher() = default;
        ~ChangeJournalWatcher() { Terminate(); }

        bool Initialize(const char* path);

        void Terminate();

        int64_t NextUsn() const { return m_nextUsn; }

        bool SetNextUsn(int64_t usn);

        bool GetEntries(EntryDelegate iterator);

    private:
        bool ReadRecords(DWORD& bytesReturned);
        bool ParseRecord(const USN_RECORD* record, Entry& entry);

    private:
        uint8_t* m_buffer{ nullptr };

        HANDLE m_volume{ INVALID_HANDLE_VALUE };
        DWORDLONG m_journalId{ 0 };
        USN m_lowestUsn{ 0 };
        USN m_nextUsn{ 0 };
    };
}

#endif
