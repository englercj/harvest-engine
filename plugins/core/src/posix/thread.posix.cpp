// Copyright Chad Engler

#include "he/core/thread.h"

#include "he/core/cpu_info.h"
#include "he/core/scope_guard.h"
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
    static bool _MakeCpuSet(cpu_set_t& set, uint64_t mask)
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

        return anySet;
    }

    struct _ThreadThunkData
    {
        Pfn_ThreadProc proc;
        void* userData;
    };

    void* _ThreadThunk(void* data)
    {
        _ThreadThunkData* thunkData = static_cast<_ThreadThunkData*>(data);
        thunkData->proc(thunkData->userData);
        Allocator::GetDefault().Delete(thunkData);
        return 0;
    }

    uint32_t Thread::GetId()
    {
    #if defined(HE_PLATFORM_LINUX)
        static_assert(sizeof(uint32_t) >= sizeof(pid_t));
        return static_cast<uint32_t>(syscall(SYS_gettid));
    #else
        static_assert(sizeof(uint32_t) >= sizeof(pthread_id_np_t));
        return static_cast<uint32_t>(pthread_getthreadid_np());
    #endif
    }

    void Thread::SetName(const char* name)
    {
    #if defined(HE_PLATFORM_LINUX)
        prctl(PR_SET_NAME, name, 0, 0, 0);
    #else
        pthread_setname_np(pthread_self(), name);
    #endif
    }

    void Thread::Sleep(Duration amount)
    {
        const timespec req = PosixTimeFromDuration(amount);
        timespec rem = { 0, 0 };
        nanosleep(&req, &rem);
    }

    void Thread::Yield()
    {
        sched_yield();
    }

    Result Thread::Start(const ThreadDesc& desc)
    {
        if (!HE_VERIFY(desc.proc != nullptr, HE_MSG("Thread procedure must be non-null.")))
            return Result::InvalidParameter;

        if (!HE_VERIFY(!m_handle, HE_MSG("Thread is already running.")))
            return Result::InvalidParameter;

        pthread_attr_t attr;
        pthread_attr_init(&attr);
        HE_AT_SCOPE_EXIT([&] { pthread_attr_destroy(&attr); });

        if (desc.stackSize > 0)
        {
            const int rc = pthread_attr_setstacksize(&attr, desc.stackSize);
            if (rc != 0)
                return PosixResult(rc);
        }

        if (desc.affinity > 0)
        {
            cpu_set_t set;
            if (!HE_VERIFY(_MakeCpuSet(set, mask), HE_MSG("Affinity mask shouldn't be zero.")))
                return Result::InvalidParameter;

            const int rc = pthread_attr_setaffinity_np(&attr, sizeof(set), &set);
            if (rc != 0)
                return PosixResult(rc);
        }

        _ThreadThunkData* thunkData = Allocator::GetDefault().New<_ThreadThunkData>(desc.proc, desc.data);

        pthread_t th;
        int rc = pthread_create(&th, &attr, _ThreadThunk, thunkData);

        if (rc != 0)
        {
            Allocator::GetDefault().Delete(thunkData);
            return PosixResult(rc);
        }

        m_handle = reinterpret_cast<void*>(th);
        return Result::Success;
    }

    Result Thread::Join()
    {
        const pthread_t th = reinterpret_cast<pthread_t>(m_handle);
        const int rc = pthread_join(th, nullptr);
        return PosixResult(rc);
    }

    Result Thread::Detach()
    {
        const pthread_t th = reinterpret_cast<pthread_t>(m_handle);
        const int rc = pthread_detach(th);
        return PosixResult(rc);
    }

    Result Thread::SetAffinity(uint64_t mask)
    {
        cpu_set_t set;
        if (!HE_VERIFY(_MakeCpuSet(set, mask), HE_MSG("Affinity mask shouldn't be zero.")))
            return Result::InvalidParameter;

        const pthread_t th = reinterpret_cast<pthread_t>(m_handle);
        const int rc = pthread_setaffinity_np(th, sizeof(set), &set);
        return PosixResult(rc);
    }
}

#endif
