// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/utils.h"

#include "he/core/alloca.h"
#include "he/core/enum_ops.h"
#include "he/core/macros.h"
#include "he/core/test.h"
#include "he/core/type_traits.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
enum class Flags : uint32_t
{
    None = 0,
    A = 1 << 0,
    B = 1 << 1,
    C = 1 << 2,
};
HE_ENUM_FLAGS(Flags);

// ------------------------------------------------------------------------------------------------
HE_TEST(core, utils, IsAligned)
{
    static_assert(IsAligned(0u, 8));
    static_assert(IsAligned(8u, 8));
    static_assert(IsAligned(16u, 8));
    static_assert(IsAligned(480u, 8));
    static_assert(!IsAligned(9u, 8));
    static_assert(!IsAligned(17u, 8));
    static_assert(!IsAligned(479u, 8));

    HE_EXPECT(IsAligned(0u, 8));
    HE_EXPECT(IsAligned(8u, 8));
    HE_EXPECT(IsAligned(16u, 8));
    HE_EXPECT(IsAligned(480u, 8));
    HE_EXPECT(!IsAligned(9u, 8));
    HE_EXPECT(!IsAligned(17u, 8));
    HE_EXPECT(!IsAligned(479u, 8));
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
HE_TEST(core, utils, AlignDown)
{
    static_assert(AlignDown(0u, 8) == 0);
    static_assert(AlignDown(8u, 8) == 8);
    static_assert(AlignDown(16u, 8) == 16);
    static_assert(AlignDown(480u, 8) == 480);
    static_assert(AlignDown(9u, 8) == 8);
    static_assert(AlignDown(17u, 8) == 16);
    static_assert(AlignDown(479u, 8) == 472);

    HE_EXPECT_EQ(AlignDown(0u, 8), 0);
    HE_EXPECT_EQ(AlignDown(8u, 8), 8);
    HE_EXPECT_EQ(AlignDown(16u, 8), 16);
    HE_EXPECT_EQ(AlignDown(480u, 8), 480);
    HE_EXPECT_EQ(AlignDown(9u, 8), 8);
    HE_EXPECT_EQ(AlignDown(17u, 8), 16);
    HE_EXPECT_EQ(AlignDown(479u, 8), 472);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, utils, AlignDown_pointer)
{
    char* a = HE_ALLOCA(char, 22);
    char* a2 = AlignDown(a, 128);
    HE_EXPECT(IsAligned(a2, 128));

    NonTrivial* nt = new NonTrivial();
    NonTrivial* nt2 = AlignDown(nt, 128);
    HE_EXPECT(IsAligned(nt2, 128));

    delete nt;
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, utils, AlignUp)
{
    static_assert(AlignUp(0u, 8) == 0);
    static_assert(AlignUp(8u, 8) == 8);
    static_assert(AlignUp(16u, 8) == 16);
    static_assert(AlignUp(480u, 8) == 480);
    static_assert(AlignUp(9u, 8) == 16);
    static_assert(AlignUp(17u, 8) == 24);
    static_assert(AlignUp(479u, 8) == 480);

    HE_EXPECT_EQ(AlignUp(0u, 8), 0);
    HE_EXPECT_EQ(AlignUp(8u, 8), 8);
    HE_EXPECT_EQ(AlignUp(16u, 8), 16);
    HE_EXPECT_EQ(AlignUp(480u, 8), 480);
    HE_EXPECT_EQ(AlignUp(9u, 8), 16);
    HE_EXPECT_EQ(AlignUp(17u, 8), 24);
    HE_EXPECT_EQ(AlignUp(479u, 8), 480);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, utils, AlignUp_pointer)
{
    char* a = HE_ALLOCA(char, 22);
    char* a2 = AlignUp(a, 128);
    HE_EXPECT(IsAligned(a2, 128));

    NonTrivial* nt = new NonTrivial();
    NonTrivial* nt2 = AlignUp(nt, 128);
    HE_EXPECT(IsAligned(nt2, 128));

    delete nt;
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, utils, IsPowerOf2)
{
    static_assert(IsPowerOf2(1u));
    static_assert(IsPowerOf2(2u));
    static_assert(IsPowerOf2(4u));
    static_assert(IsPowerOf2(8u));
    static_assert(IsPowerOf2(16u));
    static_assert(IsPowerOf2(256u));
    static_assert(IsPowerOf2(65536u));
    static_assert(IsPowerOf2(536870912u));

    static_assert(!IsPowerOf2(0u));
    static_assert(!IsPowerOf2(3u));
    static_assert(!IsPowerOf2(7u));
    static_assert(!IsPowerOf2(65534u));
    static_assert(!IsPowerOf2(536870911u));
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
HE_TEST(core, utils, HasFlag)
{
    HE_EXPECT(HasFlag(Flags::A | Flags::B, Flags::A));
    HE_EXPECT(HasFlag(Flags::A | Flags::B, Flags::B));
    HE_EXPECT(!HasFlag(Flags::A | Flags::B, Flags::C));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, utils, HasFlags)
{
    HE_EXPECT(HasFlags(Flags::A | Flags::B, Flags::A));
    HE_EXPECT(HasFlags(Flags::A | Flags::B, Flags::B));
    HE_EXPECT(!HasFlags(Flags::A | Flags::B, Flags::C));

    HE_EXPECT(HasFlags(Flags::A | Flags::B, Flags::A | Flags::B));
    HE_EXPECT(HasFlags(Flags::A | Flags::B, Flags::B | Flags::A));
    HE_EXPECT(!HasFlags(Flags::A | Flags::B, Flags::A | Flags::C));
    HE_EXPECT(!HasFlags(Flags::A | Flags::B, Flags::C));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, utils, HasAnyFlags)
{
    HE_EXPECT(HasAnyFlags(Flags::A | Flags::B, Flags::A));
    HE_EXPECT(HasAnyFlags(Flags::A | Flags::B, Flags::B));
    HE_EXPECT(!HasAnyFlags(Flags::A | Flags::B, Flags::C));

    HE_EXPECT(HasAnyFlags(Flags::A | Flags::B, Flags::A | Flags::B));
    HE_EXPECT(HasAnyFlags(Flags::A | Flags::B, Flags::B | Flags::A));
    HE_EXPECT(HasAnyFlags(Flags::A | Flags::B, Flags::A | Flags::C));
    HE_EXPECT(!HasAnyFlags(Flags::A | Flags::B, Flags::C));
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

    static_assert(IsSame<decltype(Forward<int>(x)), int&&>);
    static_assert(IsSame<decltype(Forward<const int&>(x)), const int&>);
    static_assert(IsSame<decltype(Forward<int&>(x)), int&>);
    static_assert(IsSame<decltype(Forward<int&&>(x)), int&&>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, utils, Move)
{
    int x;
    int& rx(x);
    HE_UNUSED(rx);

    static_assert(IsSame<decltype(Move(x)), int&&>);
    static_assert(IsSame<decltype(Move(rx)), int&&>);
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

        TestMove(const TestMove&) noexcept { copyCtor = true; }
        TestMove& operator=(const TestMove&) noexcept { copyAssign = true; return *this; }

        TestMove(TestMove&&) noexcept { moveCtor = true; }
        TestMove& operator=(TestMove&&) noexcept { moveAssign = true; return *this; }
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
