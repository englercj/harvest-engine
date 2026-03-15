// Copyright Chad Engler

#include "fixtures.h"

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
    const String testDir = GetTempTestPath(TestDir);

    Directory::RemoveContents(testDir.Data());
    Directory::Remove(testDir.Data());

    FileSink sink;

    Result r = sink.Configure(testDir.Data(), "prefix");
    HE_EXPECT(r, r);

    AddLogSink(sink);

    HE_LOGF_INFO(log_sinks_test, TestMsg);

    RemoveLogSink(sink);

    DirectoryScanner scanner;
    HE_EXPECT(scanner.Open(testDir.Data()));

    DirectoryScanner::Entry entry;
    if (scanner.NextEntry(entry))
    {
        String path = testDir;
        ConcatPath(path, entry.name);

        String data;
        HE_EXPECT(File::ReadAll(data, path.Data()));

        std::cout << "    " << data.Data() << std::endl;

        // only one entry expected.
        HE_EXPECT(!scanner.NextEntry(entry));
    }

    HE_EXPECT(Directory::RemoveContents(testDir.Data()));
    HE_EXPECT(Directory::Remove(testDir.Data()));
}
