// Copyright Chad Engler

#pragma once

#include "directory_service.h"

#include "he/core/clock.h"
#include "he/core/key_value.h"
#include "he/core/log.h"
#include "he/core/log_sinks.h"
#include "he/core/vector.h"

#include <deque>

namespace he::editor
{
    class LogService
    {
    public:
        LogService(DirectoryService& directoryService);

        bool Initialize();
        void Terminate();

        template <typename F>
        void ForEach(F&& itr) const
        {
            LockGuard lock(m_mutex);
            for (const LogEntry& entry : m_entries)
            {
                if (!itr(entry))
                    break;
            }
        }

        static void LogHandler(void* userData, const LogSource& source, const KeyValue* kvs, uint32_t count);

    private:
        struct LogEntry
        {
            LogSource source{};
            Vector<KeyValue> kvs{};
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
