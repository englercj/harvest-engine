// Copyright Chad Engler

#include "he/core/log_sinks.h"

#include "he/core/assert.h"
#include "he/core/appender.h"
#include "he/core/clock.h"
#include "he/core/clock_fmt.h"
#include "he/core/debugger.h"
#include "he/core/directory.h"
#include "he/core/enum_fmt.h"
#include "he/core/key_value_fmt.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/string_fmt.h"

#include "fmt/format.h"

#include <cstdio>

namespace he
{
    template <typename TimeFmt>
    static void FormatFileLogLine(String& out, const LogSource& source, const KeyValue* kvs, uint32_t count, SystemTime now)
    {
        double fractionalSeconds = now.val / static_cast<double>(he::Seconds::Ratio);
        fractionalSeconds -= static_cast<uint64_t>(fractionalSeconds);

        if (count == 1 && kvs[0].Kind() == KeyValue::ValueKind::String && String::Equal(kvs[0].Key(), HE_MSG_KEY))
        {
            fmt::format_to(
                Appender(out),
                FMT_STRING("[{:%Y-%m-%d_%H-%M-%S}{:.04f}] [{}]({}) {}\n"),
                TimeFmt(now),
                fractionalSeconds,
                source.level,
                source.category,
                kvs[0].GetString());
        }
        else
        {
            fmt::format_to(
                Appender(out),
                FMT_STRING("[{:%Y-%m-%d_%H-%M-%S}{:.04f}] [{}]({}) {}\n"),
                TimeFmt(now),
                fractionalSeconds,
                source.level,
                source.category,
                fmt::join(kvs, kvs + count, ", "));
        }
    }

    template <typename TimeFmt>
    static void FormatFileName(String& out)
    {
        SystemTime now = SystemClock::Now();

        fmt::format_to(
            Appender(out),
            FMT_STRING("_{:%Y-%m-%d_%H-%M-%S}.log"),
            TimeFmt(now));
    }

    FileSink::FileSink(Allocator& allocator)
        : m_buf(allocator)
    {}

    Result FileSink::Configure(const char* directory, const char* prefix, bool utcTime)
    {
        LockGuard lock(m_mutex);

        m_utc = utcTime;

        Result r = Directory::Create(directory, true);
        if (!r)
            return r;

        m_file.Close();

        m_buf = directory;
        ConcatPath(m_buf, prefix);

        if (m_utc)
            FormatFileName<FmtUtcTime>(m_buf);
        else
            FormatFileName<FmtLocalTime>(m_buf);

        return m_file.Open(m_buf.Data(), FileOpenMode::WriteTruncate);
    }

    void FileSink::OnLogEntry(const LogSource& source, const KeyValue* kvs, uint32_t count)
    {
        SystemTime now = SystemClock::Now();

        LockGuard lock(m_mutex);

        if (!m_file.IsOpen())
            return;

        m_buf.Clear();
        if (m_utc)
            FormatFileLogLine<FmtUtcTime>(m_buf, source, kvs, count, now);
        else
            FormatFileLogLine<FmtLocalTime>(m_buf, source, kvs, count, now);

        m_file.Write(m_buf.Data(), m_buf.Size());
    }

    void DebuggerSink(const LogSource& source, const KeyValue* kvs, uint32_t count)
    {
        String msg(Allocator::GetTemp());

        if (count == 1 && kvs[0].Kind() == KeyValue::ValueKind::String && String::Equal(kvs[0].Key(), HE_MSG_KEY))
        {
            fmt::format_to(
                Appender(msg),
                "{}({}): [{}]({}) {}\n",
                source.file,
                source.line,
                source.level,
                source.category,
                kvs[0].GetString());
        }
        else
        {
            fmt::format_to(
                Appender(msg),
                "{}({}): [{}]({}) {}\n",
                source.file,
                source.line,
                source.level,
                source.category,
                fmt::join(kvs, kvs + count, ", "));
        }

        PrintToDebugger(msg.Data());
    }

    void ConsoleSink(const LogSource& source, const KeyValue* kvs, uint32_t count)
    {
        String msg(Allocator::GetTemp());

        if (count == 1 && kvs[0].Kind() == KeyValue::ValueKind::String && String::Equal(kvs[0].Key(), HE_MSG_KEY))
        {
            fmt::format_to(
                Appender(msg),
                "[{}]({}) {}\n",
                source.level,
                source.category,
                kvs[0].GetString());
        }
        else
        {
            fmt::format_to(
                Appender(msg),
                "[{}]({}) {}\n",
                source.level,
                source.category,
                fmt::join(kvs, kvs + count, ", "));
        }

        auto* stream = source.level >= LogLevel::Warn ? stderr : stdout;
        fputs(msg.Data(), stream);
    }
}
