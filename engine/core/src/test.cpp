// Copyright Chad Engler

#include "he/core/test.h"

#include "he/core/string.h"
#include "he/core/vector.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdarg>

namespace he
{
    const TestInfo TestFixture::EmptyTestInfo{};

    struct _TestRunner
    {
        static _TestRunner& Get()
        {
            static _TestRunner s_runner{};
            return s_runner;
        }

        Vector<TestFixture*> tests{ CrtAllocator::Get() };
        std::atomic<int32_t> failureCount{ 0 };
    };

    static void Print(const char* msg)
    {
        if (IsDebuggerAttached())
            OutputToDebugger(msg);
        fputs(msg, stdout);
    }

    TestFixture::TestFixture()
    {
        _TestRunner::Get().tests.PushBack(this);
    }

    void TestFixture::Run()
    {
        TestBody();
    }

    void SortTests()
    {
        _TestRunner& runner = _TestRunner::Get();

        std::sort(runner.tests.begin(), runner.tests.end(), [](const TestFixture* a, const TestFixture* b)
        {
            const TestInfo& infoA = a->GetTestInfo();
            const TestInfo& infoB = b->GetTestInfo();

            const int32_t moduleNameCmp = String::CompareI(infoA.moduleName, infoB.moduleName);
            if (moduleNameCmp != 0)
                return moduleNameCmp < 0;

            const int32_t suiteNameCmp = String::CompareI(infoA.suiteName, infoB.suiteName);
            if (suiteNameCmp != 0)
                return suiteNameCmp < 0;

            const int32_t testNameCmp = String::CompareI(infoA.testName, infoB.testName);
            if (testNameCmp != 0)
                return testNameCmp < 0;

            return false;
        });
    }

    int32_t RunAllTests()
    {
        SortTests();

        _TestRunner& runner = _TestRunner::Get();
        runner.failureCount = 0;

        for (TestFixture* fixture : runner.tests)
        {
            {
                const TestInfo& info = fixture->GetTestInfo();
                fmt::memory_buffer buf;
                fmt::format_to(fmt::appender(buf), "{}:{}:{}\n", info.moduleName, info.suiteName, info.testName);
                buf.push_back('\0');
                Print(buf.data());
            }

            fixture->Run();
        }

        return runner.failureCount;
    }

    void internal::HandleTestFailure(const char* file, uint32_t line, const char* expr, const char* params)
    {
        ++_TestRunner::Get().failureCount;

        fmt::memory_buffer buf;
        fmt::format_to(fmt::appender(buf), "{}({}): Expectation failed: {}\n", file, line, expr);

        if (!String::IsEmpty(params))
        {
            fmt::format_to(fmt::appender(buf), "{}", params);
        }

        buf.push_back('\0');
        Print(buf.data());

        // TODO:
        // he::HandleError(he::ErrorType::Expect, file, line, "", expr, buf.data());
    }
}
