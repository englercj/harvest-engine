// Copyright Chad Engler

#pragma once

#include "directory_service.h"

#include "he/core/clock.h"
#include "he/core/log.h"
#include "he/core/log_sinks.h"

#include "fmt/format.h"

#include <deque>
#include <mutex>

namespace he::editor
{
    class LogService
    {
    public:
        LogService(Allocator& allocator, DirectoryService& directoryService);

        bool Initialize();
        void Terminate();

        auto EntriesBegin() { return m_entries.begin(); }
        auto EntriesEnd() { return m_entries.end(); }

        static void LogHandler(void* userData, const LogSource& source, const LogKV* kvs, uint32_t count);

    private:
        struct LogEntry
        {
            LogEntry(Allocator& allocator, const LogSource& src) : source(src), msg(allocator) {}

            const LogSource& source;
            String msg;

            SystemTime timestamp{ SystemClock::Now() };
        };

    private:
        Allocator& m_allocator;
        DirectoryService& m_directoryService;

        FileSink m_fileSink;

        static constexpr size_t MaxEntries = 256;

        std::mutex m_mutex{};
        std::deque<LogEntry> m_entries{};
    };
}
