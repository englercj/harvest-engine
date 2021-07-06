// Copyright Chad Engler

#include "he/core/log.h"

#include "he/core/debug.h"

#include "fmt/format.h"

namespace he
{
    static std::vector<LogSinkFunc> g_sinks;

    const char* AsString(LogLevel x)
    {
        switch (x)
        {
            case LogLevel::Trace: return "Trace";
            case LogLevel::Debug: return "Debug";
            case LogLevel::Info: return "Info";
            case LogLevel::Warn: return "Warn";
            case LogLevel::Error: return "Error";
        }

        return "<unknown>";
    }

    void AddLogSink(LogSinkFunc sink)
    {
        g_sinks.push_back(sink);
    }

    template <typename T>
    static void DebugFormatKV(const char* key, const T& value, fmt::memory_buffer& buf)
    {
        fmt::format_to(buf, "{} = {}, ", key, value);
    }

    void DebugOutputSink(const LogSource& source, const LogKV* kvs, uint32_t count)
    {
        fmt::memory_buffer buf;
        fmt::format_to(buf, "{}({}): [{}]({}) ", source.file, source.line, AsString(source.level), source.category);

        for (uint32_t i = 0; i < count; ++i)
        {
            const LogKV& kv = kvs[i];
            HE_KV_DISPATCH(kv, DebugFormatKV, buf);
        }

        buf.push_back('\n');
        buf.push_back('\0');
        return OutputToDebugger(buf.data());
    }

    void Log(const LogSource& source, const LogKV* kvs, uint32_t count)
    {
        for (LogSinkFunc sink : g_sinks)
        {
            sink(source, kvs, count);
        }
    }
}
