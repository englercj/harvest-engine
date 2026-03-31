// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/range_ops.h"

#include "he/core/span.h"
#include "he/core/test.h"

using namespace he;

namespace
{
    constexpr bool TestConstexprRangeSort()
    {
        int values[] = { 5, 1, 4, 2, 3 };
        RangeSort(values, HE_LENGTH_OF(values));
        return values[0] == 1
            && values[1] == 2
            && values[2] == 3
            && values[3] == 4
            && values[4] == 5;
    }

    constexpr bool TestConstexprRangeStableSort()
    {
        struct Item
        {
            int key;
            int order;
        };

        Item values[] =
        {
            { 2, 0 },
            { 1, 0 },
            { 2, 1 },
            { 1, 1 },
            { 2, 2 },
            { 1, 2 },
        };

        RangeStableSort(values, HE_LENGTH_OF(values), [](const Item& a, const Item& b)
        {
            return a.key < b.key;
        });

        return values[0].key == 1 && values[0].order == 0
            && values[1].key == 1 && values[1].order == 1
            && values[2].key == 1 && values[2].order == 2
            && values[3].key == 2 && values[3].order == 0
            && values[4].key == 2 && values[4].order == 1
            && values[5].key == 2 && values[5].order == 2;
    }

    static_assert(TestConstexprRangeSort());
    static_assert(TestConstexprRangeStableSort());
}

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

    // same-sized trivially-copyable mixed types should still use assignment conversion semantics
    {
        struct Src
        {
            uint32_t value;
            operator uint32_t() const { return value + 10u; }
        };

        uint32_t dst[] = { 0u, 0u, 0u };
        const Src src[] = { { 1u }, { 2u }, { 7u } };

        RangeCopy(dst, src, HE_LENGTH_OF(dst));
        HE_EXPECT_EQ(dst[0], 11u);
        HE_EXPECT_EQ(dst[1], 12u);
        HE_EXPECT_EQ(dst[2], 17u);
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

    // same-sized trivially-copyable mixed types should still use assignment conversion semantics
    {
        struct Src
        {
            uint32_t value;
            operator uint32_t() const { return value + 10u; }
        };

        uint32_t dst[] = { 0u, 0u, 0u };
        Src src[] = { { 1u }, { 2u }, { 7u } };

        RangeMove(dst, src, HE_LENGTH_OF(dst));
        HE_EXPECT_EQ(dst[0], 11u);
        HE_EXPECT_EQ(dst[1], 12u);
        HE_EXPECT_EQ(dst[2], 17u);
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
HE_TEST(core, range_ops, RangeSort)
{
    // trivial range of scalars, should sort ascending with the default predicate
    {
        int values[] = { 8, 1, 5, 2, 9, 4, 7, 3, 6, 0 };
        int expected[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

        RangeSort(values, HE_LENGTH_OF(values));
        HE_EXPECT_EQ_MEM(values, expected, sizeof(values));
    }

    // range overload with a custom predicate should preserve the requested ordering
    {
        struct Item
        {
            int key;
            int order;
        };

        Item values[] =
        {
            { 4, 0 },
            { 1, 0 },
            { 3, 0 },
            { 2, 0 },
            { 5, 0 },
        };

        Item expected[] =
        {
            { 5, 0 },
            { 4, 0 },
            { 3, 0 },
            { 2, 0 },
            { 1, 0 },
        };

        Span span(values);
        RangeSort(span, [](const Item& a, const Item& b)
        {
            return a.key > b.key;
        });

        for (uint32_t i = 0; i < HE_LENGTH_OF(values); ++i)
        {
            HE_EXPECT_EQ(values[i].key, expected[i].key);
            HE_EXPECT_EQ(values[i].order, expected[i].order);
        }
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, range_ops, RangeStableSort)
{
    struct Item
    {
        int key;
        int order;
    };

    // stable sort should preserve original order among equivalent keys
    {
        Item values[] =
        {
            { 2, 0 },
            { 1, 0 },
            { 2, 1 },
            { 1, 1 },
            { 2, 2 },
            { 1, 2 },
        };

        RangeStableSort(values, HE_LENGTH_OF(values), [](const Item& a, const Item& b)
        {
            return a.key < b.key;
        });

        const int expectedKeys[] = { 1, 1, 1, 2, 2, 2 };
        const int expectedOrders[] = { 0, 1, 2, 0, 1, 2 };

        for (uint32_t i = 0; i < HE_LENGTH_OF(values); ++i)
        {
            HE_EXPECT_EQ(values[i].key, expectedKeys[i]);
            HE_EXPECT_EQ(values[i].order, expectedOrders[i]);
        }
    }

    // default predicate should sort objects that define operator<
    {
        struct OrderedItem
        {
            int key;
            int order;

            bool operator<(const OrderedItem& x) const { return key < x.key; }
        };

        OrderedItem values[] =
        {
            { 3, 0 },
            { 1, 0 },
            { 4, 0 },
            { 1, 1 },
            { 5, 0 },
            { 9, 0 },
            { 2, 0 },
        };

        RangeStableSort(values, HE_LENGTH_OF(values));

        const int expectedKeys[] = { 1, 1, 2, 3, 4, 5, 9 };
        const int expectedOrders[] = { 0, 1, 0, 0, 0, 0, 0 };

        for (uint32_t i = 0; i < HE_LENGTH_OF(values); ++i)
        {
            HE_EXPECT_EQ(values[i].key, expectedKeys[i]);
            HE_EXPECT_EQ(values[i].order, expectedOrders[i]);
        }
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

    // floating-point ranges should follow operator== semantics rather than bitwise equality
    {
        const float positiveZero[] = { 0.0f, 1.0f };
        const float negativeZero[] = { -0.0f, 1.0f };
        const float nanValue = BitCast<float>(0x7fc00000u);
        const float nanA[] = { nanValue };
        const float nanB[] = { nanValue };

        HE_EXPECT(RangeEqual(positiveZero, negativeZero, HE_LENGTH_OF(positiveZero)));
        HE_EXPECT(!RangeEqual(nanA, nanB, HE_LENGTH_OF(nanA)));
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
