// Copyright Chad Engler

#include "he/core/log_sinks.h"

#include "he/core/directory.h"
#include "he/core/path.h"
#include "he/core/test.h"

#include <iostream>

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, log_sinks, FormatKVsTo)
{
    const LogKV kvs[] =
    {
        { "bool", true },
        { "int", 10 },
        { "uint", 20u },
        { "double", 50.12 },
        { "str", "test" },
    };

    String values;
    FormatKVsTo(values, kvs, HE_LENGTH_OF(kvs));

    HE_EXPECT_EQ_STR(values.Data(), "bool = true, int = 10, uint = 20, double = 50.12, str = test");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, log_sinks, DebuggerSink)
{
    DebuggerSink sink;
    AddLogSink(sink);

    HE_LOGF_INFO(log_sinks_test, "Testing debugger sink.");

    RemoveLogSink(sink);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, log_sinks, FileSink)
{
    constexpr char TestDir[] = "ed7b27d0-054a-4080-a225-799091865a1a";
    constexpr char TestMsg[] = "Testing file sink.";

    FileSink sink;

    Result r = sink.Configure(TestDir, "prefix");

    AddLogSink(sink);

    HE_LOGF_INFO(log_sinks_test, TestMsg);

    RemoveLogSink(sink);

    DirectoryScanner scanner;
    HE_EXPECT(scanner.Open(TestDir));

    DirectoryScanner::Entry entry;
    if (scanner.NextEntry(entry))
    {
        String path = TestDir;
        ConcatPath(path, entry.name);

        Vector<char> data;
        HE_EXPECT(File::ReadAll(data, path.Data()));
        data.PushBack('\0');

        std::cout << "    " << data.Data() << std::endl;

        // only one entry expected.
        HE_EXPECT(!scanner.NextEntry(entry));
    }
}
