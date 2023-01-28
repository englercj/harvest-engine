// Copyright Chad Engler

#pragma once

#include "he/core/fmt.h"
#include "he/core/string.h"
#include "he/core/utils.h"

namespace he
{
    /// Outputs `s` to the debugger output. On Windows this will output to the debug channel
    /// even when the debugger is not attached. You can see such messages using DebugView.
    ///
    /// \param s The string to print.
    void PrintToDebugger(const char* s);

    /// Outputs the formatted string to the debugger output.
    ///
    /// \param fmt The format string.
    /// \param args The arguments for the format string.
    template <typename... Args>
    void PrintToDebugger(FmtString<Args...> fmt, Args&&... args)
    {
        const String buf = Format(fmt, Forward<Args>(args)...);
        return PrintToDebugger(buf.Data());
    }

    /// Checks if the debugger is currently attached to this program and returns the result.
    ///
    /// \note Not all platforms support checking this value, in which case false is returned.
    ///
    /// \return True if the debugger is currently attached.
    bool IsDebuggerAttached();
}
