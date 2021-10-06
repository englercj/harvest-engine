// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/appender.h"
#include "he/core/debug.h"
#include "he/core/enum_fmt.h"
#include "he/core/error.h"
#include "he/core/macros.h"
#include "he/core/memory_ops.h"
#include "he/core/result.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/core/types.h"
#include "he/core/utils.h"

#include "fmt/format.h"

#include <atomic>

/// Checks the expectation that `expr` evaluates to true.
///
/// Any additional parameters are logged as context.
#define HE_EXPECT(expr, ...) \
    do { \
        HE_PUSH_WARNINGS() \
        HE_DISABLE_MSVC_WARNING(4127) \
        ++he::internal::g_totalExpectations; \
        if (!(expr)) { \
            if constexpr (HE_PP_COUNT_ARGS(__VA_ARGS__) > 0) { \
                he::String _testParamFormatBuffer(Allocator::GetTemp()); \
                HE_PP_FOREACH(HE_EXPECT_PARAM_FORMATTER_, (__VA_ARGS__)) \
                he::internal::HandleTestFailure(__FILE__, __LINE__, #expr, _testParamFormatBuffer.Data()); \
            } else { \
                he::internal::HandleTestFailure(__FILE__, __LINE__, #expr, ""); \
            } \
            HE_DEBUG_BREAK(); \
        } \
        HE_POP_WARNINGS() \
    } while(0)

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
#define HE_EXPECT_EQ_STR(a, b) HE_EXPECT(String::Equal((a), (b)), a, b)

/// Check the expectation that the null terminated string `a` is not equal to the null terminated
/// string `b`.
///
/// Automatically passes `a` and `b` as context to be logged. Be careful, this macro may cause
/// the expressions to be evaluated more than once.
#define HE_EXPECT_NE_STR(a, b) HE_EXPECT(!String::Equal((a), (b)), a, b)

/// Check the expectation that the memory `a` is equal to the memory `b`.
#define HE_EXPECT_EQ_MEM(a, b, len) HE_EXPECT(MemEqual((a), (b), len))

/// Check the expectation that the memory `a` is not equal to the memory `b`.
#define HE_EXPECT_NE_MEM(a, b, len) HE_EXPECT(!MemEqual((a), (b), len))

/// Check the expectation that the pointer `a` points to the same memory as `b`.
#define HE_EXPECT_EQ_PTR(a, b) HE_EXPECT((a) == (b), fmt::ptr(a), fmt::ptr(b))

/// Check the expectation that the pointer `a` does not point to the same memory as `b`.
#define HE_EXPECT_NE_PTR(a, b) HE_EXPECT((a) != (b), fmt::ptr(a), fmt::ptr(b))

/// Check the expectation that the value `a` and the value `b` are within `diff` floating point steps from eachother.
#define HE_EXPECT_EQ_ULP(a, b, diff) HE_EXPECT(he::EqualUlp(a, b, diff), a, b)

/// Defines a test case for `module` in `suite` called `name`.
#define HE_TEST(module, suite, name) HE_TEST_(module, suite, name, he::TestFixture)

/// Defines a test case for `module` using `fixture` called `name`. The name of the `fixture`
/// class is also used as the suite name.
#define HE_TEST_F(module, fixture, name) HE_TEST_(module, fixture, name, fixture)

namespace he
{
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
        /// Constructs the test case and adds it to the global test list.
        TestFixture();

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

        /// Runs the test case.
        ///
        /// This is used internally to run the test.
        void Run();

        /// Returns the metadata about this test case.
        ///
        /// There is no need to implement this function manually because the
        /// #HE_TEST(module, suite, name) and #HE_TEST_F(module, suite, name) macros will
        /// do it for you.
        virtual const TestInfo& GetTestInfo() const { return EmptyTestInfo; }

        /// The body of the test case that contains expectations.
        ///
        /// This is used inernally to run the test.
        virtual void TestBody() = 0;

    private:
        static const TestInfo EmptyTestInfo;
    };

    /// Runs all the registered tests.
    ///
    /// \return Zero if all tests pass, or a non-zero value if there was a failure.
    int32_t RunAllTests();

    /// Checks if two floating point values are within `maxUlpDiff` floating point steps of eachother.
    ///
    /// Based on: https://www.gamasutra.com/view/news/162368/Indepth_Comparing_floating_point_numbers_2012_edition.php
    ///
    /// \param[in] a The first floating point value to compare.
    /// \param[in] b The second floating point value to compare.
    /// \param[in] maxUlpDiff The maximum tolerance for floating point ULP diff.
    /// \return True if the values are within tolerance, false otherwise.
    constexpr bool EqualUlp(float a, float b, int32_t maxUlpDiff)
    {

        // Treat -0 and +0 as equal
        if (a == b)
            return true;

        const int32_t ai = BitCast<int32_t>(a);
        const int32_t bi = BitCast<int32_t>(b);

        const bool asigned = (ai >> 31) != 0;
        const bool bsigned = (bi >> 31) != 0;

        // Different signs mean they do not match
        if (asigned != bsigned)
            return false;

        const int32_t ulpDiff = ai > bi ? ai - bi : bi - ai;

        return ulpDiff <= maxUlpDiff;
    }

namespace internal
{
    extern std::atomic<uint32_t> g_totalExpectations;

    /// Handler for a failed test.
    ///
    /// This is used internally and is not meant to be called directly.
    void HandleTestFailure(const char* file, uint32_t line, const char* expr, const char* params);
}
}

// Internal macro that generates the code for a test case.
#define HE_TEST_(module, suite, name, fixture) \
    static_assert(sizeof(HE_STRINGIFY(module)) > 1, "Test module name must not be empty"); \
    static_assert(sizeof(HE_STRINGIFY(suite)) > 1, "Test suite name must not be empty"); \
    static_assert(sizeof(HE_STRINGIFY(name)) > 1, "Test name must not be empty"); \
    class HE_TEST_CLASS_NAME_(module, suite, name) : public fixture { \
    public: \
        HE_TEST_CLASS_BODY_(HE_TEST_CLASS_NAME_(module, suite, name)); \
    private: \
        static const he::TestInfo TestInfo; \
        const he::TestInfo& GetTestInfo() const override { return TestInfo; } \
        void TestBody() override; \
    }; \
    const he::TestInfo HE_TEST_CLASS_NAME_(module, suite, name)::TestInfo{ HE_STRINGIFY(module), HE_STRINGIFY(suite), HE_STRINGIFY(name), __FILE__, __LINE__ }; \
    static HE_TEST_CLASS_NAME_(module, suite, name) HE_TEST_VARIABLE_NAME_(module, suite, name){}; \
    void HE_TEST_CLASS_NAME_(module, suite, name)::TestBody()

// Internal macro that generates the name of a test case class.
#define HE_TEST_CLASS_NAME_(module, suite, name) _heTestClass_ ## module ## _ ## suite ## _ ## name

// Internal macro that generates the name of a test case variable.
#define HE_TEST_VARIABLE_NAME_(module, suite, name) g_heTestInstance_ ## module ## _ ## suite ## _ ## name

// Internal macro that generates the body of the test case class. Since the name is reused
// so often, this makes it  abit more convenient.
#define HE_TEST_CLASS_BODY_(Class) \
    Class() = default; \
    ~Class() override = default; \
    Class(const Class&) = delete; \
    Class(Class&&) = delete; \
    Class& operator=(const Class&) = delete; \
    Class& operator=(Class&&) = delete

// Internal macro used in the format loop for HE_EXPECT params
#define HE_EXPECT_PARAM_FORMATTER_(x) \
    do { \
        constexpr char _exprString[] = #x " = "; \
        constexpr uint32_t _exprStringLen = HE_LENGTH_OF(_exprString) - 1; \
        _testParamFormatBuffer.Append(_exprString, _exprStringLen); \
        fmt::format_to(he::Appender(_testParamFormatBuffer), "{}\n", (x)); \
    } while (0);
