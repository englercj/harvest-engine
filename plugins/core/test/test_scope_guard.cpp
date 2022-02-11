// Copyright Chad Engler

#include "he/core/scope_guard.h"

#include "he/core/test.h"
#include "he/core/utils.h"

using namespace he;

static int s_count;
static void OnScopeExit() { ++s_count; }

// ------------------------------------------------------------------------------------------------
HE_TEST(core, scope_guard, AT_SCOPE_EXIT)
{
    s_count = 0;

    // Function pointer
    {
        HE_AT_SCOPE_EXIT(OnScopeExit);
    }
    HE_EXPECT_EQ(s_count, 1);

    // Lambda rvalue
    {
        HE_AT_SCOPE_EXIT([&] { ++s_count; });
    }
    HE_EXPECT_EQ(s_count, 2);

    // Lambda lvalue
    {
        auto f = [&] { ++s_count; };
        HE_AT_SCOPE_EXIT(f);
    }
    HE_EXPECT_EQ(s_count, 3);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, scope_guard, MakeScopeGuard)
{
    s_count = 0;

    // Function pointer
    {
        auto x = MakeScopeGuard(OnScopeExit);
    }
    HE_EXPECT_EQ(s_count, 1);

    // Lambda rvalue
    {
        auto x = MakeScopeGuard([&] { ++s_count; });
    }
    HE_EXPECT_EQ(s_count, 2);

    // Lambda lvalue
    {
        auto f = [&] { ++s_count; };
        auto x = MakeScopeGuard(f);
    }
    HE_EXPECT_EQ(s_count, 3);

    // Move constructor
    {
        auto x = MakeScopeGuard(OnScopeExit);
        auto y(Move(x));
    }
    HE_EXPECT_EQ(s_count, 4);

    // Move assignment
    {
        auto x = MakeScopeGuard(OnScopeExit);
        auto y = MakeScopeGuard(OnScopeExit);
        y = Move(x);
    }
    HE_EXPECT_EQ(s_count, 5);

    // Dismiss
    {
        auto x = MakeScopeGuard(OnScopeExit);
        x.Dismiss();
    }
    HE_EXPECT_EQ(s_count, 5);
}
