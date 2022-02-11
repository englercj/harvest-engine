// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/string_view.h"

#include "he/core/allocator.h"
#include "he/core/string.h"
#include "he/core/string_view_fmt.h"
#include "he/core/test.h"

#include <string>

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_view, Construct)
{
    constexpr char TestString[] = "Hello, world!";
    constexpr uint32_t TestStringLen = HE_LENGTH_OF(TestString) - 1;

    {
        StringView s;
        StringViewTestAttorney::Test(s, nullptr, 0);
    }

    {
        StringView s(TestString);
        StringViewTestAttorney::Test(s, TestString, TestStringLen);
    }

    {
        String str(TestString);

        StringView s(str);
        StringViewTestAttorney::Test(s, str.Data(), str.Size());
    }

    {
        std::string str(TestString);

        StringView s(str);
        StringViewTestAttorney::Test(s, str.data(), static_cast<uint32_t>(str.size()));
    }

    {
        StringView s(TestString, TestString + TestStringLen);
        StringViewTestAttorney::Test(s, TestString, TestStringLen);
    }

    {
        StringView s(TestString, TestStringLen);
        StringViewTestAttorney::Test(s, TestString, TestStringLen);
    }

    {
        constexpr uint32_t len = 4;
        StringView s(TestString, len);
        StringViewTestAttorney::Test(s, TestString, len);

        StringView copyCtor(s);
        StringViewTestAttorney::Test(copyCtor, TestString, len);

        StringView copyAssign;
        copyAssign = s;
        StringViewTestAttorney::Test(copyAssign, TestString, len);

        StringView moveCtor(Move(s));
        StringViewTestAttorney::Test(moveCtor, TestString, len);

        StringView moveAssign;
        moveAssign = Move(s);
        StringViewTestAttorney::Test(moveAssign, TestString, len);
    }

    {
        constexpr char TestStringWithNull[] = "Hello\0world!";
        StringView s(TestStringWithNull);
        StringViewTestAttorney::Test(s, TestStringWithNull, 5);
        HE_EXPECT_EQ_STR(s.Data(), "Hello");
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_view, operator_index)
{
    constexpr char TestString[] = "Hello, world!";

    StringView s(TestString);
    StringViewTestAttorney::Test(s, TestString, HE_LENGTH_OF(TestString) - 1);

    HE_EXPECT_EQ(s[0], 'H');
    HE_EXPECT_EQ(s[5], ',');
    HE_EXPECT_EQ(s[9], 'r');
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_view, operator_eq)
{
    const StringView a("Hello, world!");
    const StringView b("Hello, world!");
    const StringView c("Goodbye, world!");

    HE_EXPECT((a == a), a, a);
    HE_EXPECT((a == b), a, b);
    HE_EXPECT(!(a == c), a, c);

    HE_EXPECT((b == a), b, a);
    HE_EXPECT((b == b), b, b);
    HE_EXPECT(!(b == c), b, c);

    HE_EXPECT(!(c == a), c, a);
    HE_EXPECT(!(c == b), c, b);
    HE_EXPECT((c == c), c, c);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_view, operator_ne)
{
    const StringView a("Hello, world!");
    const StringView b("Hello, world!");
    const StringView c("Goodbye, world!");

    HE_EXPECT(!(a != a), a, a);
    HE_EXPECT(!(a != b), a, b);
    HE_EXPECT((a != c), a, c);

    HE_EXPECT(!(b != a), b, a);
    HE_EXPECT(!(b != b), b, b);
    HE_EXPECT((b != c), b, c);

    HE_EXPECT((c != a), c, a);
    HE_EXPECT((c != b), c, b);
    HE_EXPECT(!(c != c), c, c);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_view, operator_lt)
{
    const StringView a("Hello, world!");
    const StringView b("Hello, world!");
    const StringView c("Goodbye, world!");

    HE_EXPECT(!(a < a), a, a);
    HE_EXPECT(!(a < b), a, b);
    HE_EXPECT(!(a < c), a, c);

    HE_EXPECT(!(b < a), b, a);
    HE_EXPECT(!(b < b), b, b);
    HE_EXPECT(!(b < c), b, c);

    HE_EXPECT((c < a), c, a);
    HE_EXPECT((c < b), c, b);
    HE_EXPECT(!(c < c), c, c);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_view, operator_le)
{
    const StringView a("Hello, world!");
    const StringView b("Hello, world!");
    const StringView c("Goodbye, world!");

    HE_EXPECT((a <= a), a, a);
    HE_EXPECT((a <= b), a, b);
    HE_EXPECT(!(a <= c), a, c);

    HE_EXPECT((b <= a), b, a);
    HE_EXPECT((b <= b), b, b);
    HE_EXPECT(!(b <= c), b, c);

    HE_EXPECT((c <= a), c, a);
    HE_EXPECT((c <= b), c, b);
    HE_EXPECT((c <= c), c, c);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_view, operator_gt)
{
    const StringView a("Hello, world!");
    const StringView b("Hello, world!");
    const StringView c("Goodbye, world!");

    HE_EXPECT(!(a > a), a, a);
    HE_EXPECT(!(a > b), a, b);
    HE_EXPECT((a > c), a, c);

    HE_EXPECT(!(b > a), b, a);
    HE_EXPECT(!(b > b), b, b);
    HE_EXPECT((b > c), b, c);

    HE_EXPECT(!(c > a), c, a);
    HE_EXPECT(!(c > b), c, b);
    HE_EXPECT(!(c > c), c, c);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_view, operator_ge)
{
    const StringView a("Hello, world!");
    const StringView b("Hello, world!");
    const StringView c("Goodbye, world!");

    HE_EXPECT((a >= a), a, a);
    HE_EXPECT((a >= b), a, b);
    HE_EXPECT((a >= c), a, c);

    HE_EXPECT((b >= a), b, a);
    HE_EXPECT((b >= b), b, b);
    HE_EXPECT((b >= c), b, c);

    HE_EXPECT(!(c >= a), c, a);
    HE_EXPECT(!(c >= b), c, b);
    HE_EXPECT((c >= c), c, c);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_view, IsEmpty)
{
    {
        StringView s;
        StringViewTestAttorney::Test(s, nullptr, 0);

        HE_EXPECT(s.IsEmpty());
    }

    {
        constexpr char TestString[] = "Hello, world!";

        StringView s(TestString);
        StringViewTestAttorney::Test(s, TestString, HE_LENGTH_OF(TestString) - 1);

        HE_EXPECT(!s.IsEmpty());
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_view, Size)
{
    {
        StringView s;
        StringViewTestAttorney::Test(s, nullptr, 0);

        HE_EXPECT_EQ(s.Size(), 0);
    }

    {
        constexpr char TestString[] = "Hello, world!";

        StringView s(TestString);
        StringViewTestAttorney::Test(s, TestString, HE_LENGTH_OF(TestString) - 1);

        HE_EXPECT_EQ(s.Size(), HE_LENGTH_OF(TestString) - 1);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_view, Data)
{
    {
        StringView s;
        StringViewTestAttorney::Test(s, nullptr, 0);

        HE_EXPECT(s.Data() == nullptr);
    }

    {
        constexpr char TestString[] = "Hello, world!";

        StringView s(TestString);
        StringViewTestAttorney::Test(s, TestString, HE_LENGTH_OF(TestString) - 1);

        HE_EXPECT(s.Data() == TestString);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_view, Front)
{
    constexpr char TestString[] = "Hello, world!";

    StringView s(TestString);
    StringViewTestAttorney::Test(s, TestString, HE_LENGTH_OF(TestString) - 1);

    HE_EXPECT_EQ(s.Front(), 'H');
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_view, Back)
{
    constexpr char TestString[] = "Hello, world!";

    StringView s(TestString);
    StringViewTestAttorney::Test(s, TestString, HE_LENGTH_OF(TestString) - 1);

    HE_EXPECT_EQ(s.Back(), '!');
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_view, ToInteger)
{
    // Decimal
    HE_EXPECT_EQ(StringView("1").ToInteger<int32_t>(), 1);
    HE_EXPECT_EQ(StringView("-1").ToInteger<int32_t>(), -1);
    HE_EXPECT_EQ(StringView("1").ToInteger<uint32_t>(), 1);
    HE_EXPECT_EQ(StringView("-1").ToInteger<uint32_t>(), 0xffffffff);

    // Hex
    HE_EXPECT_EQ(StringView("abc").ToInteger<uint32_t>(16), 0xabc);
    HE_EXPECT_EQ(StringView("0xffffffff").ToInteger<uint32_t>(16), 0xffffffff);

    // Binary
    HE_EXPECT_EQ(StringView("0101001").ToInteger<uint32_t>(2), 0b0101001);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_view, ToFloat)
{
    HE_EXPECT_EQ(StringView("3.141592654").ToFloat(), 3.141592654f);
    HE_EXPECT_EQ(StringView("3.141592653589793238462643383279502884").ToFloat<double>(), 3.141592653589793238462643383279502884);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_view, CompareTo)
{
    const StringView a("Hello, world!");
    const StringView b("Hello, world!");
    const StringView c("Goodbye, world!");

    HE_EXPECT_EQ(a.CompareTo(a), 0);
    HE_EXPECT_EQ(a.CompareTo(b), 0);
    HE_EXPECT_GT(a.CompareTo(c), 0);

    HE_EXPECT_EQ(b.CompareTo(a), 0);
    HE_EXPECT_EQ(b.CompareTo(b), 0);
    HE_EXPECT_GT(b.CompareTo(c), 0);

    HE_EXPECT_LT(c.CompareTo(a), 0);
    HE_EXPECT_LT(c.CompareTo(b), 0);
    HE_EXPECT_EQ(c.CompareTo(c), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_view, Begin)
{
    {
        StringView s;
        StringViewTestAttorney::Test(s, nullptr, 0);

        HE_EXPECT(s.Begin() == nullptr);
    }

    {
        constexpr char TestString[] = "Hello, world!";

        StringView s(TestString);
        StringViewTestAttorney::Test(s, TestString, HE_LENGTH_OF(TestString) - 1);

        HE_EXPECT(s.Begin() == TestString);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_view, End)
{
    {
        StringView s;
        StringViewTestAttorney::Test(s, nullptr, 0);

        HE_EXPECT(s.End() == nullptr);
    }

    {
        constexpr char TestString[] = "Hello, world!";

        StringView s(TestString);
        StringViewTestAttorney::Test(s, TestString, HE_LENGTH_OF(TestString) - 1);

        HE_EXPECT(s.End() == (TestString + HE_LENGTH_OF(TestString) - 1));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_view, RangeBasedFor)
{
    constexpr char TestString[] = "Hello, world!";

    StringView s(TestString);

    uint32_t i = 0;
    for (char c : s)
    {
        HE_EXPECT_EQ(c, TestString[i++]);
    }
    HE_EXPECT_EQ(i, HE_LENGTH_OF(TestString) - 1);
}
