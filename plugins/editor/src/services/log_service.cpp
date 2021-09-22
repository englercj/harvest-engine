// Copyright Chad Engler

#include "log_service.h"

#include "he/core/log.h"
#include "he/core/memory_ops.h"
#include "he/core/path.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"

namespace he::editor
{
    LogService::LogService(Allocator& allocator, DirectoryService& directoryService)
        : m_allocator(allocator)
        , m_directoryService(directoryService)
        , m_fileSink(allocator)
    {}

    bool LogService::Initialize()
    {
        AddLogSink(*this);

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

    void LogService::LogHandler(void* userData, const LogSource& source, const LogKV* kvs, uint32_t count)
    {
        LogService& service = *static_cast<LogService*>(userData);

        std::lock_guard<std::mutex> lock(service.m_mutex);

        if (service.m_entries.size() == MaxEntries)
            service.m_entries.pop_front();

        LogEntry& entry = service.m_entries.emplace_back(service.m_allocator, source);

        // TODO: Maybe just copy the KVs and display them later in a formatted UI? Treat a couple like "message" special at display time?

        fmt::format_to(StringAppender(entry.msg), "[{}]({}) ", AsString(source.level), source.category);
        FormatKVsTo(entry.msg, kvs, count);
        entry.msg.PushBack('\n');
    }
}
