// Copyright Chad Engler

#include "he/editor/services/log_service.h"

#include "he/core/key_value.h"
#include "he/core/log.h"
#include "he/core/log_sinks.h"
#include "he/core/memory_ops.h"
#include "he/core/path.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"

namespace he::editor
{
    LogService::LogService(DirectoryService& directoryService) noexcept
        : m_directoryService(directoryService)
    {}

    bool LogService::Initialize()
    {
        AddLogSink(*this);

        // TODO: log rotation for files
        const String dir = m_directoryService.GetAppDirectory(DirectoryService::DirType::Logs);
        const Result r = m_fileSink.Configure(dir.Data(), "he_editor");
        if (!r)
        {
            HE_LOGF_ERROR(editor, "Failed to initialize file log sink. Error: {}", r);
            return false;
        }

        AddLogSink(m_fileSink);
        return true;
    }

    void LogService::Terminate()
    {
        RemoveLogSink(*this);
        RemoveLogSink(m_fileSink);
    }

    uint32_t LogService::GetNumEntries(LogLevel level) const
    {
        return GetLevelCount(level).load();
    }

    uint32_t LogService::GetEntriesHash() const
    {
        return m_entriesHash.load();
    }

    static void HashEntry(Hash<CRC32C>& crc, const LogService::Entry& entry)
    {
        crc.Update(entry.timestamp.val);
        crc.Update(entry.source.level);
        crc.Update(entry.source.line);
        crc.Update(entry.source.file);
        crc.Update(entry.source.funcName);
        crc.Update(entry.source.category);

        for (const KeyValue& kv : entry.kvs)
        {
            switch (kv.Kind())
            {
                case he::KeyValue::ValueKind::Bool: crc.Update(kv.GetBool()); break;
                case he::KeyValue::ValueKind::Enum: crc.Update(kv.GetEnumValue()); break;
                case he::KeyValue::ValueKind::Int: crc.Update(kv.GetInt()); break;
                case he::KeyValue::ValueKind::Uint: crc.Update(kv.GetUint()); break;
                case he::KeyValue::ValueKind::Double: crc.Update(kv.GetDouble()); break;
                case he::KeyValue::ValueKind::String:
                {
                    const String& str = kv.GetString();
                    crc.Update(str.Data(), str.Size());
                    break;
                }
            }
        }
    }

    void LogService::OnLogEntry(const LogSource& source, const KeyValue* kvs, uint32_t count)
    {
        LockGuard lock(m_mutex);

        if (m_entries.size() == MaxEntries)
        {
            GetLevelCount(m_entries.front().source.level).fetch_sub(1);
            m_entries.pop_front();
        }

        Entry& entry = m_entries.emplace_back();
        entry.source = source;
        entry.kvs.Insert(0, kvs, kvs + count);
        HashEntry(m_entriesCrc, entry);

        GetLevelCount(entry.source.level).fetch_add(1);
        m_entriesHash.store(m_entriesCrc.Value());
    }

    const std::atomic<uint32_t>& LogService::GetLevelCount(LogLevel level) const
    {
        const uint32_t index = static_cast<uint32_t>(level);
        HE_ASSERT(index < HE_LENGTH_OF(m_levelCounts));

        return m_levelCounts[index];
    }
}
