// Copyright Chad Engler

#include "he/core/thread.h"

#include "he/core/string.h"
#include "he/core/utils.h"

#if defined(HE_PLATFORM_API_POSIX)

#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include <sys/prctl.h>

namespace he
{
    static_assert(sizeof(ThreadHandle) >= sizeof(pthread_t));
    static_assert(alignof(ThreadHandle) >= alignof(pthread_t));

    ThreadHandle GetCurrentThreadHandle()
    {
        return reinterpret_cast<ThreadHandle>(pthread_self());
    }

    Result SetThreadAffinity(ThreadHandle thread, uint64_t mask)
    {
        cpu_set_t set;
        CPU_ZERO(&set);

        static_assert(__CPU_SETSIZE >= (sizeof(mask) * 8));

        for (uint32_t i = 0; i < (sizeof(mask) * 8); ++i)
        {
            const uint64_t flag = 1 << i;
            if ((mask & flag) != 0)
            {
                CPU_SET(i, &set);
            }
        }

        const int rc = pthread_setaffinity_np(static_cast<pthread_t>(thread), sizeof(set), &set);
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
