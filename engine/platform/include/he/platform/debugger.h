// Copyright Chad Engler

#pragma once

#include "he/core/log.h"
#include "he/core/utils.h"
#include "he/core/types.h"

#include "fmt/format.h"

namespace he
{
    class Debugger
    {
    public:
        virtual ~Debugger() {}

        /// Outputs `s` to the debugger output. On MSVC this will output to the debug channel
        /// even when the debugger is not attached. You can see such messages using DebugView.
        ///
        /// \param s The string to output
        virtual void Print(const char* s) const = 0;

        /// Outputs the formatted string to the debugger output.
        ///
        /// \param fmt The format string
        /// \param args The arguments for the format string
        template <typename... Args>
        void Print(const char* fmt, Args&&... args) const
        {
            fmt::memory_buffer buf;
            fmt::format_to(fmt::appender(buf), fmt, Forward<Args>(args)...);
            buf.push_back('\0');
            return Print(buf.data());
        }

        // Returns `true` if the debugger is currently attached
        virtual bool IsAttached() const = 0;
    };

    /// Log sink that logs to the debugger.
    ///
    /// \param source The source information for this log entry.
    /// \param kvs An array of key-value pairs.
    /// \param count The size of the `kvs` array.
    void DebugOutputSink(const LogSource& source, const LogKV* kvs, uint32_t count);
}
