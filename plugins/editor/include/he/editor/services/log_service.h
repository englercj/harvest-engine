// Copyright Chad Engler

#pragma once

#include "he/core/clock.h"
#include "he/core/hash.h"
#include "he/core/key_value.h"
#include "he/core/log.h"
#include "he/core/log_sinks.h"
#include "he/core/vector.h"
#include "he/editor/services/directory_service.h"

#include <atomic>
#include <deque>

namespace he::editor
{
    class LogService
    {
    public:
        struct Entry
        {
            LogSource source{};
            Vector<KeyValue> kvs{};
            SystemTime timestamp{ SystemClock::Now() };
        };

    public:
        LogService(DirectoryService& directoryService) noexcept;

        bool Initialize();
        void Terminate();

        template <typename F>
        void ForEach(F&& itr)
        {
            LockGuard lock(m_mutex);
            for (const Entry& entry : m_entries)
            {
                if (!itr(entry))
                    break;
            }
        }

        uint32_t GetNumEntries(LogLevel level) const;
        uint32_t GetEntriesHash() const;

        void OnLogEntry(const LogSource& source, const KeyValue* kvs, uint32_t count);

    private:
        const std::atomic<uint32_t>& GetLevelCount(LogLevel level) const;

        std::atomic<uint32_t>& GetLevelCount(LogLevel level)
        {
            return const_cast<std::atomic<uint32_t>&>(const_cast<const LogService*>(this)->GetLevelCount(level));
        }

    private:
        static constexpr size_t MaxEntries = 256;

    private:
        DirectoryService& m_directoryService;

        FileSink m_fileSink{};

        std::atomic<uint32_t> m_levelCounts[5]{};
        std::atomic<uint32_t> m_entriesHash{};

        Mutex m_mutex{};
        std::deque<Entry> m_entries{};
        CRC32C m_entriesCrc{};
    };
}
