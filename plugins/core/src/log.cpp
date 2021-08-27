// Copyright Chad Engler

#include "he/core/log.h"

#include "he/core/assert.h"
#include "he/core/debug.h"
#include "he/core/vector.h"

#include "fmt/format.h"

namespace he
{
    static Vector<LogSinkFunc>& GetSinks()
    {
        static Vector<LogSinkFunc> s_sinks{ CrtAllocator::Get() };
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
        GetSinks().PushBack(sink);
    }

    void RemoveLogSink(LogSinkFunc sink)
    {
        Vector<LogSinkFunc>& sinks = GetSinks();

        for (uint32_t i = 0; i < sinks.Size(); ++i)
        {
            if (sinks[i] == sink)
            {
                sinks.Erase(i, 1);
                return;
            }
        }
    }

    void Log(const LogSource& source, const LogKV* kvs, uint32_t count)
    {
        Vector<LogSinkFunc>& sinks = GetSinks();

        for (LogSinkFunc sink : sinks)
        {
            sink(source, kvs, count);
        }
    }

    void DebuggerSink(const LogSource& source, const LogKV* kvs, uint32_t count)
    {
        fmt::memory_buffer buf;
        fmt::format_to(fmt::appender(buf), "{}({}): [{}]({}) ", source.file, source.line, AsString(source.level), source.category);

        constexpr auto ValueFmt = FMT_STRING("{} = {}");

        for (uint32_t i = 0; i < count; ++i)
        {
            const LogKV& kv = kvs[i];

            switch (kv.type)
            {
                case LogKV::ValueType::Bool: fmt::format_to(fmt::appender(buf), ValueFmt, kv.key, kv.value.b); break;
                case LogKV::ValueType::Int: fmt::format_to(fmt::appender(buf), ValueFmt, kv.key, kv.value.i); break;
                case LogKV::ValueType::Uint: fmt::format_to(fmt::appender(buf), ValueFmt, kv.key, kv.value.u); break;
                case LogKV::ValueType::Double: fmt::format_to(fmt::appender(buf), ValueFmt, kv.key, kv.value.d); break;
                case LogKV::ValueType::String: fmt::format_to(fmt::appender(buf), ValueFmt, kv.key, kv.value.s.data()); break;
            }

            if (i != (count - 1))
            {
                buf.push_back(',');
                buf.push_back(' ');
            }
        }

        buf.push_back('\n');
        buf.push_back('\0');

        PrintToDebugger(buf.data());
    }
}
