// Copyright Chad Engler

#include "he/core/test.h"

#include "he/core/alloca.h"
#include "he/core/atomic.h"
#include "he/core/fmt.h"
#include "he/core/clock.h"
#include "he/core/error.h"
#include "he/core/string.h"
#include "he/core/string_ops.h"

#include <algorithm>
#include <cstdlib>

namespace he
{
namespace internal
{
    Atomic<uint64_t> g_totalTestRuns{ 0 };
    Atomic<uint64_t> g_totalTestExpects{ 0 };
    Atomic<uint64_t> g_totalTestFailures{ 0 };

    ScopedExpectErrorHandler::ScopedExpectErrorHandler(ErrorKind kind)
        : m_expectedKind(kind)
    {
        m_old = GetErrorHandler(m_oldUser);
        SetErrorHandler(&ScopedExpectErrorHandler::HandleError, this);
    }

    void ScopedExpectErrorHandler::Release()
    {
        if (!m_handlerReset)
        {
            m_handlerReset = true;
            SetErrorHandler(m_old, m_oldUser);
        }
    }

    bool ScopedExpectErrorHandler::HandleError(void* ptr, const ErrorSource& source, const KeyValue* kvs, uint32_t count)
    {
        ScopedExpectErrorHandler* self = static_cast<ScopedExpectErrorHandler*>(ptr);
        if (source.kind == self->m_expectedKind)
        {
            self->m_handlerReset = true;
            self->m_isTriggered = true;
            SetErrorHandler(self->m_old, self->m_oldUser);
            return false;
        }
        else if (self->m_old)
        {
            return self->m_old(self->m_oldUser, source, kvs, count);
        }
        else
        {
            return DefaultErrorHandler(nullptr, source, kvs, count);
        }
    }
}

    template <>
    const char* EnumTraits<TestEventKind>::ToString(TestEventKind x) noexcept
    {
        switch (x)
        {
            case TestEventKind::TestFailure: return "TestFailure";
            case TestEventKind::TestTiming: return "TestTiming";
        }

        return "<unknown>";
    }

    const TestInfo TestFixture::EmptyTestInfo{};

    static void SortTests()
    {
        Vector<TestFixture*>& tests = GetAllTests();

        std::sort(tests.begin(), tests.end(), [](const TestFixture* a, const TestFixture* b)
        {
            const TestInfo& infoA = a->GetTestInfo();
            const TestInfo& infoB = b->GetTestInfo();

            const int32_t moduleNameCmp = StrCompI(infoA.moduleName, infoB.moduleName);
            if (moduleNameCmp != 0)
                return moduleNameCmp < 0;

            const int32_t suiteNameCmp = StrCompI(infoA.suiteName, infoB.suiteName);
            if (suiteNameCmp != 0)
                return suiteNameCmp < 0;

            const int32_t testNameCmp = StrCompI(infoA.testName, infoB.testName);
            if (testNameCmp != 0)
                return testNameCmp < 0;

            return false;
        });
    }

    static bool TestLibErrorHandler(void*, const ErrorSource& source, const KeyValue* kvs, uint32_t count)
    {
        switch (source.kind)
        {
            case ErrorKind::Assert:
            case ErrorKind::Except:
            case ErrorKind::Verify:
                return DefaultErrorHandler(nullptr, source, kvs, count);
            case ErrorKind::Expect:
                break;
        }

        KeyValue* newKVs = Allocator::GetDefault().NewArray<KeyValue>(count + 1);
        newKVs[0] = KeyValue("test_event_kind", TestEventKind::TestFailure);
        for (uint32_t i = 1; i <= count; ++i)
        {
            newKVs[i] = Move(kvs[i - 1]);
        }

        LogSource logSource;
        logSource.level = LogLevel::Error;
        logSource.line = source.line;
        logSource.file = source.file;
        logSource.funcName = source.funcName;
        logSource.category = "he_test";

        Log(logSource, newKVs, count + 1);

        Allocator::GetDefault().DeleteArray(newKVs);

        return true;
    }

    Vector<TestFixture*>& GetAllTests()
    {
        static Vector<TestFixture*> s_tests{};
        return s_tests;
    }

    uint64_t RunAllTests(const char* filter)
    {
        SortTests();

        String testFqn;

        const MonotonicTime startAll = MonotonicClock::Now();

        for (TestFixture* fixture : GetAllTests())
        {
            const TestInfo& info = fixture->GetTestInfo();

            if (!StrEmpty(filter))
            {
                testFqn.Clear();
                FormatTo(testFqn, "{}:{}:{}", info.moduleName, info.suiteName, info.testName);
                if (StrFind(testFqn.Data(), filter) == nullptr)
                    continue;
            }

            ++internal::g_totalTestRuns;

            HE_LOGF_INFO(he_test, "Running {}:{}:{}", info.moduleName, info.suiteName, info.testName);

            MonotonicTime start;
            MonotonicTime end;
            {
                ScopedErrorHandler errorGuard(TestLibErrorHandler);

                start = MonotonicClock::Now();

                fixture->Before();
                fixture->Run();
                fixture->After();

                end = MonotonicClock::Now();
            }

            HE_LOG_INFO(he_test,
                HE_KV(test_event_kind, TestEventKind::TestTiming),
                HE_KV(test_time_ns, (end - start).val),
                HE_KV(test_module_name, info.moduleName),
                HE_KV(test_suite_name, info.suiteName),
                HE_KV(test_name, info.testName));
        }

        const MonotonicTime endAll = MonotonicClock::Now();
        const Duration testTimeAll = endAll - startAll;


        HE_LOGF_INFO(he_test, "Ran {} tests with {} expectations in {:.4g} seconds. {} tests failed.",
            internal::g_totalTestRuns.Load(),
            internal::g_totalTestExpects.Load(),
            ToPeriod<Seconds, float>(testTimeAll),
            internal::g_totalTestFailures.Load());

        HE_LOG_INFO(he_test,
            HE_KV(test_event_kind, TestEventKind::TestTiming),
            HE_KV(test_time_ns, testTimeAll.val));

        return internal::g_totalTestFailures.Load();
    }
}
