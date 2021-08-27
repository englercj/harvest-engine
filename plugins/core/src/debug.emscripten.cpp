// Copyright Chad Engler

#include "he/core/debug.h"

#if defined(HE_PLATFORM_EMSCRIPTEN)

#include <emscripten/emscripten.h>

namespace he
{
    void PrintToDebugger(const char* s)
    {
        emscripten_log(EM_LOG_CONSOLE, s);
    }

    bool IsDebuggerAttached()
    {
        return false;
    }
}

#endif
