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
enum class TestStrEnum : uint32_t
{
    A,
    B,
    C,
};

template <>
const char* he::AsString(TestStrEnum x)
{
    switch (x)
    {
        case TestStrEnum::A: return "A";
        case TestStrEnum::B: return "B";
        case TestStrEnum::C: return "C";
    }

    return "<unknown>";
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, enum_ops, Flags)
{
    HE_EXPECT((~Flags::A) == Flags(~1u));

    HE_EXPECT((Flags::A | Flags::A) == Flags(1));
    HE_EXPECT((Flags::A | Flags::B) == Flags(3));
    HE_EXPECT((Flags::B | Flags::C) == Flags(6));

    HE_EXPECT((Flags::A & Flags::A) == Flags(1));
    HE_EXPECT((Flags::A & Flags::B) == Flags(0));
    HE_EXPECT((Flags::B & Flags::C) == Flags(0));

    HE_EXPECT((Flags::A ^ Flags::A) == Flags(0));
    HE_EXPECT((Flags::A ^ Flags::B) == Flags(3));
    HE_EXPECT((Flags::B ^ Flags::C) == Flags(6));

    Flags f;

    f = Flags::None;
    HE_EXPECT((f |= Flags::A) == Flags(1));
    HE_EXPECT((f |= Flags::B) == Flags(3));
    HE_EXPECT((f |= Flags::C) == Flags(7));

    f = Flags::None;
    HE_EXPECT((f &= Flags::A) == Flags(0));
    HE_EXPECT((f &= Flags::B) == Flags(0));
    HE_EXPECT((f &= Flags::C) == Flags(0));

    f = Flags::A;
    HE_EXPECT((f &= Flags::A) == Flags(1));
    HE_EXPECT((f &= Flags::B) == Flags(0));
    HE_EXPECT((f &= Flags::C) == Flags(0));

    f = Flags::None;
    HE_EXPECT((f ^= Flags::A) == Flags(1));
    HE_EXPECT((f ^= Flags::B) == Flags(3));
    HE_EXPECT((f ^= Flags::C) == Flags(7));
    HE_EXPECT((f ^= Flags::A) == Flags(6));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, enum_ops, AsString)
{
    HE_EXPECT_EQ_STR(AsString(TestStrEnum::A), "A");
    HE_EXPECT_EQ_STR(AsString(TestStrEnum::B), "B");
    HE_EXPECT_EQ_STR(AsString(TestStrEnum::C), "C");
    HE_EXPECT_EQ_STR(AsString(static_cast<TestStrEnum>(100)), "<unknown>");
}
