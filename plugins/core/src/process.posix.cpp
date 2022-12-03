// Copyright Chad Engler

#include "he/core/process.h"

#include "he/core/alloca.h"
#include "he/core/allocator.h"
#include "he/core/wstr.h"

#if defined(HE_PLATFORM_API_POSIX)

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

namespace he
{
    constexpr uint32_t MaxStackLen = 2048;

    Result GetEnv(const char* name, String& outValue)
    {
        const char* v = getenv(name);
        if (v == nullptr)
        {
            outValue.Clear();
            return PosixResult(ENOENT);
        }

        outValue = v;
        return Result::Success;
    }

    Result SetEnv(const char* name, const char* value)
    {
        const int rc = value ? setenv(name, value, 1) : unsetenv(name);
        if (rc < 0)
            return Result::FromLastError();

        return Result::Success;
    }

    uint32_t GetCurrentProcessId()
    {
        return getpid();
    }

    bool IsProcessRunning(uint32_t pid)
    {
        const int rc = kill(static_cast<pid_t>(pid), 0);
        return rc == 0;
    }
}

#endif
