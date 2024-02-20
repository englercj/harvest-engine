// Copyright Chad Engler

#include "he/core/process.h"

#if defined(HE_PLATFORM_WASM)

#include "he/core/wasm/lib_core.wasm.h"

namespace he
{
    Result GetEnv(const char* name, String& outValue)
    {
        // TODO: Should we implement some kind of env storage?
        HE_UNUSED(name, outValue);
        return Result::NotSupported;
    }

    Result SetEnv(const char* name, const char* value)
    {
        // TODO: Should we implement some kind of env storage?
        HE_UNUSED(name, value);
        return Result::NotSupported;
    }

    uint32_t GetCurrentProcessId()
    {
        // Arbitrary value to identify our process.
        return 1u;
    }

    bool IsProcessRunning(uint32_t pid)
    {
        // We're the only process running as far as we know.
        return pid == GetCurrentProcessId();
    }

    Result GetCurrentProcessFilename(String& out)
    {
        // No such thing as process names in wasm.
        return Result::NotSupported;
    }

    [[noreturn]] void TerminateProcess()
    {
        // Cause an unconditional trap to terminate the process.
        __asm__ volatile("unreachable");
    }
}

#endif
