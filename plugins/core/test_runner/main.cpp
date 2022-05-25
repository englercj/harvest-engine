// Copyright Chad Engler

#include "he/core/args.h"
#include "he/core/appender.h"
#include "he/core/assert.h"
#include "he/core/clock.h"
#include "he/core/debugger.h"
#include "he/core/key_value.h"
#include "he/core/key_value_fmt.h"
#include "he/core/log.h"
#include "he/core/log_sinks.h"
#include "he/core/macros.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/core/test.h"

#include "fmt/format.h"

#include <iostream>

using namespace he;

// TODO: JUnit XML output format support
//      See: https://github.com/windyroad/JUnit-Schema/blob/master/JUnit.xsd
//      See: https://stackoverflow.com/questions/4922867/what-is-the-junit-xml-format-specification-that-hudson-supports
//      See: https://www.ibm.com/docs/en/developer-for-zos/9.1.1?topic=formats-junit-xml-format

// TODO: GitHub Markdown output format support (Job Summary)
//      See: https://github.blog/2022-05-09-supercharging-github-actions-with-job-summaries/

struct AppArgs
{
    bool help{ false };
    bool timings{ false };
    const char* filter{ nullptr };
    //const char* junitOutFile{ nullptr };
    //const char* markdownOutFile{ nullptr };
};
static AppArgs s_args;

static const KeyValue* FindKV(const char* key, const KeyValue* kvs, uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        const KeyValue* kv = kvs + i;

        if (String::Equal(key, kv->Key()))
            return kv;
    }

    return nullptr;
}

static const KeyValue& GetKV(const char* key, const KeyValue* kvs, uint32_t count)
{
    const KeyValue* kv = FindKV(key, kvs, count);
    HE_ASSERT(kv != nullptr, HE_MSG("Unable to find key name. This may be an invalid log entry."), HE_VAL(key));
    return *kv;
}

static bool HandleTestLibLogEntry(const LogSource& source, const KeyValue* kvs, uint32_t count)
{
    // Handle errors from the test framework
    if (source.level == LogLevel::Error)
    {
        // Log the initial failure line
        const ErrorKind kind = kvs[0].GetEnum<ErrorKind>(); // error_kind
        const String& expr = kvs[1].GetString(); // error_expr
        std::cout << AsString(kind) << " failed: " << expr.Data() << " [" << source.file << '(' << source.line << ")]" << std::endl;

        // First two keys are `error_kind` and `error_expr` so skip those and log the rest
        if (count > 2)
        {
            String buf;
            fmt::format_to(Appender(buf), "    {}", fmt::join(kvs + 2, kvs + count, "\n    "));
            std::cout << buf.Data() << std::endl;
        }

        return true;
    }

    // Check for the test_kind key which is a sentinel for special handling of test events
    // If we don't find it, we consider this a "normal" log and don't handle it here.
    const KeyValue* kvKind = FindKV("test_event_kind", kvs, count);
    if (!kvKind)
        return false;

    // Special handling of test lifecycle events
    const TestEventKind kind = kvKind->GetEnum<TestEventKind>();
    switch (kind)
    {
        case TestEventKind::TestTiming:
        {
            if (s_args.timings)
            {
                const int64_t timeNs = GetKV("test_time_ns", kvs, count).GetInt();
                const double timeMs = ToPeriod<Milliseconds, double>({ timeNs });
                std::cout << "    -> test took " << timeMs << " ms" << std::endl;
            }
            break;
        }
    }

    return true;
}

static void TestRunnerLogSink(void*, const LogSource& source, const KeyValue* kvs, uint32_t count)
{
    if (String::Equal(source.category, "he_test"))
    {
        if (HandleTestLibLogEntry(source, kvs, count))
            return;
    }

    ConsoleSink(nullptr, source, kvs, count);
}

#include "he/core/main.inl"
int he::AppMain(int argc, char* argv[])
{
    if (IsDebuggerAttached())
        AddLogSink(DebuggerSink);

    AddLogSink(TestRunnerLogSink);

    static ArgDesc argDescriptors[] =
    {
        { s_args.help,        'h', "help",        "Output this help text." },
        { s_args.timings,     't', "times"        "Output timings for test runs in milliseconds." },
        { s_args.filter,      'f', "filter"       "Search string to filter the tests that are run." },
        //{ s_args.junitOutFile,     "junit",       "File to output junit formatted results to." },
        //{ s_args.markdownOutFile,  "markdown",    "File to output github-flavored markdown formatted results to." },
    };

    ArgResult result = ParseArgs(argDescriptors, argc, argv);

    if (!result || s_args.help)
    {
        String help = MakeHelpString(argDescriptors, argv[0], &result);
        std::cerr << help.Data() << std::endl;
        return -1;
    }

    return static_cast<int>(RunAllTests(s_args.filter));
}
