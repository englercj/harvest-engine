// Copyright Chad Engler

#include "debugger_impl.h"

#if defined(HE_PLATFORM_EMSCRIPTEN)

#include <emscripten/emscripten.h>

namespace he
{
    void DebuggerImpl::Print(const char* s) const
    {
        emscripten_log(EM_LOG_CONSOLE, s);
    }

    bool DebuggerImpl::IsAttached() const
    {
        return false;
    }
}

#endif
