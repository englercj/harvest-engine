// Copyright Chad Engler

#include "log_service.h"

#include "he/core/appender.h"
#include "he/core/enum_fmt.h"
#include "he/core/key_value.h"
#include "he/core/key_value_fmt.h"
#include "he/core/log.h"
#include "he/core/log_sinks.h"
#include "he/core/memory_ops.h"
#include "he/core/path.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"

#include "fmt/format.h"

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

    static void HashEntry(CRC32C& crc, const LogService::Entry& entry)
    {
        crc.Scalar(entry.timestamp.val);
        crc.Scalar(AsUnderlyingType(entry.source.level));
        crc.Scalar(entry.source.line);
        crc.String(entry.source.file);
        crc.String(entry.source.funcName);
        crc.String(entry.source.category);

        for (const KeyValue& kv : entry.kvs)
        {
            switch (kv.Kind())
            {
                case he::KeyValue::ValueKind::Bool: crc.Scalar(kv.GetBool()); break;
                case he::KeyValue::ValueKind::Enum: crc.Scalar(kv.GetEnumValue()); break;
                case he::KeyValue::ValueKind::Int: crc.Scalar(kv.GetInt()); break;
                case he::KeyValue::ValueKind::Uint: crc.Scalar(kv.GetUint()); break;
                case he::KeyValue::ValueKind::Double: crc.Scalar(kv.GetDouble()); break;
                case he::KeyValue::ValueKind::String:
                {
                    const String& str = kv.GetString();
                    crc.Data(str.Data(), str.Size());
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
        m_entriesHash.store(m_entriesCrc.Done());
    }

    const std::atomic<uint32_t>& LogService::GetLevelCount(LogLevel level) const
    {
        const uint32_t index = static_cast<uint32_t>(level);
        HE_ASSERT(index < HE_LENGTH_OF(m_levelCounts));

        return m_levelCounts[index];
    }
}
