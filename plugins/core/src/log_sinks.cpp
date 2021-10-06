// Copyright Chad Engler

#include "he/core/log_sinks.h"

#include "he/core/appender.h"
#include "he/core/clock.h"
#include "he/core/enum_fmt.h"
#include "he/core/path.h"

#include "fmt/format.h"
#include "fmt/chrono.h"

#include <chrono>

namespace he
{
    void FormatKVsTo(String& dst, const LogKV* kvs, uint32_t count)
    {
        constexpr auto ValueFmt = FMT_STRING("{} = {}");

        for (uint32_t i = 0; i < count; ++i)
        {
            const LogKV& kv = kvs[i];

            switch (kv.type)
            {
                case LogKV::ValueType::Bool: fmt::format_to(Appender(dst), ValueFmt, kv.key, kv.value.b); break;
                case LogKV::ValueType::Int: fmt::format_to(Appender(dst), ValueFmt, kv.key, kv.value.i); break;
                case LogKV::ValueType::Uint: fmt::format_to(Appender(dst), ValueFmt, kv.key, kv.value.u); break;
                case LogKV::ValueType::Double: fmt::format_to(Appender(dst), ValueFmt, kv.key, kv.value.d); break;
                case LogKV::ValueType::String: fmt::format_to(Appender(dst), ValueFmt, kv.key, kv.value.s.Data()); break;
            }

            if (i != (count - 1))
            {
                dst.PushBack(',');
                dst.PushBack(' ');
            }
        }
    }

    DebuggerSink::DebuggerSink(Allocator& allocator)
        : m_buf(allocator)
    {}

    void DebuggerSink::LogHandler(void* userData, const LogSource& source, const LogKV* kvs, uint32_t count)
    {
        DebuggerSink& sink = *static_cast<DebuggerSink*>(userData);

        std::lock_guard<std::mutex> lock(sink.m_mutex);

        sink.m_buf.Clear();
        fmt::format_to(Appender(sink.m_buf), "{}({}): [{}]({}) ", source.file, source.line, source.level, source.category);
        FormatKVsTo(sink.m_buf, kvs, count);
        sink.m_buf.PushBack('\n');

        PrintToDebugger(sink.m_buf.Data());
    }

    FileSink::FileSink(Allocator& allocator)
        : m_buf(allocator)
    {}

    Result FileSink::Configure(const char* directory, const char* prefix)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_file.Close();

        m_buf = directory;
        ConcatPath(m_buf, prefix);

        SystemTime now = SystemClock::Now();

        std::chrono::system_clock::time_point timePoint{
            std::chrono::duration_cast<std::chrono::system_clock::duration>(
                std::chrono::nanoseconds{ now.ns }) };

        std::time_t time = std::chrono::system_clock::to_time_t(timePoint);

        fmt::format_to(Appender(m_buf), "_{:%Y-%m-%d_%H-%M-%S}.log", fmt::localtime(time));

        return m_file.Open(m_buf.Data(), FileOpenMode::WriteTruncate);
    }

    void FileSink::LogHandler(void* userData, const LogSource& source, const LogKV* kvs, uint32_t count)
    {
        FileSink& sink = *static_cast<FileSink*>(userData);

        std::lock_guard<std::mutex> lock(sink.m_mutex);

        if (!sink.m_file.IsOpen())
            return;

        sink.m_buf.Clear();
        fmt::format_to(Appender(sink.m_buf), "[{}]({}) ", source.level, source.category);
        FormatKVsTo(sink.m_buf, kvs, count);
        sink.m_buf.PushBack('\n');

        sink.m_file.Write(sink.m_buf.Data(), sink.m_buf.Size());
    }
}
