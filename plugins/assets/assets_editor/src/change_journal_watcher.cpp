// Copyright Chad Engler

#include "he/assets/change_journal_watcher.h"

#if !defined(HE_PLATFORM_WINDOWS)

namespace he::assets
{
    bool ChangeJournalWatcher::Initialize(const char* path)
    {
        HE_UNUSED(path);
        return false;
    }

    void ChangeJournalWatcher::Terminate()
    {

    }

    bool ChangeJournalWatcher::SetNextUsn(int64_t usn)
    {
        HE_UNUSED(usn);
        return false;
    }

    bool ChangeJournalWatcher::GetEntries(EntryDelegate iterator)
    {
        HE_UNUSED(iterator);
        return false;
    }
}

#endif
