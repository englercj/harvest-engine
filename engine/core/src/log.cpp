// Copyright Chad Engler

#include "he/core/log.h"

#include "he/core/assert.h"
#include "he/core/debug.h"

#include "fmt/format.h"

#include <vector>

namespace he
{
    static std::vector<LogSinkFunc>& GetSinks()
    {
        static std::vector<LogSinkFunc> s_sinks{};
        return s_sinks;
    }

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

    bool LogKV::GetBool() const
    {
        HE_ASSERT(type == ValueType::Bool);
        return value.b;
    }

    int64_t LogKV::GetInt() const
    {
        HE_ASSERT(type == ValueType::Int);
        return value.i;
    }

    uint64_t LogKV::GetUint() const
    {
        HE_ASSERT(type == ValueType::Uint);
        return value.u;
    }

    double LogKV::GetDouble() const
    {
        HE_ASSERT(type == ValueType::Double);
        return value.d;
    }

    const char* LogKV::GetString() const
    {
        HE_ASSERT(type == ValueType::String);
        return value.s.data();
    }

    void AddLogSink(LogSinkFunc sink)
    {
        GetSinks().push_back(sink);
    }

    void RemoveLogSink(LogSinkFunc sink)
    {
        std::vector<LogSinkFunc>& sinks = GetSinks();

        for (uint32_t i = 0; i < sinks.size(); ++i)
        {
            if (sinks[i] == sink)
            {
                sinks.erase(sinks.begin() + i, sinks.begin() + i + 1);
                return;
            }
        }
    }

    void DebugOutputSink(const LogSource& source, const LogKV* kvs, uint32_t count)
    {
        fmt::memory_buffer buf;
        fmt::format_to(fmt::appender(buf), "{}({}): [{}]({}) ", source.file, source.line, AsString(source.level), source.category);

        for (uint32_t i = 0; i < count; ++i)
        {
            const LogKV& kv = kvs[i];

            constexpr auto ValueFmt = FMT_STRING("{} = {}, ");

            switch (kv.type)
            {
                case LogKV::ValueType::Bool: fmt::format_to(fmt::appender(buf), ValueFmt, kv.key, kv.value.b); break;
                case LogKV::ValueType::Int: fmt::format_to(fmt::appender(buf), ValueFmt, kv.key, kv.value.i); break;
                case LogKV::ValueType::Uint: fmt::format_to(fmt::appender(buf), ValueFmt, kv.key, kv.value.u); break;
                case LogKV::ValueType::Double: fmt::format_to(fmt::appender(buf), ValueFmt, kv.key, kv.value.d); break;
                case LogKV::ValueType::String: fmt::format_to(fmt::appender(buf), ValueFmt, kv.key, kv.value.s.data()); break;
            }
        }

        buf.push_back('\n');
        buf.push_back('\0');
        return OutputToDebugger(buf.data());
    }

    void Log(const LogSource& source, const LogKV* kvs, uint32_t count)
    {
        std::vector<LogSinkFunc>& sinks = GetSinks();

        for (LogSinkFunc sink : sinks)
        {
            sink(source, kvs, count);
        }
    }
}
