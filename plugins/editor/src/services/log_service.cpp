// Copyright Chad Engler

#include "log_service.h"

#include "he/core/appender.h"
#include "he/core/enum_fmt.h"
#include "he/core/key_value.h"
#include "he/core/key_value_fmt.h"
#include "he/core/log.h"
#include "he/core/memory_ops.h"
#include "he/core/path.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"

#include "fmt/format.h"

namespace he::editor
{
    LogService::LogService(DirectoryService& directoryService)
        : m_directoryService(directoryService)
    {}

    bool LogService::Initialize()
    {
        AddLogSink(*this);

        // TODO: log rotation for files
        const String dir = m_directoryService.GetAppDirectory(DirectoryService::DirType::Logs);
        const Result r = m_fileSink.Configure(dir.Data(), "editor");
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

    void LogService::OnLogEntry(const LogSource& source, const KeyValue* kvs, uint32_t count)
    {
        LockGuard lock(m_mutex);

        if (m_entries.size() == MaxEntries)
            m_entries.pop_front();

        LogEntry& entry = m_entries.emplace_back();
        entry.source = source;
        entry.kvs.Insert(0, kvs, kvs + count);
    }
}
