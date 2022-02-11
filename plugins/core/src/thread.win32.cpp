// Copyright Chad Engler

#include "he/core/thread.h"

#include "he/core/wstr.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

namespace he
{
    static_assert(sizeof(ThreadHandle) >= sizeof(HANDLE));
    static_assert(alignof(ThreadHandle) >= alignof(HANDLE));

    ThreadHandle GetCurrentThreadHandle()
    {
        return reinterpret_cast<ThreadHandle>(::GetCurrentThread());
    }

    Result SetThreadAffinity(ThreadHandle thread, uint64_t mask)
    {
        DWORD_PTR processMask = 0;
        DWORD_PTR systemMask = 0;
        if (!::GetProcessAffinityMask(::GetCurrentProcess(), &processMask, &systemMask))
            return Result::FromLastError();

        mask &= processMask;
        if (!::SetThreadAffinityMask(reinterpret_cast<HANDLE>(thread), mask))
            return Result::FromLastError();

        return Result::Success;
    }

    void SetCurrentThreadName(const char* name)
    {
        ::SetThreadDescription(::GetCurrentThread(), HE_TO_WSTR(name));
    }
}

#endif
