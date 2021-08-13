// Copyright Chad Engler

#include "he/core/enum_ops.h"

#include "he/core/test.h"

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
HE_TEST(core, enum_ops, Flags)
{
    HE_EXPECT_EQ(~Flags::A, Flags(~1u));

    HE_EXPECT_EQ(Flags::A | Flags::A, Flags(1));
    HE_EXPECT_EQ(Flags::A | Flags::B, Flags(3));
    HE_EXPECT_EQ(Flags::B | Flags::C, Flags(6));

    HE_EXPECT_EQ(Flags::A & Flags::A, Flags(1));
    HE_EXPECT_EQ(Flags::A & Flags::B, Flags(0));
    HE_EXPECT_EQ(Flags::B & Flags::C, Flags(0));

    HE_EXPECT_EQ(Flags::A ^ Flags::A, Flags(0));
    HE_EXPECT_EQ(Flags::A ^ Flags::B, Flags(3));
    HE_EXPECT_EQ(Flags::B ^ Flags::C, Flags(6));

    Flags f;

    f = Flags::None;
    HE_EXPECT_EQ(f |= Flags::A, Flags(1));
    HE_EXPECT_EQ(f |= Flags::B, Flags(3));
    HE_EXPECT_EQ(f |= Flags::C, Flags(7));

    f = Flags::None;
    HE_EXPECT_EQ(f &= Flags::A, Flags(0));
    HE_EXPECT_EQ(f &= Flags::B, Flags(0));
    HE_EXPECT_EQ(f &= Flags::C, Flags(0));

    f = Flags::A;
    HE_EXPECT_EQ(f &= Flags::A, Flags(1));
    HE_EXPECT_EQ(f &= Flags::B, Flags(0));
    HE_EXPECT_EQ(f &= Flags::C, Flags(0));

    f = Flags::None;
    HE_EXPECT_EQ(f ^= Flags::A, Flags(1));
    HE_EXPECT_EQ(f ^= Flags::B, Flags(3));
    HE_EXPECT_EQ(f ^= Flags::C, Flags(7));
    HE_EXPECT_EQ(f ^= Flags::A, Flags(6));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, enum_ops, HasFlag)
{
    HE_EXPECT(HasFlag(Flags::A | Flags::B, Flags::A));
    HE_EXPECT(HasFlag(Flags::A | Flags::B, Flags::B));
    HE_EXPECT(!HasFlag(Flags::A | Flags::B, Flags::C));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, enum_ops, HasFlags)
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
HE_TEST(core, enum_ops, HasAnyFlags)
{
    HE_EXPECT(HasAnyFlags(Flags::A | Flags::B, Flags::A));
    HE_EXPECT(HasAnyFlags(Flags::A | Flags::B, Flags::B));
    HE_EXPECT(!HasAnyFlags(Flags::A | Flags::B, Flags::C));

    HE_EXPECT(HasAnyFlags(Flags::A | Flags::B, Flags::A | Flags::B));
    HE_EXPECT(HasAnyFlags(Flags::A | Flags::B, Flags::B | Flags::A));
    HE_EXPECT(HasAnyFlags(Flags::A | Flags::B, Flags::A | Flags::C));
    HE_EXPECT(!HasAnyFlags(Flags::A | Flags::B, Flags::C));
}
