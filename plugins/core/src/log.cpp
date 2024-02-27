// Copyright Chad Engler

#include "he/core/log.h"

#include "he/core/enum_ops.h"
#include "he/core/vector.h"

namespace he
{
    static Vector<LogDelegate>& GetSinks()
    {
        static Vector<LogDelegate> s_sinks;
        return s_sinks;
    }

    void AddLogSink(LogDelegate sink)
    {
        GetSinks().PushBack(sink);
    }

    void RemoveLogSink(LogDelegate sink)
    {
        Vector<LogDelegate>& sinks = GetSinks();

        for (uint32_t i = 0; i < sinks.Size(); ++i)
        {
            if (sinks[i] == sink)
            {
                sinks.Erase(i, 1);
                return;
            }
        }
    }

    void Log(const LogSource& source, const KeyValue* kvs, uint32_t count)
    {
        Vector<LogDelegate>& sinks = GetSinks();

        for (const LogDelegate& sink : sinks)
        {
            sink(source, kvs, count);
        }
    }

    template <>
    const char* EnumTraits<LogLevel>::ToString(LogLevel x) noexcept
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
