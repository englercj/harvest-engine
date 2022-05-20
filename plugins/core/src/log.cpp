// Copyright Chad Engler

#include "he/core/log.h"

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

    void Log(const LogSource& source, const KeyValue* kvs, uint32_t count)
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
}
