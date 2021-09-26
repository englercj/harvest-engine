// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/utils.h"

#include "he/core/alloca.h"
#include "he/core/macros.h"
#include "he/core/test.h"

#include <type_traits>

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, utils, IsAligned)
{
    static_assert(IsAligned(0, 8));
    static_assert(IsAligned(8, 8));
    static_assert(IsAligned(16, 8));
    static_assert(IsAligned(480, 8));
    static_assert(!IsAligned(9, 8));
    static_assert(!IsAligned(17, 8));
    static_assert(!IsAligned(479, 8));

    HE_EXPECT(IsAligned(0, 8));
    HE_EXPECT(IsAligned(8, 8));
    HE_EXPECT(IsAligned(16, 8));
    HE_EXPECT(IsAligned(480, 8));
    HE_EXPECT(!IsAligned(9, 8));
    HE_EXPECT(!IsAligned(17, 8));
    HE_EXPECT(!IsAligned(479, 8));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, utils, IsAligned_pointer)
{
    char* a = HE_ALLOCA(char, 22);
    HE_EXPECT(IsAligned(a, 8));

    NonTrivial* nt = new NonTrivial();
    HE_EXPECT(IsAligned(nt, 8));

    delete nt;
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, utils, Min)
{
    static_assert(Min(1, 2) == 1, "");
    static_assert(Min(1234567.678f, 7654321.876f) == 1234567.678f, "");

    HE_EXPECT_EQ(Min(1, 2), 1);
    HE_EXPECT_EQ(Min(1234567.678f, 7654321.876f), 1234567.678f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, utils, Max)
{
    static_assert(Max(1, 2) == 2, "");
    static_assert(Max(1234567.678f, 7654321.876f) == 7654321.876f, "");

    HE_EXPECT_EQ(Max(1, 2), 2);
    HE_EXPECT_EQ(Max(1234567.678f, 7654321.876f), 7654321.876f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, utils, Clamp)
{
    static_assert(Clamp(1, 0, 2) == 1, "");
    static_assert(Clamp(0.8564563872f, 0.0f, 1.0f) == 0.8564563872f, "");
    static_assert(Clamp(-1.0, 0.0, 2.0) == 0.0, "");
    static_assert(Clamp(10.5, 0.0, 2.0) == 2.0, "");

    HE_EXPECT_EQ(Clamp(1, 0, 2), 1);
    HE_EXPECT_EQ(Clamp(0.8564563872f, 0.0f, 1.0f), 0.8564563872f);
    HE_EXPECT_EQ(Clamp(-1.0, 0.0, 2.0), 0.0);
    HE_EXPECT_EQ(Clamp(10.5, 0.0, 2.0), 2.0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, utils, Abs)
{
    static_assert(Abs(1) == 1, "");
    static_assert(Abs(-1) == 1, "");

    HE_EXPECT_EQ(Abs(1), 1);
    HE_EXPECT_EQ(Abs(-1), 1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, utils, BitCast)
{
    static_assert(BitCast<uint32_t>(0x3f800000u) == 0x3f800000);
    static_assert(BitCast<uint32_t>(1.0f) == 0x3f800000);
    static_assert(BitCast<uint64_t>(-1ll) == 0xffffffffffffffff);

    HE_EXPECT_EQ(BitCast<uint32_t>(0x3f800000u), 0x3f800000);
    HE_EXPECT_EQ(BitCast<uint32_t>(1.0f), 0x3f800000);
    HE_EXPECT_EQ(BitCast<uint64_t>(-1ll), 0xffffffffffffffff);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, utils, Forward)
{
    int x;
    HE_UNUSED(x);

    static_assert(std::is_same_v<decltype(Forward<int>(x)), int&&>);
    static_assert(std::is_same_v<decltype(Forward<const int&>(x)), const int&>);
    static_assert(std::is_same_v<decltype(Forward<int&>(x)), int&>);
    static_assert(std::is_same_v<decltype(Forward<int&&>(x)), int&&>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, utils, Move)
{
    int x;
    int& rx(x);
    HE_UNUSED(rx);

    static_assert(std::is_same_v<decltype(Move(x)), int&&>);
    static_assert(std::is_same_v<decltype(Move(rx)), int&&>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, utils, Exchange)
{
    int a = 1;
    int b = Exchange(a, 2);
    HE_EXPECT_EQ(a, 2);
    HE_EXPECT_EQ(b, 1);

    struct TestMove
    {
        bool copyCtor{ false };
        bool copyAssign{ false };

        bool moveCtor{ false };
        bool moveAssign{ false };

        TestMove() = default;

        TestMove(const TestMove&) { copyCtor = true; }
        TestMove& operator=(const TestMove&) { copyAssign = true; return *this; }

        TestMove(TestMove&&) { moveCtor = true; }
        TestMove& operator=(TestMove&&) { moveAssign = true; return *this; }
    };

    TestMove x{};
    TestMove y{};
    HE_EXPECT(!x.copyCtor);
    HE_EXPECT(!x.copyAssign);
    HE_EXPECT(!x.moveCtor);
    HE_EXPECT(!x.moveAssign);
    HE_EXPECT(!y.copyCtor);
    HE_EXPECT(!y.copyAssign);
    HE_EXPECT(!y.moveCtor);
    HE_EXPECT(!y.moveAssign);

    y = Exchange(x, {});
    HE_EXPECT(!x.copyCtor);
    HE_EXPECT(!x.copyAssign);
    HE_EXPECT(!x.moveCtor);
    HE_EXPECT(x.moveAssign);
    HE_EXPECT(!y.copyCtor);
    HE_EXPECT(!y.copyAssign);
    HE_EXPECT(!y.moveCtor);
    HE_EXPECT(y.moveAssign);
}
