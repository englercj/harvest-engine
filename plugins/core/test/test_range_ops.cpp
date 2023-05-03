// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/range_ops.h"

#include "he/core/span.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, range_ops, RangeCopy)
{
    // trivial range of ints, should be optimized to a memmove
    {
        int a[] = { 1, 2, 3, 4, 5 };
        int b[] = { 0, 0, 0, 0, 0 };

        RangeCopy(a, b, 5);
        HE_EXPECT_EQ_MEM(a, b, sizeof(a));
    }

    // trivial range of objects, should be optimized to a memmove
    {
        struct A { int a; };
        A a[] = { { 1 }, { 2 }, { 3 }, { 4 }, { 5 } };
        A b[] = { { 0 }, { 0 }, { 0 }, { 0 }, { 0 } };

        RangeCopy(a, b, 5);
        HE_EXPECT_EQ_MEM(a, b, sizeof(a));
    }

    // span of ints, should be optimized to a memmove
    {
        int a[] = { 1, 2, 3, 4, 5 };
        int b[] = { 0, 0, 0, 0, 0 };

        Span sa(a);
        RangeCopy(sa, Span(b));
        HE_EXPECT_EQ_MEM(a, b, sizeof(a));
    }

    // copyable objects, should loop and assign
    {
        CopyOnly a[5]{};
        CopyOnly b[5]{};

        for (uint32_t i = 0; i < HE_LENGTH_OF(a); ++i)
        {
            HE_EXPECT(!a[i].copyAssigned);
        }

        RangeCopy(a, b, 5);

        for (uint32_t i = 0; i < HE_LENGTH_OF(a); ++i)
        {
            HE_EXPECT(a[i].copyAssigned);
        }
    }

    // copyable and moveable objects, should loop and assign
    {
        CopyAndMove a[5]{};
        CopyAndMove b[5]{};

        for (uint32_t i = 0; i < HE_LENGTH_OF(a); ++i)
        {
            HE_EXPECT(!a[i].copyAssigned);
        }

        RangeCopy(a, b, 5);

        for (uint32_t i = 0; i < HE_LENGTH_OF(a); ++i)
        {
            HE_EXPECT(a[i].copyAssigned);
        }
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, range_ops, RangeMove)
{
    // trivial range of ints, should be optimized to a memmove
    {
        int a[] = { 1, 2, 3, 4, 5 };
        int b[] = { 0, 0, 0, 0, 0 };

        RangeMove(a, b, 5);
        HE_EXPECT_EQ_MEM(a, b, sizeof(a));
    }

    // trivial range of objects, should be optimized to a memmove
    {
        struct A { int a; };
        A a[] = { { 1 }, { 2 }, { 3 }, { 4 }, { 5 } };
        A b[] = { { 0 }, { 0 }, { 0 }, { 0 }, { 0 } };

        RangeMove(a, b, 5);
        HE_EXPECT_EQ_MEM(a, b, sizeof(a));
    }

    // span of ints, should be optimized to a memmove
    {
        int a[] = { 1, 2, 3, 4, 5 };
        int b[] = { 0, 0, 0, 0, 0 };

        Span sa(a);
        Span sb(b);
        RangeMove(sa, sb);
        HE_EXPECT_EQ_MEM(a, b, sizeof(a));
    }

    // moveable objects, should loop and assign
    {
        MoveOnly a[5]{};
        MoveOnly b[5]{};

        for (uint32_t i = 0; i < HE_LENGTH_OF(a); ++i)
        {
            HE_EXPECT(!a[i].moveAssigned);
        }

        RangeMove(a, b, 5);

        for (uint32_t i = 0; i < HE_LENGTH_OF(a); ++i)
        {
            HE_EXPECT(a[i].moveAssigned);
        }
    }

    // copyable objects, should loop and assign
    {
        CopyAndMove a[5]{};
        CopyAndMove b[5]{};

        for (uint32_t i = 0; i < HE_LENGTH_OF(a); ++i)
        {
            HE_EXPECT(!a[i].copyAssigned);
            HE_EXPECT(!a[i].moveAssigned);
        }

        RangeMove(a, b, 5);

        for (uint32_t i = 0; i < HE_LENGTH_OF(a); ++i)
        {
            HE_EXPECT(!a[i].copyAssigned);
            HE_EXPECT(a[i].moveAssigned);
        }
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, range_ops, RangeFill)
{
    // trivial range of chars, should be optimized to a memset
    {
        char a[] = { 'z', 'x', 'c', 'v', 'b' };
        char b[] = { 'a', 'a', 'a', 'a', 'a' };

        RangeFill(a, 5, 'a');
        HE_EXPECT_EQ_MEM(a, b, sizeof(a));
    }

    // trivial range of scalars, should be optimized to a memset zero
    {
        int a[] = { 1, 2, 3, 4, 5 };
        int b[] = { 0, 0, 0, 0, 0 };

        RangeFill(a, 5, 0);
        HE_EXPECT_EQ_MEM(a, b, sizeof(a));
    }

    // trivial range of objects, should loop and assign
    {
        struct A { int a; };
        A a[] = { { 1 }, { 2 }, { 3 }, { 4 }, { 5 } };
        A b[] = { { 8 }, { 8 }, { 8 }, { 8 }, { 8 } };

        RangeFill(a, 5, A{ 8 });
        HE_EXPECT_EQ_MEM(a, b, sizeof(a));
    }

    // span of ints, should loop and assign
    {
        int a[] = { 1, 2, 3, 4, 5 };
        int b[] = { 8, 8, 8, 8, 8 };

        Span sa(a);
        RangeFill(sa, 8);
        HE_EXPECT_EQ_MEM(a, b, sizeof(a));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, range_ops, RangeFind)
{
    // trivial range of chars, should be optimized to a memchr
    {
        char a[] = { 'z', 'x', 'c', 'v', 'b' };

        HE_EXPECT_EQ_PTR(RangeFind(a, 5, 'x'), a + 1);
        HE_EXPECT_EQ_PTR(RangeFind(a, 5, 'v'), a + 3);
        HE_EXPECT(!RangeFind(a, 5, 'a'));
    }

    // trivial range of scalars, should loop and compare
    {
        int a[] = { 1, 2, 3, 4, 5 };

        HE_EXPECT_EQ_PTR(RangeFind(a, 5, 2), a + 1);
        HE_EXPECT_EQ_PTR(RangeFind(a, 5, 4), a + 3);
        HE_EXPECT(!RangeFind(a, 5, 8));
    }

    // trivial range of objects, should loop and compare
    {
        struct A { int a; bool operator==(const A& x) const { return a == x.a; } };
        A a[] = { { 1 }, { 2 }, { 3 }, { 4 }, { 5 } };

        HE_EXPECT_EQ_PTR(RangeFind(a, 5, A{ 2 }), a + 1);
        HE_EXPECT_EQ_PTR(RangeFind(a, 5, A{ 4 }), a + 3);
        HE_EXPECT(!RangeFind(a, 5, A{ 8 }));
    }

    // span of objects, should loop and compare
    {
        struct A { int a; bool operator==(const A& x) const { return a == x.a; } };
        A a[] = { { 1 }, { 2 }, { 3 }, { 4 }, { 5 } };

        Span sa(a);
        HE_EXPECT_EQ_PTR(RangeFind(sa, A{ 2 }), a + 1);
        HE_EXPECT_EQ_PTR(RangeFind(sa, A{ 4 }), a + 3);
        HE_EXPECT(!RangeFind(sa, A{ 8 }));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, range_ops, RangeFindIf)
{
    // trivial range of chars
    {
        char a[] = { 'z', 'x', 'c', 'v', 'b' };

        auto pred1 = [](char c) { return c == 'x'; };
        auto pred2 = [](char c) { return c == 'v'; };
        auto pred3 = [](char c) { return c == 'a'; };

        HE_EXPECT_EQ_PTR(RangeFindIf(a, 5, pred1), a + 1);
        HE_EXPECT_EQ_PTR(RangeFindIf(a, 5, pred2), a + 3);
        HE_EXPECT(!RangeFindIf(a, 5, pred3));
    }

    // trivial range of scalars
    {
        int a[] = { 1, 2, 3, 4, 5 };

        auto pred1 = [](int i) { return i == 2; };
        auto pred2 = [](int i) { return i == 4; };
        auto pred3 = [](int i) { return i == 8; };

        HE_EXPECT_EQ_PTR(RangeFindIf(a, 5, pred1), a + 1);
        HE_EXPECT_EQ_PTR(RangeFindIf(a, 5, pred2), a + 3);
        HE_EXPECT(!RangeFindIf(a, 5, pred3));
    }

    // trivial range of objects
    {
        struct A { int a; };
        A a[] = { { 1 }, { 2 }, { 3 }, { 4 }, { 5 } };

        auto pred1 = [](const A& a) { return a.a == 2; };
        auto pred2 = [](const A& a) { return a.a == 4; };
        auto pred3 = [](const A& a) { return a.a == 8; };

        HE_EXPECT_EQ_PTR(RangeFindIf(a, 5, pred1), a + 1);
        HE_EXPECT_EQ_PTR(RangeFindIf(a, 5, pred2), a + 3);
        HE_EXPECT(!RangeFindIf(a, 5, pred3));
    }

    // span of objects
    {
        struct A { int a; bool operator==(const A& x) const { return a == x.a; } };
        A a[] = { { 1 }, { 2 }, { 3 }, { 4 }, { 5 } };

        auto pred1 = [](const A& a) { return a.a == 2; };
        auto pred2 = [](const A& a) { return a.a == 4; };
        auto pred3 = [](const A& a) { return a.a == 8; };

        Span sa(a);
        HE_EXPECT_EQ_PTR(RangeFindIf(sa, pred1), a + 1);
        HE_EXPECT_EQ_PTR(RangeFindIf(sa, pred2), a + 3);
        HE_EXPECT(!RangeFindIf(sa, pred3));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, range_ops, RangeEqual)
{
    // trivial range of chars, should be optimized to a memcmp
    {
        char a[] = { 'z', 'x', 'c', 'v', 'b' };
        char b[] = { 'a', 'b', 'c', 'd', 'e' };

        HE_EXPECT(RangeEqual(a, a, 5));
        HE_EXPECT(RangeEqual(b, b, 5));
        HE_EXPECT(!RangeEqual(a, b, 5));
    }

    // trivial range of scalars, should be optimized to a memcmp
    {
        int a[] = { 1, 2, 3, 4, 5 };
        int b[] = { 1, 2, 3, 6, 7 };

        HE_EXPECT(RangeEqual(a, a, 5));
        HE_EXPECT(RangeEqual(b, b, 5));
        HE_EXPECT(!RangeEqual(a, b, 5));
    }

    // trivial range of objects, should loop and compare
    {
        struct A { int a; bool operator==(const A& x) const { return a == x.a; } };
        A a[] = { { 1 }, { 2 }, { 3 }, { 4 }, { 5 } };
        A b[] = { { 1 }, { 2 }, { 3 }, { 6 }, { 7 } };

        HE_EXPECT(RangeEqual(a, a, 5));
        HE_EXPECT(RangeEqual(b, b, 5));
        HE_EXPECT(!RangeEqual(a, b, 5));
    }

    // span of objects, should loop and compare
    {
        struct A { int a; bool operator==(const A& x) const { return a == x.a; } };
        A a[] = { { 1 }, { 2 }, { 3 }, { 4 }, { 5 } };
        A b[] = { { 1 }, { 2 }, { 3 }, { 6 }, { 7 } };

        Span sa(a);
        Span sb(b);
        HE_EXPECT(RangeEqual(sa, sa));
        HE_EXPECT(RangeEqual(sb, sb));
        HE_EXPECT(!RangeEqual(sa, sb));
    }

    // trivial range of chars, should loop and compare
    {
        char a[] = { 'z', 'x', 'c', 'v', 'b' };
        char b[] = { 'a', 'b', 'c', 'd', 'e' };

        auto pred = [](char a, char b) { return a == b; };

        HE_EXPECT(RangeEqual(a, a, 5, pred));
        HE_EXPECT(RangeEqual(b, b, 5, pred));
        HE_EXPECT(!RangeEqual(a, b, 5, pred));
    }

    // trivial range of scalars, should loop and compare
    {
        int a[] = { 1, 2, 3, 4, 5 };
        int b[] = { 1, 2, 3, 6, 7 };

        auto pred = [](int a, int b) { return a == b; };

        HE_EXPECT(RangeEqual(a, a, 5, pred));
        HE_EXPECT(RangeEqual(b, b, 5, pred));
        HE_EXPECT(!RangeEqual(a, b, 5, pred));
    }

    // trivial range of objects, should loop and compare
    {
        struct A { int a; };
        A a[] = { { 1 }, { 2 }, { 3 }, { 4 }, { 5 } };
        A b[] = { { 1 }, { 2 }, { 3 }, { 6 }, { 7 } };

        auto pred = [](const A& a, const A& b) { return a.a == b.a; };

        HE_EXPECT(RangeEqual(a, a, 5, pred));
        HE_EXPECT(RangeEqual(b, b, 5, pred));
        HE_EXPECT(!RangeEqual(a, b, 5, pred));
    }

    // span of objects, should loop and compare
    {
        struct A { int a; };
        A a[] = { { 1 }, { 2 }, { 3 }, { 4 }, { 5 } };
        A b[] = { { 1 }, { 2 }, { 3 }, { 6 }, { 7 } };

        auto pred = [](const A& a, const A& b) { return a.a == b.a; };

        Span sa(a);
        Span sb(b);
        HE_EXPECT(RangeEqual(sa, sa, pred));
        HE_EXPECT(RangeEqual(sb, sb, pred));
        HE_EXPECT(!RangeEqual(sa, sb, pred));
    }
}
