// Copyright Chad Engler

#pragma once

#include "he/core/clock.h"
#include "he/core/delegate.h"
#include "he/core/enum_ops.h"
#include "he/core/string.h"
#include "he/core/path.h"

#if defined(HE_PLATFORM_API_WIN32)


namespace he::assets
{
    enum class ChangeJournalReasonFlag : uint32_t
    {
        None            = 0,
        Added           = 1 << 0,
        Removed         = 1 << 1,
        Modified        = 1 << 2,
        Renamed_OldName = 1 << 3,
        Renamed_NewName = 1 << 4,
    };
    HE_ENUM_FLAGS(ChangeJournalReasonFlag);

    class ChangeJournalWatcher final
    {
    public:
        struct Entry
        {
            String path{};
            int64_t usn{ 0 };
            SystemTime timestamp{ 0 };
            uint32_t attributeFlags{ 0 };
            ChangeJournalReasonFlag reasonFlags{ 0 };
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
        uint8_t* m_buffer{ nullptr };

        void* m_volume{ nullptr };
        uint64_t m_journalId{ 0 };
        int64_t m_lowestUsn{ 0 };
        int64_t m_nextUsn{ 0 };
    };
}

#endif
