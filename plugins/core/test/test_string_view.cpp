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
HE_TEST(core, StringView, Construct)
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
        String str(CrtAllocator::Get(), TestString);

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
        static const char TestStringWithNull[] = "Hello\0world!";
        StringView s(TestStringWithNull);
        StringViewTestAttorney::Test(s, TestStringWithNull, 5);
        HE_EXPECT_EQ_STR(s.Data(), "Hello");
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, StringView, operator_index)
{
    static const char TestString[] = "Hello, world!";

    StringView s(TestString);
    StringViewTestAttorney::Test(s, TestString, HE_LENGTH_OF(TestString) - 1);

    HE_EXPECT_EQ(s[0], 'H');
    HE_EXPECT_EQ(s[5], ',');
    HE_EXPECT_EQ(s[9], 'r');
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, StringView, operator_eq)
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
HE_TEST(core, StringView, operator_ne)
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
HE_TEST(core, StringView, operator_lt)
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
HE_TEST(core, StringView, operator_le)
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
HE_TEST(core, StringView, operator_gt)
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
HE_TEST(core, StringView, operator_ge)
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
HE_TEST(core, StringView, IsEmpty)
{
    {
        StringView s;
        StringViewTestAttorney::Test(s, nullptr, 0);

        HE_EXPECT(s.IsEmpty());
    }

    {
        static const char TestString[] = "Hello, world!";

        StringView s(TestString);
        StringViewTestAttorney::Test(s, TestString, HE_LENGTH_OF(TestString) - 1);

        HE_EXPECT(!s.IsEmpty());
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, StringView, Size)
{
    {
        StringView s;
        StringViewTestAttorney::Test(s, nullptr, 0);

        HE_EXPECT_EQ(s.Size(), 0);
    }

    {
        static const char TestString[] = "Hello, world!";

        StringView s(TestString);
        StringViewTestAttorney::Test(s, TestString, HE_LENGTH_OF(TestString) - 1);

        HE_EXPECT_EQ(s.Size(), HE_LENGTH_OF(TestString) - 1);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, StringView, Data)
{
    {
        StringView s;
        StringViewTestAttorney::Test(s, nullptr, 0);

        HE_EXPECT(s.Data() == nullptr);
    }

    {
        static const char TestString[] = "Hello, world!";

        StringView s(TestString);
        StringViewTestAttorney::Test(s, TestString, HE_LENGTH_OF(TestString) - 1);

        HE_EXPECT(s.Data() == TestString);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, StringView, Front)
{
    static const char TestString[] = "Hello, world!";

    StringView s(TestString);
    StringViewTestAttorney::Test(s, TestString, HE_LENGTH_OF(TestString) - 1);

    HE_EXPECT_EQ(s.Front(), 'H');
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, StringView, Back)
{
    static const char TestString[] = "Hello, world!";

    StringView s(TestString);
    StringViewTestAttorney::Test(s, TestString, HE_LENGTH_OF(TestString) - 1);

    HE_EXPECT_EQ(s.Back(), '!');
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, StringView, ToInteger)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, StringView, ToFloat)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, StringView, CompareTo)
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
HE_TEST(core, StringView, Begin)
{
    {
        StringView s;
        StringViewTestAttorney::Test(s, nullptr, 0);

        HE_EXPECT(s.Begin() == nullptr);
    }

    {
        static const char TestString[] = "Hello, world!";

        StringView s(TestString);
        StringViewTestAttorney::Test(s, TestString, HE_LENGTH_OF(TestString) - 1);

        HE_EXPECT(s.Begin() == TestString);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, StringView, End)
{
    {
        StringView s;
        StringViewTestAttorney::Test(s, nullptr, 0);

        HE_EXPECT(s.End() == nullptr);
    }

    {
        static const char TestString[] = "Hello, world!";

        StringView s(TestString);
        StringViewTestAttorney::Test(s, TestString, HE_LENGTH_OF(TestString) - 1);

        HE_EXPECT(s.End() == (TestString + HE_LENGTH_OF(TestString) - 1));
    }
}
