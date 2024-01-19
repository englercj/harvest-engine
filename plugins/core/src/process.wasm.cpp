// Copyright Chad Engler

#include "he/core/process.h"


#if defined(HE_PLATFORM_WASM)

#include "wasm_core.js.h"

namespace he
{
    Result GetEnv(const char* name, String& outValue)
    {
        HE_UNUSED(name, outValue);
        return Result::NotSupported;
    }

    Result SetEnv(const char* name, const char* value)
    {
        HE_UNUSED(name, value);
        return Result::NotSupported;
    }

    uint32_t GetCurrentProcessId()
    {
        // Arbitrary value to identify our process.
        return 0x3eb;
    }

    bool IsProcessRunning(uint32_t pid)
    {
        // We're the only process running as far as we know.
        return pid == GetCurrentProcessId();
    }

    Result GetCurrentProcessFilename(String& out)
    {
        return Result::NotSupported;
    }
}

#endif
