// Copyright Chad Engler

#include "he/core/log_sinks.h"

#include "he/core/directory.h"
#include "he/core/path.h"
#include "he/core/result_fmt.h"
#include "he/core/test.h"

#include <iostream>

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, log_sinks, DebuggerSink)
{
    AddLogSink(DebuggerSink);

    HE_LOGF_INFO(log_sinks_test, "Testing debugger sink.");

    RemoveLogSink(DebuggerSink);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, log_sinks, FileSink)
{
    constexpr char TestDir[] = "ed7b27d0-054a-4080-a225-799091865a1a";
    constexpr char TestMsg[] = "Testing file sink.";

    Directory::RemoveContents(TestDir);
    Directory::Remove(TestDir);

    FileSink sink;

    Result r = sink.Configure(TestDir, "prefix");
    HE_EXPECT(r, r);

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

        String data;
        HE_EXPECT(File::ReadAll(data, path.Data()));

        std::cout << "    " << data.Data() << std::endl;

        // only one entry expected.
        HE_EXPECT(!scanner.NextEntry(entry));
    }
}
