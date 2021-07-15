// Copyright Chad Engler

#include "he/core/debug.h"

#include "he/core/platform.h"

#if HE_PLATFORM_EMSCRIPTEN

#include <emscripten/emscripten.h>

namespace he
{
    void OutputToDebugger(const char* s)
    {
        emscripten_log(EM_LOG_CONSOLE, s);
    }

    bool IsDebuggerAttached()
    {
        return false;
    }
}

#endif
