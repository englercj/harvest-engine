// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/file.h"
#include "he/core/string.h"
#include "he/core/sync.h"
#include "he/core/types.h"

namespace he
{
    struct LogSource;

    /// Log sink that logs to a file. The file's name is based on the current system time
    /// and prefixed with a user-defined value.
    ///
    /// TODO: Use async file IO to reduce lock contention and stalls when writing.
    class FileSink
    {
    public:
        /// Handler function that can be added a sink. Make sure to pass the FileSink instance
        /// as the userData parameter when calling \ref AddLogSink.
        ///
        /// \param[in] source The source information for this log entry.
        /// \param[in] kvs An array of key-value pairs.
        /// \param[in] count The size of the `kvs` array.
        static void LogHandler(void* userData, const LogSource& source, const KeyValue* kvs, uint32_t count)
        {
            static_cast<FileSink*>(userData)->Handler(source, kvs, count);
        }

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

    private:
        void Handler(const LogSource& source, const KeyValue* kvs, uint32_t count);

    private:
        Mutex m_mutex{};
        String m_buf{};
        File m_file{};
        bool m_utc{ false };
    };

    /// Log sink that logs to the debugger.
    ///
    /// \param[in] userData The userData passed when adding the sink. Not used.
    /// \param[in] source The source information for this log entry.
    /// \param[in] kvs An array of key-value pairs.
    /// \param[in] count The size of the `kvs` array.
    void DebuggerSink(void* userData, const LogSource& source, const KeyValue* kvs, uint32_t count);

    /// Log sink that logs to stdout (and stderr for warnings and errors).
    ///
    /// \param[in] userData The userData passed when adding the sink. Not used.
    /// \param[in] source The source information for this log entry.
    /// \param[in] kvs An array of key-value pairs.
    /// \param[in] count The size of the `kvs` array.
    void ConsoleSink(void* userData, const LogSource& source, const KeyValue* kvs, uint32_t count);
}
