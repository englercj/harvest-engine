// Copyright Chad Engler

#pragma once

#include "directory_service.h"

#include "he/core/clock.h"
#include "he/core/log.h"
#include "he/core/log_sinks.h"

#include "fmt/core.h"

#include <deque>

namespace he::editor
{
    class LogService
    {
    public:
        LogService(DirectoryService& directoryService);

        bool Initialize();
        void Terminate();

        auto EntriesBegin() { return m_entries.begin(); }
        auto EntriesEnd() { return m_entries.end(); }

        static void LogHandler(void* userData, const LogSource& source, const KeyValue* kvs, uint32_t count);

    private:
        struct LogEntry
        {
            LogEntry(const LogSource& src) : source(src) {}

            const LogSource& source;
            String msg{};
            SystemTime timestamp{ SystemClock::Now() };
        };

    private:
        static constexpr size_t MaxEntries = 256;

    private:
        DirectoryService& m_directoryService;

        FileSink m_fileSink{};

        Mutex m_mutex{};
        std::deque<LogEntry> m_entries{};
    };
}
