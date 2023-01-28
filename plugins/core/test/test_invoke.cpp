// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/invoke.h"

#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
// Fixtures

static void _TestFunc_NoArgs_Void() {}
static int _TestFunc_NoArgs_Int() { return 0; }

static void _TestFunc_OneArg_Void(int) {}
static int _TestFunc_OneArg_Int(int v) { return v; }

static void _TestFunc_TwoArgs_Void(int, int) {}
static int _TestFunc_TwoArgs_Int(int v, int x) { return v + x; }

namespace
{
    struct _TestFunctor_NoArgs_Void { void operator()() {} };
    struct _TestFunctor_NoArgs_Int { int operator()() { return 5; } };

    struct _TestFunctor_OneArg_Void { void operator()(int) {} };
    struct _TestFunctor_OneArg_Int { int operator()(int v) { return v; } };

    struct _TestFunctor_TwoArgs_Void { void operator()(int, int) {} };
    struct _TestFunctor_TwoArgs_Int { int operator()(int v, int x) { return v + x; } };

    struct _TestObject
    {
        void MemberFunc_NoArgs_Void() {}
        int MemberFunc_NoArgs_Int() { return 10; }

        void MemberFunc_OneArg_Void(int) {}
        int MemberFunc_OneArg_Int(int v) { return v; }

        void MemberFunc_TwoArgs_Void(int, int) {}
        int MemberFunc_TwoArgs_Int(int v, int x) { return v + x; }

        int intValue = 20;
        const int constIntValue = 40;
    };
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, invoke, free_functions)
{
    Invoke(_TestFunc_NoArgs_Void);
    HE_EXPECT_EQ(Invoke(_TestFunc_NoArgs_Int), 0);

    Invoke(_TestFunc_OneArg_Void, 5);
    HE_EXPECT_EQ(Invoke(_TestFunc_OneArg_Int, 5), 5);

    Invoke(_TestFunc_TwoArgs_Void, 5, 5);
    HE_EXPECT_EQ(Invoke(_TestFunc_TwoArgs_Int, 5, 5), 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, invoke, functors)
{
    Invoke(_TestFunctor_NoArgs_Void{});
    HE_EXPECT_EQ(Invoke(_TestFunctor_NoArgs_Int{}), 5);

    Invoke(_TestFunctor_OneArg_Void{}, 5);
    HE_EXPECT_EQ(Invoke(_TestFunctor_OneArg_Int{}, 5), 5);

    Invoke(_TestFunctor_TwoArgs_Void{}, 5, 5);
    HE_EXPECT_EQ(Invoke(_TestFunctor_TwoArgs_Int{}, 5, 5), 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, invoke, pmf_object)
{
    _TestObject t;
    Invoke(&_TestObject::MemberFunc_NoArgs_Void, t);
    HE_EXPECT_EQ(Invoke(&_TestObject::MemberFunc_NoArgs_Int, t), 10);

    Invoke(&_TestObject::MemberFunc_OneArg_Void, t, 5);
    HE_EXPECT_EQ(Invoke(&_TestObject::MemberFunc_OneArg_Int, t, 5), 5);

    Invoke(&_TestObject::MemberFunc_TwoArgs_Void, t, 5, 5);
    HE_EXPECT_EQ(Invoke(&_TestObject::MemberFunc_TwoArgs_Int, t, 5, 5), 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, invoke, pmf_pointer)
{
    _TestObject t;
    Invoke(&_TestObject::MemberFunc_NoArgs_Void, &t);
    HE_EXPECT_EQ(Invoke(&_TestObject::MemberFunc_NoArgs_Int, &t), 10);

    Invoke(&_TestObject::MemberFunc_OneArg_Void, &t, 5);
    HE_EXPECT_EQ(Invoke(&_TestObject::MemberFunc_OneArg_Int, &t, 5), 5);

    Invoke(&_TestObject::MemberFunc_TwoArgs_Void, &t, 5, 5);
    HE_EXPECT_EQ(Invoke(&_TestObject::MemberFunc_TwoArgs_Int, &t, 5, 5), 10);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, invoke, pmd_object)
{
    _TestObject t;
    HE_EXPECT_EQ(Invoke(&_TestObject::intValue, t), 20);
    HE_EXPECT_EQ(Invoke(&_TestObject::constIntValue, t), 40);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, invoke, pmd_pointer)
{
    _TestObject t;
    HE_EXPECT_EQ(Invoke(&_TestObject::intValue, &t), 20);
    HE_EXPECT_EQ(Invoke(&_TestObject::constIntValue, &t), 40);
}
