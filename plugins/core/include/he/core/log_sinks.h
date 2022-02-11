// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/file.h"
#include "he/core/log.h"
#include "he/core/types.h"

#include <mutex>

namespace he
{
    /// Format key-value log pairs to a string output.
    ///
    /// \param[out] dst The destination string to append to.
    /// \param[in] kvs The key-value array to format.
    /// \param[in] count The size of the key-value array.
    void FormatKVsTo(String& dst, const LogKV* kvs, uint32_t count);

    /// Log sink that logs to the debugger.
    class DebuggerSink
    {
    public:
        /// Constructs a new debugger sink.
        ///
        /// \param[in] allocator The allocator to use.
        DebuggerSink(Allocator& allocator = Allocator::GetDefault());

        /// Handler function that can be added a sink. Make sure to pass the DebuggerSink instance
        /// as the userData parameter when calling \ref AddLogSink.
        ///
        /// \param[in] source The source information for this log entry.
        /// \param[in] kvs An array of key-value pairs.
        /// \param[in] count The size of the `kvs` array.
        static void LogHandler(void* userData, const LogSource& source, const LogKV* kvs, uint32_t count);

    private:
        std::mutex m_mutex{};
        String m_buf;
    };

    /// Log sink that logs to a file. The file's name is based on the current system time
    /// and prefixed with a user-defined value.
    class FileSink
    {
    public:
        /// Constructs a new file sink.
        ///
        /// \param[in] allocator The allocator to use.
        FileSink(Allocator& allocator = Allocator::GetDefault());

        /// Configures a new file sink.
        ///
        /// \param[in] directory The output directory to store the file.
        /// \param[in] prefix The prefix of the file to write to.
        /// \param[in] utcTime Optional. When set uses UTC times instead of local times.
        /// \return The result of opening the file to write to.
        Result Configure(const char* directory, const char* prefix, bool utcTime = false);

        /// Handler function that can be added a sink. Make sure to pass the FileSink instance
        /// as the userData parameter when calling \ref AddLogSink.
        ///
        /// \param[in] source The source information for this log entry.
        /// \param[in] kvs An array of key-value pairs.
        /// \param[in] count The size of the `kvs` array.
        static void LogHandler(void* userData, const LogSource& source, const LogKV* kvs, uint32_t count);

    private:
        std::mutex m_mutex{};
        String m_buf{};
        File m_file{};
        bool m_utc{ false };
    };
}
