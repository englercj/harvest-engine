// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/macros.h"

#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, macros, LENGTH_OF)
{
    char x[5];
    static_assert(HE_LENGTH_OF(x) == 5);

    uint32_t y[50];
    static_assert(HE_LENGTH_OF(y) == 50);

    NonTrivial z[9];
    static_assert(HE_LENGTH_OF(z) == 9);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, macros, STRINGIFY)
{
    HE_EXPECT_EQ_STR(HE_STRINGIFY(test), "test");

    const char value[] = HE_STRINGIFY("something");
    HE_EXPECT_EQ_STR(value, "\"something\"");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, macros, UNIQUE_NAME)
{
    HE_PUSH_WARNINGS();
    HE_DISABLE_MSVC_WARNING(4101); // unused variable
    HE_DISABLE_GCC_CLANG_WARNING("-Wunused-variable");

    int HE_UNIQUE_NAME(a);
    int HE_UNIQUE_NAME(a);
    int HE_UNIQUE_NAME(a);
    int HE_UNIQUE_NAME(a);

    HE_POP_WARNINGS();
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, macros, PP_JOIN)
{
    int HE_PP_JOIN(foo, bar) = 0;
    HE_EXPECT_EQ(foobar, 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, macros, PP_EXPAND)
{
    int HE_PP_EXPAND(HE_PP_JOIN(foo, bar)) = 0;
    HE_EXPECT_EQ(foobar, 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, macros, PP_COUNT_ARGS)
{
    static_assert(HE_PP_COUNT_ARGS(1, 2, 3, 4, 5) == 5);
    static_assert(HE_PP_COUNT_ARGS(x, 1, *, |, %, =, (1,2), P) == 8);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, macros, PP_FIRST_ARG)
{
    static_assert(HE_PP_FIRST_ARG(1, 2, 3) == 1);
    static_assert(HE_PP_FIRST_ARG(5, 4) == 5);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, macros, PP_REST_ARGS)
{
    HE_PUSH_WARNINGS();
    HE_DISABLE_GCC_CLANG_WARNING("-Wunused-value");

    static_assert(HE_PP_REST_ARGS(1, 2) == 2);
    static_assert(HE_PP_REST_ARGS(1, 2, 3) == 3); // (2, 3) resolves to 3

    HE_POP_WARNINGS();

    HE_EXPECT_EQ_STR(HE_STRINGIFY(HE_PP_REST_ARGS(5, 4, 3)), "(4, 3)");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, macros, PP_FOREACH)
{
#define HE_TEST_VALUE_(x) (x * 2),

    constexpr int x[] =
    {
        HE_PP_FOREACH(HE_TEST_VALUE_, (1, 2, 3))
    };

    static_assert(HE_LENGTH_OF(x) == 3);
    static_assert(x[0] == 2);
    static_assert(x[1] == 4);
    static_assert(x[2] == 6);
}
