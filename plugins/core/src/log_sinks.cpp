// Copyright Chad Engler

#include "he/core/log_sinks.h"

#include "he/core/assert.h"
#include "he/core/fmt.h"
#include "he/core/clock.h"
#include "he/core/clock_fmt.h"
#include "he/core/debugger.h"
#include "he/core/directory.h"
#include "he/core/key_value_fmt.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/string_fmt.h"

#if defined(HE_PLATFORM_API_WASM)
    #include "he/core/wasm/lib_core.wasm.h"
#else
    #include <iostream>
#endif

namespace he
{
    template <typename TimeFmt>
    static void FormatFileLogLine(String& out, const LogSource& source, const KeyValue* kvs, uint32_t count, SystemTime now)
    {
        double fractionalSeconds = now.val / static_cast<double>(he::Seconds::Ratio);
        fractionalSeconds -= static_cast<uint64_t>(fractionalSeconds);

        FormatTo(
            out,
            "[{:%Y-%m-%d_%H-%M-%S}{:.04f}] [{:s}]({}) {}\n",
            TimeFmt(now),
            fractionalSeconds,
            source.level,
            source.category,
            FmtJoin(kvs, kvs + count, ", "));
    }

    template <typename TimeFmt>
    static void FormatFileName(String& out)
    {
        SystemTime now = SystemClock::Now();
        FormatTo(out, "_{:%Y-%m-%d_%H-%M-%S}.log", TimeFmt(now));
    }

    FileSink::FileSink(Allocator& allocator) noexcept
        : m_buf(allocator)
    {
        m_buf.Reserve(BufferFlushSize + 1024);
    }

    FileSink::~FileSink() noexcept
    {
        Flush();
    }

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
        if (m_utc)
            FormatFileLogLine<FmtUtcTime>(m_buf, source, kvs, count, now);
        else
            FormatFileLogLine<FmtLocalTime>(m_buf, source, kvs, count, now);

        if (m_buf.Size() >= BufferFlushSize)
            Flush();
    }

    void FileSink::Flush()
    {
        if (m_file.IsOpen() && !m_buf.IsEmpty())
        {
            m_file.Write(m_buf.Data(), m_buf.Size());
        }
        m_buf.Clear();
    }

    void DebuggerSink(const void*, const LogSource& source, const KeyValue* kvs, uint32_t count)
    {
        const String msg = Format(
            "{}({}): [{:s}]({}) {}\n",
            source.file,
            source.line,
            source.level,
            source.category,
            FmtJoin(kvs, kvs + count, ", "));

        PrintToDebugger(msg.Data());
    }

    void ConsoleSink(const void*, const LogSource& source, const KeyValue* kvs, uint32_t count)
    {
        const String msg = Format("[{:s}]({}) {}\n", source.level, source.category, FmtJoin(kvs, kvs + count, ", "));

    #if defined(HE_PLATFORM_API_WASM)
        heWASM_ConsoleLogLevel level = heWASM_ConsoleLogLevel::Log;
        switch (source.level)
        {
            case LogLevel::Trace: level = heWASM_ConsoleLogLevel::Debug; break;
            case LogLevel::Debug: level = heWASM_ConsoleLogLevel::Debug; break;
            case LogLevel::Info: level = heWASM_ConsoleLogLevel::Info; break;
            case LogLevel::Warn: level = heWASM_ConsoleLogLevel::Warn; break;
            case LogLevel::Error: level = heWASM_ConsoleLogLevel::Error; break;
        }
        heWASM_ConsoleLog(level, msg.Data());
    #else
        if (source.level >= LogLevel::Warn)
            std::cerr << msg.Data() << std::endl;
        else
            std::cout << msg.Data() << std::endl;
    #endif
    }
}
