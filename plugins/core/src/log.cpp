// Copyright Chad Engler

#include "he/core/log.h"

#include "he/core/assert.h"
#include "he/core/debug.h"
#include "he/core/enum_ops.h"
#include "he/core/vector.h"

#include "fmt/core.h"

namespace he
{
    struct LogSinkStorage
    {
        LogSinkFunc func;
        void* userData;
    };

    static Vector<LogSinkStorage>& GetSinks()
    {
        static Vector<LogSinkStorage> s_sinks;
        return s_sinks;
    }

    bool LogKV::GetBool() const
    {
        HE_ASSERT(kind == Kind::Bool);
        return value.b;
    }

    int64_t LogKV::GetInt() const
    {
        HE_ASSERT(kind == Kind::Int);
        return value.i;
    }

    uint64_t LogKV::GetUint() const
    {
        HE_ASSERT(kind == Kind::Uint);
        return value.u;
    }

    double LogKV::GetDouble() const
    {
        HE_ASSERT(kind == Kind::Double);
        return value.d;
    }

    const char* LogKV::GetString() const
    {
        HE_ASSERT(kind == Kind::String);
        return value.s.Data();
    }

    void AddLogSink(LogSinkFunc sink, void* userData)
    {
        GetSinks().PushBack({ sink, userData });
    }

    void RemoveLogSink(LogSinkFunc sink, void* userData)
    {
        Vector<LogSinkStorage>& sinks = GetSinks();

        for (uint32_t i = 0; i < sinks.Size(); ++i)
        {
            if (sinks[i].func == sink && sinks[i].userData == userData)
            {
                sinks.Erase(i, 1);
                return;
            }
        }
    }

    void Log(const LogSource& source, const LogKV* kvs, uint32_t count)
    {
        Vector<LogSinkStorage>& sinks = GetSinks();

        for (const LogSinkStorage& sink : sinks)
        {
            sink.func(sink.userData, source, kvs, count);
        }
    }

    template <>
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

    template <>
    const char* AsString(LogKV::Kind x)
    {
        switch (x)
        {
            case LogKV::Kind::Bool: return "Bool";
            case LogKV::Kind::Int: return "Int";
            case LogKV::Kind::Uint: return "Uint";
            case LogKV::Kind::Double: return "Double";
            case LogKV::Kind::String: return "String";
        }

        return "<unknown>";
    }
}
