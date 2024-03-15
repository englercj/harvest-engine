// Copyright Chad Engler

#include "he/core/process.h"

#include "he/core/compiler.h"
#include "he/core/string.h"

#if defined(HE_PLATFORM_API_WASM)

#include "he/core/wasm/lib_core.wasm.h"

namespace he
{
    Result GetEnv([[maybe_unused]] const char* name, [[maybe_unused]] String& outValue)
    {
        // TODO: Should we implement some kind of env storage?
        return Result::NotSupported;
    }

    Result SetEnv([[maybe_unused]] const char* name, [[maybe_unused]] const char* value)
    {
        // TODO: Should we implement some kind of env storage?
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

    Result GetCurrentProcessFilename([[maybe_unused]] String& out)
    {
        // No such thing as process names in wasm.
        return Result::NotSupported;
    }

    [[noreturn]] void TerminateProcess()
    {
        // Cause an unconditional trap to terminate the process.
        HE_UNREACHABLE();
    }
}

#endif
