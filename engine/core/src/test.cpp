// Copyright Chad Engler

#include "he/core/test.h"

#include <atomic>
#include <cstdio>
#include <cstdarg>

namespace he
{
    const TestInfo TestFixture::EmptyTestInfo{};

    static TestFixture* s_root{ nullptr };
    static std::atomic<int32_t> s_testFailures{ 0 };

    static void Print(const char* msg)
    {
        if (IsDebuggerAttached())
            OutputToDebugger(msg);
        fputs(msg, stdout);
    }

    TestFixture::TestFixture()
        : m_next(s_root)
    {
        s_root = this;
    }

    void TestFixture::Run()
    {
        TestBody();
    }

    void TestFixture::HandleTestFailure(const char* file, uint32_t line, const char* expr, const char* params)
    {
        ++s_testFailures;

        fmt::memory_buffer buf;
        fmt::format_to(buf, "{}({}): Expectation failed: {}\n", file, line, expr);

        if (!String::IsEmpty(params))
        {
            fmt::format_to(buf, "{}", params);
        }

        buf.push_back('\0');
        Print(buf.data());

        // TODO:
        // he::HandleError(he::ErrorType::Expect, file, line, "", expr, buf.data());
    }

    int32_t RunAllTests()
    {
        s_testFailures = 0;

        TestFixture* fixture = s_root;
        while (fixture)
        {
            {
                const TestInfo& info = fixture->GetTestInfo();
                fmt::memory_buffer buf;
                fmt::format_to(buf, "{}:{}:{}\n", info.mname, info.suite, info.name);
                buf.push_back('\0');
                Print(buf.data());
            }

            fixture->Run();
            fixture = fixture->m_next;
        }

        return s_testFailures;
    }
}
