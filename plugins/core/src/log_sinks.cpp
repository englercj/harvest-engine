// Copyright Chad Engler

#include "he/core/log_sinks.h"

#include "he/core/appender.h"
#include "he/core/clock.h"
#include "he/core/clock_fmt.h"
#include "he/core/debugger.h"
#include "he/core/directory.h"
#include "he/core/enum_fmt.h"
#include "he/core/path.h"

#include "fmt/format.h"

namespace he
{
    void FormatKVsTo(String& dst, const LogKV* kvs, uint32_t count)
    {
        constexpr auto ValueFmt = FMT_STRING("{} = {}");

        for (uint32_t i = 0; i < count; ++i)
        {
            const LogKV& kv = kvs[i];

            switch (kv.kind)
            {
                case LogKV::Kind::Bool: fmt::format_to(Appender(dst), ValueFmt, kv.key, kv.value.b); break;
                case LogKV::Kind::Int: fmt::format_to(Appender(dst), ValueFmt, kv.key, kv.value.i); break;
                case LogKV::Kind::Uint: fmt::format_to(Appender(dst), ValueFmt, kv.key, kv.value.u); break;
                case LogKV::Kind::Double: fmt::format_to(Appender(dst), ValueFmt, kv.key, kv.value.d); break;
                case LogKV::Kind::String: fmt::format_to(Appender(dst), ValueFmt, kv.key, kv.value.s.Data()); break;
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

    Result FileSink::Configure(const char* directory, const char* prefix, bool utcTime)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_utc = utcTime;

        Result r = Directory::Create(directory, true);
        if (!r)
            return r;

        m_file.Close();

        m_buf = directory;
        ConcatPath(m_buf, prefix);


        constexpr auto LogFileNameFmt = FMT_STRING("_{:%Y-%m-%d_%H-%M-%S}.log");

        SystemTime now = SystemClock::Now();
        if (m_utc)
            fmt::format_to(Appender(m_buf), LogFileNameFmt, FmtUtcTime(now));
        else
            fmt::format_to(Appender(m_buf), LogFileNameFmt, FmtLocalTime(now));

        return m_file.Open(m_buf.Data(), FileOpenMode::WriteTruncate);
    }

    void FileSink::LogHandler(void* userData, const LogSource& source, const LogKV* kvs, uint32_t count)
    {
        SystemTime now = SystemClock::Now();
        double fractionalSeconds = now.val / static_cast<double>(he::Seconds::Ratio);
        fractionalSeconds -= static_cast<uint64_t>(fractionalSeconds);

        FileSink& sink = *static_cast<FileSink*>(userData);

        std::lock_guard<std::mutex> lock(sink.m_mutex);

        if (!sink.m_file.IsOpen())
            return;

        constexpr auto LogLineHdrFmt = FMT_STRING("[{:%Y-%m-%d_%H-%M-%S}{:.03f}][{}]({}) ");

        sink.m_buf.Clear();
        if (sink.m_utc)
            fmt::format_to(Appender(sink.m_buf), LogLineHdrFmt, FmtUtcTime(now), fractionalSeconds, source.level, source.category);
        else
            fmt::format_to(Appender(sink.m_buf), LogLineHdrFmt, FmtLocalTime(now), fractionalSeconds, source.level, source.category);

        FormatKVsTo(sink.m_buf, kvs, count);
        sink.m_buf.PushBack('\n');

        sink.m_file.Write(sink.m_buf.Data(), sink.m_buf.Size());
    }
}
