// Copyright Chad Engler

#include "he/core/debug.h"

#if defined(HE_PLATFORM_WASM)

#include "wasm_core.js.h"

namespace he
{
    void PrintToDebugger(const char* s)
    {
        heWASM_ConsoleLog(heWASM_ConsoleLogLevel::Log, s);
    }

    bool IsDebuggerAttached()
    {
        // Detecting this is unreliable at best, so just assume it isn't.
        return false;
    }
}

#endif
