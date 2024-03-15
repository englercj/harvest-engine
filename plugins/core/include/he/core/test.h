// Copyright Chad Engler

#pragma once

#include "he/core/atomic.h"
#include "he/core/debug.h"
#include "he/core/error.h"
#include "he/core/fmt.h"
#include "he/core/key_value.h"
#include "he/core/log.h"
#include "he/core/macros.h"
#include "he/core/math.h"
#include "he/core/memory_ops.h"
#include "he/core/span.h"
#include "he/core/string_ops.h"
#include "he/core/string_fmt.h"
#include "he/core/types.h"
#include "he/core/utils.h"
#include "he/core/vector.h"

/// Checks the expectation that `expr` evaluates to true.
///
/// Any additional parameters are logged as context on a failure.
#define HE_EXPECT(expr, ...) \
    do { \
        HE_PUSH_WARNINGS() \
        HE_DISABLE_MSVC_WARNING(4127) \
        ++::he::internal::g_totalTestExpects; \
        if (!HE_ERROR_IF(Expect, expr, HE_PP_FOREACH(HE_EXPECT_PARAM_FORMATTER_, (__VA_ARGS__)))) { \
            ++::he::internal::g_totalTestFailures; \
        } \
        HE_POP_WARNINGS() \
    } while(0)

/// Checks that the provided block of code will trigger an error. Only checks for a single error
/// to occur, and only with an \ref ErrorKind of `kind`.
///
/// This macro works by overriding the global error handler until an error of the proper kind has
/// happened, or until the code completes without error.
#define HE_EXPECT_ERROR(Kind, ...) \
    do { \
        ::he::internal::ScopedExpectErrorHandler _expectErrorHandler(::he::ErrorKind::Kind); \
        __VA_ARGS__ \
        _expectErrorHandler.Release(); \
        HE_EXPECT(_expectErrorHandler.IsTriggered()); \
    } while (0)

/// Checks that the provided block of code will trigger an assertion.
#define HE_EXPECT_ASSERT(...) HE_EXPECT_ERROR(Assert, __VA_ARGS__)

/// Checks that the provided block of code will trigger an verification.
#define HE_EXPECT_VERIFY(...) HE_EXPECT_ERROR(Verify, __VA_ARGS__)

/// Checks the expectation that `a` is equal to `b`.
///
/// Automatically passes `a` and `b` as context to be logged. Be careful, this macro may cause
/// the expressions to be evaluated more than once.
#define HE_EXPECT_EQ(a, b) HE_EXPECT((a) == (b), a, b)

/// Checks the expectation that `a` is not equal to `b`.
///
/// Automatically passes `a` and `b` as context to be logged. Be careful, this macro may cause
/// the expressions to be evaluated more than once.
#define HE_EXPECT_NE(a, b) HE_EXPECT((a) != (b), a, b)

/// Checks the expectation that `a` is less than `b`.
///
/// Automatically passes `a` and `b` as context to be logged. Be careful, this macro may cause
/// the expressions to be evaluated more than once.
#define HE_EXPECT_LT(a, b) HE_EXPECT((a) < (b), a, b)

/// Checks the expectation that `a` is less than or equal to `b`.
///
/// Automatically passes `a` and `b` as context to be logged. Be careful, this macro may cause
/// the expressions to be evaluated more than once.
#define HE_EXPECT_LE(a, b) HE_EXPECT((a) <= (b), a, b)

/// Checks the expectation that `a` is greater than `b`.
///
/// Automatically passes `a` and `b` as context to be logged. Be careful, this macro may cause
/// the expressions to be evaluated more than once.
#define HE_EXPECT_GT(a, b) HE_EXPECT((a) > (b), a, b)

/// Checks the expectation that `a` is greater than or equal to `b`.
///
/// Automatically passes `a` and `b` as context to be logged. Be careful, this macro may cause
/// the expressions to be evaluated more than once.
#define HE_EXPECT_GE(a, b) HE_EXPECT((a) >= (b), a, b)

/// Check the expectation that the null terminated string `a` is equal to the null terminated
/// string `b`.
///
/// Automatically passes `a` and `b` as context to be logged. Be careful, this macro may cause
/// the expressions to be evaluated more than once.
#define HE_EXPECT_EQ_STR(a, b) HE_EXPECT(::he::StrEqual((a), (b)), a, b)

/// Check the expectation that the null terminated string `a` is not equal to the null terminated
/// string `b`.
///
/// Automatically passes `a` and `b` as context to be logged. Be careful, this macro may cause
/// the expressions to be evaluated more than once.
#define HE_EXPECT_NE_STR(a, b) HE_EXPECT(!::he::StrEqual((a), (b)), a, b)

/// Check the expectation that the memory `a` is equal to the memory `b`.
#define HE_EXPECT_EQ_MEM(a, b, len) HE_EXPECT(::he::MemEqual((a), (b), len))

/// Check the expectation that the memory `a` is not equal to the memory `b`.
#define HE_EXPECT_NE_MEM(a, b, len) HE_EXPECT(!::he::MemEqual((a), (b), len))

/// Check the expectation that the pointer `a` points to the same memory as `b`.
#define HE_EXPECT_EQ_PTR(a, b) HE_EXPECT((a) == (b), ::he::FmtPtr(a), ::he::FmtPtr(b))

/// Check the expectation that the pointer `a` does not point to the same memory as `b`.
#define HE_EXPECT_NE_PTR(a, b) HE_EXPECT((a) != (b), ::he::FmtPtr(a), ::he::FmtPtr(b))

/// Check the expectation that the value `a` and the value `b` are within `diff`
/// floating point value steps from each other.
#define HE_EXPECT_EQ_ULP(a, b, diff) HE_EXPECT(::he::IsNearlyEqualULP(a, b, diff), a, b)

/// Defines a test case for `module` in `suite` called `name`.
#define HE_TEST(module, suite, name) HE_TEST_(module, suite, name, ::he::TestFixture)

/// Defines a test case for `module` using `fixture` called `name`. The name of the `fixture`
/// class is also used as the suite name.
#define HE_TEST_F(module, suite, name, fixture) HE_TEST_(module, suite, name, fixture)

namespace he
{
    /// Type enum passed along to test logs to indicate what type of event it is.
    enum class TestEventKind : uint8_t
    {
        TestFailure,
        TestTiming,
    };

    /// General information about a test case.
    struct TestInfo
    {
        const char* moduleName; /// name of the module this test belongs to.
        const char* suiteName;  /// Name of the suite this test is in.
        const char* testName;   /// Name of the test case.
        const char* file;       /// File the test case is defined in.
        uint32_t line;          /// Line the test case is defined on.
    };

    /// Base fixture class that drives a test case. You can create custom fixtures by
    class TestFixture
    {
    public:
        /// Constructs the test case.
        TestFixture() = default;

        /// Destructs the test case.
        virtual ~TestFixture() = default;

        /// Called after the test case has run.
        ///
        /// Implement this function if you have setup work to perform before the test case runs.
        virtual void Before() {}

        /// Called after the test case has run.
        ///
        /// Implement this function if you have clean up work to perform after the test case runs.
        virtual void After() {}

    public:
        /// Runs the test case.
        ///
        /// This is used internally to run the test.
        void Run() { TestBody(); }

        /// Returns the metadata about this test case.
        ///
        /// There is no need to implement this function manually because the
        /// \ref HE_TEST and \ref HE_TEST_F macros will do it for you.
        virtual const TestInfo& GetTestInfo() const { return EmptyTestInfo; }

        /// The body of the test case that contains expectations.
        ///
        /// This is used inernally to run the test.
        virtual void TestBody() = 0;

    private:
        static const TestInfo EmptyTestInfo;
    };

    Vector<TestFixture*>& GetAllTests();

    /// Runs all the registered tests.
    ///
    /// \param[in] filter A string filter to limit the tests that are run. The filter string is
    ///     tested against the "fully qualified" test name, which looks like "module:suite:test".
    ///     If the filter string is contained anywhere within that string, it is included.
    /// \return The number of tests that failed.
    uint64_t RunAllTests(const char* filter = nullptr);

namespace internal
{
    extern Atomic<uint64_t> g_totalTestRuns;
    extern Atomic<uint64_t> g_totalTestExpects;
    extern Atomic<uint64_t> g_totalTestFailures;

    class ScopedExpectErrorHandler
    {
    public:
        ScopedExpectErrorHandler(ErrorKind kind);
        ~ScopedExpectErrorHandler() { Release(); }

        void Release();
        bool IsTriggered() const { return m_isTriggered; }

    private:
        static bool HandleError(void* ptr, const ErrorSource& source, const KeyValue* kvs, uint32_t count);

    private:
        Pfn_ErrorHandler m_old{ nullptr };
        void* m_oldUser{ nullptr };
        ErrorKind m_expectedKind{};
        bool m_handlerReset{ false };
        bool m_isTriggered{ false };
    };

    template <typename T>
    static bool RegisterTest()
    {
        static T s_instance{};
        GetAllTests().PushBack(&s_instance);
        return true;
    }
}
}

/// Internal macro that generates the name of a test case class.
/// \internal
#define HE_TEST_CLASS_NAME_(module, suite, name) _heTestClass_ ## module ## _ ## suite ## _ ## name

/// Internal macro that generates the body of the test case class. Since the name is reused
/// so often, this makes it a bit more convenient.
/// \internal
#define HE_TEST_DECL_(ClassName, ModuleNameStr, SuiteNameStr, TestNameStr, FixtureClassName) \
    static_assert(sizeof(ModuleNameStr) > 1, "Test module name must not be empty"); \
    static_assert(sizeof(SuiteNameStr) > 1, "Test suite name must not be empty"); \
    static_assert(sizeof(TestNameStr) > 1, "Test name must not be empty"); \
    class ClassName : public FixtureClassName { \
    public: \
        ClassName() = default; \
        ~ClassName() override = default; \
    private: \
        ClassName(const ClassName&) = delete; \
        ClassName(ClassName&&) = delete; \
        ClassName& operator=(const ClassName&) = delete; \
        ClassName& operator=(ClassName&&) = delete; \
        static inline bool s_registered = ::he::internal::RegisterTest<ClassName>(); \
        static const ::he::TestInfo TestInfo; \
        const ::he::TestInfo& GetTestInfo() const override { return TestInfo; } \
        void TestBody() override; \
    }; \
    const ::he::TestInfo ClassName::TestInfo{ ModuleNameStr, SuiteNameStr, TestNameStr, __FILE__, __LINE__ }; \
    void ClassName::TestBody()

/// Internal macro that generates the code for a test case.
/// \internal
#define HE_TEST_(module, suite, name, fixture) \
    HE_TEST_DECL_(HE_TEST_CLASS_NAME_(module, suite, name), HE_STRINGIFY(module), HE_STRINGIFY(suite), HE_STRINGIFY(name), fixture) \

/// Internal macro used in the format loop for HE_EXPECT params
/// \internal
#define HE_EXPECT_PARAM_FORMATTER_(x) HE_VAL(x),
