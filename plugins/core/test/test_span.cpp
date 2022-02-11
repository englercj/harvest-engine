// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/span.h"

#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, span, Construct)
{
    static uint32_t s_data[]{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    {
        Span<uint32_t> s;
        SpanTestAttorney::Test(s, nullptr, 0);
    }

    {
        Span<uint32_t> s(nullptr, 0);
        SpanTestAttorney::Test(s, nullptr, 0);
    }

    {
        Span<uint32_t> s(nullptr, nullptr);
        SpanTestAttorney::Test(s, nullptr, 0);
    }

    {
        Span<uint32_t> s(s_data);
        SpanTestAttorney::Test(s, s_data, HE_LENGTH_OF(s_data));
    }

    {
        uint32_t* end = s_data + HE_LENGTH_OF(s_data);

        Span<uint32_t> s(s_data, end);
        SpanTestAttorney::Test(s, s_data, HE_LENGTH_OF(s_data));
    }

    {
        struct TestStdRange
        {
            uint8_t values[10];

            const uint8_t* data() const { return values; }
            uint8_t* data() { return values; }
            size_t size() const { return HE_LENGTH_OF(values); }
        };

        TestStdRange arr{ { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 } };
        Span<uint8_t> s(arr);
        SpanTestAttorney::Test(s, arr.values, HE_LENGTH_OF(arr.values));
    }

    {
        struct TestRange
        {
            uint8_t values[10];

            const uint8_t* Data() const { return values; }
            uint8_t* Data() { return values; }
            uint32_t Size() const { return HE_LENGTH_OF(values); }
        };

        TestRange arr{ { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 } };
        Span<uint8_t> s(arr);
        SpanTestAttorney::Test(s, arr.values, HE_LENGTH_OF(arr.values));
    }

    {
        constexpr uint32_t len = 4;
        Span<uint32_t> s(s_data, len);
        SpanTestAttorney::Test(s, s_data, len);

        Span<uint32_t> copyCtor(s);
        SpanTestAttorney::Test(copyCtor, s_data, len);

        Span<uint32_t> copyAssign;
        copyAssign = s;
        SpanTestAttorney::Test(copyAssign, s_data, len);

        Span<uint32_t> moveCtor(Move(s));
        SpanTestAttorney::Test(moveCtor, s_data, len);

        Span<uint32_t> moveAssign;
        moveAssign = Move(s);
        SpanTestAttorney::Test(moveAssign, s_data, len);
    }

    {
        static constexpr uint32_t s_cdata[]{ 0, 1, 2, 3, 4, };
        constexpr Span<const uint32_t> constSpan(s_cdata);

        static_assert(constSpan.Front() == s_cdata[0]);
        static_assert(constSpan.Back() == s_cdata[HE_LENGTH_OF(s_cdata) - 1]);
        static_assert(constSpan[1] == s_cdata[1]);
        static_assert(constSpan.Data() == s_cdata);
        static_assert(constSpan.Size() == HE_LENGTH_OF(s_cdata));
        static_assert(constSpan.IsEmpty() == false);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, span, operator_index)
{
    static uint32_t s_data[]{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    Span<uint32_t> s(s_data);
    SpanTestAttorney::Test(s, s_data, HE_LENGTH_OF(s_data));

    HE_EXPECT_EQ(s[0], 0);
    HE_EXPECT_EQ(s[5], 5);
    HE_EXPECT_EQ(s[9], 9);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, span, IsEmpty)
{
    {
        Span<uint32_t> s;
        SpanTestAttorney::Test(s, nullptr, 0);

        HE_EXPECT(s.IsEmpty());
    }

    {
        static uint32_t s_data[]{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

        Span<uint32_t> s(s_data);
        SpanTestAttorney::Test(s, s_data, HE_LENGTH_OF(s_data));

        HE_EXPECT(!s.IsEmpty());
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, span, Size)
{
    {
        Span<uint32_t> s;
        SpanTestAttorney::Test(s, nullptr, 0);

        HE_EXPECT_EQ(s.Size(), 0);
    }

    {
        static uint32_t s_data[]{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

        Span<uint32_t> s(s_data);
        SpanTestAttorney::Test(s, s_data, HE_LENGTH_OF(s_data));

        HE_EXPECT_EQ(s.Size(), HE_LENGTH_OF(s_data));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, span, Data)
{
    {
        Span<uint32_t> s;
        SpanTestAttorney::Test(s, nullptr, 0);

        HE_EXPECT(s.Data() == nullptr);
    }

    {
        static uint32_t s_data[]{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

        Span<uint32_t> s(s_data);
        SpanTestAttorney::Test(s, s_data, HE_LENGTH_OF(s_data));

        HE_EXPECT(s.Data() == s_data);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, span, Front)
{
    static uint32_t s_data[]{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    Span<uint32_t> s(s_data);
    SpanTestAttorney::Test(s, s_data, HE_LENGTH_OF(s_data));

    HE_EXPECT_EQ(s.Front(), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, span, Back)
{
    static uint32_t s_data[]{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    Span<uint32_t> s(s_data);
    SpanTestAttorney::Test(s, s_data, HE_LENGTH_OF(s_data));

    HE_EXPECT_EQ(s.Back(), 9);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, span, AsBytes)
{
    static uint32_t s_data[]{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    Span<uint32_t> s(s_data);
    Span<const uint8_t> b = s.AsBytes();

    HE_EXPECT_EQ(s.Size() * sizeof(uint32_t), b.Size());
    HE_EXPECT_EQ_MEM(s.Data(), b.Data(), b.Size());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, span, Begin)
{
    {
        Span<uint32_t> s;
        SpanTestAttorney::Test(s, nullptr, 0);

        HE_EXPECT(s.Begin() == nullptr);
    }

    {
        static uint32_t s_data[]{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

        Span<uint32_t> s(s_data);
        SpanTestAttorney::Test(s, s_data, HE_LENGTH_OF(s_data));

        HE_EXPECT(s.Begin() == s_data);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, span, End)
{
    {
        Span<uint32_t> s;
        SpanTestAttorney::Test(s, nullptr, 0);

        HE_EXPECT(s.End() == nullptr);
    }

    {
        static uint32_t s_data[]{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

        Span<uint32_t> s(s_data);
        SpanTestAttorney::Test(s, s_data, HE_LENGTH_OF(s_data));

        HE_EXPECT(s.End() == (s_data + HE_LENGTH_OF(s_data)));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, span, RangeBasedFor)
{
    static uint32_t s_data[]{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    Span<uint32_t> s(s_data);

    uint32_t i = 0;
    for (uint32_t v : s)
    {
        HE_EXPECT_EQ(v, s_data[i++]);
    }
    HE_EXPECT_EQ(i, HE_LENGTH_OF(s_data));
}
