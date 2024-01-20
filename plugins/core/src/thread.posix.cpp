// Copyright Chad Engler

#include "he/core/thread.h"

#include "he/core/cpu_info.h"
#include "he/core/string.h"
#include "he/core/utils.h"

#if defined(HE_PLATFORM_API_POSIX)

#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>

#if defined(HE_PLATFORM_LINUX)
    #include <sys/prctl.h>
    #include <sys/syscall.h>
    #include <sys/types.h>
#endif

namespace he
{
    uintptr_t GetCurrentThreadHandle()
    {
        static_assert(sizeof(uintptr_t) >= sizeof(pthread_t));
        static_assert(IsAligned(alignof(uintptr_t) >= alignof(pthread_t)));
        return reinterpret_cast<uintptr_t>(pthread_self());
    }

    uint32_t GetCurrentThreadId()
    {
    #if defined(HE_PLATFORM_LINUX)
        static_assert(sizeof(uint32_t) >= sizeof(pid_t));
        return static_cast<uint32_t>(syscall(SYS_gettid));
    #else
        static_assert(sizeof(uint32_t) >= sizeof(pthread_id_np_t));
        return static_cast<uint32_t>(pthread_getthreadid_np());
    #endif
    }

    Result SetThreadAffinity(uintptr_t thread, uint64_t mask)
    {
        static_assert(__CPU_SETSIZE >= (sizeof(mask) * 8));

        const CpuInfo& info = GetCpuInfo();
        const uint32_t len = Min(info.threadCount, static_cast<uint32_t>(sizeof(mask) * 8));

        cpu_set_t set;
        CPU_ZERO(&set);

        bool anySet = false;
        for (uint32_t i = 0; i < len; ++i)
        {
            const uint64_t flag = 1 << i;
            if ((mask & flag) != 0)
            {
                anySet = true;
                CPU_SET(i, &set);
            }
        }

        if (!HE_VERIFY(anySet, HE_MSG("Affinity mask shouldn't be zero.")))
            return Result::InvalidParameter;

        const pthread_t th = static_cast<pthread_t>(thread);
        const int rc = pthread_setaffinity_np(th, sizeof(set), &set);
        return PosixResult(rc);
    }

    void SetCurrentThreadName(const char* name)
    {
    #if defined(HE_PLATFORM_LINUX)
        prctl(PR_SET_NAME, name, 0, 0, 0);
    #else
        pthread_setname_np(pthread_self(), name);
    #endif
    }

    void SleepCurrentThread(Duration amount)
    {
        const timespec req = PosixTimeFromDuration(amount);
        timespec rem = { 0, 0 };
        nanosleep(&req, &rem);
    }

    void YieldCurrentThread()
    {
        sched_yield();
    }
}

#endif
