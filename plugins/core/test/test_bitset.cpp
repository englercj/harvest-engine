// Copyright Chad Engler

#include "he/core/bitset.h"

#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, Construct)
{
    {
        BitSet<5> v;
        HE_EXPECT(!v.IsSet(2));
        HE_EXPECT_EQ(v.BitCount, 5);
    }

    {
        BitSet<5> v;
        HE_EXPECT(!v.IsSet(2));
        HE_EXPECT_EQ(v.BitCount, 5);

        v.Set(2);
        HE_EXPECT(v.IsSet(2));

        BitSet<5> copy = v;
        HE_EXPECT(copy.IsSet(2));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, operator_assign_copy)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, operator_index)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, operator_eq)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, operator_ne)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, Size)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, IsSet)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, HashCode)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, Begin_End)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, RangeBasedFor)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, Set)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, Unset)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, Flip)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, Clear)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, BitReference_Construct)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, BitReference_operator_assign)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, BitReference_operator_bitwise_not)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, BitReference_operator_bool)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, BitReference_Flip)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, BitSpan_Construct)
{
    BitSpan::ElementType a[] = { 1, 2, 3, 4, 5 };

    {
        BitSpan span;
        HE_EXPECT_EQ(span.Size(), 0);
    }

    {
        BitSpan span(a, 0);
        HE_EXPECT_EQ(span.Size(), 0);
    }

    {
        BitSpan span(a, HE_LENGTH_OF(a));
        HE_EXPECT_EQ(span.Size(), HE_LENGTH_OF(a) * BitSpan::BitsPerElement);
    }

    {
        BitSpan span(a);
        HE_EXPECT_EQ(span.Size(), HE_LENGTH_OF(a) * BitSpan::BitsPerElement);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, BitSpan_operator_index)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, BitSpan_operator_eq)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, BitSpan_operator_ne)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, BitSpan_Size)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, BitSpan_IsEmpty)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, BitSpan_IsSet)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, BitSpan_HashCode)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, BitSpan_Begin_End)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, BitSpan_RangeBasedFor)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, BitSpan_Set)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, BitSpan_Unset)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, BitSpan_Flip)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, BitSpan_Clear)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, bitset, EnumBitSet)
{
    enum class Test : uint8_t { None = 0, A, B, C, D };

    EnumBitSet<Test> test;

    HE_EXPECT_EQ(test.Value(), 0);
    test.Set(Test::A);
    HE_EXPECT(test.IsSet(Test::A));
    HE_EXPECT(!test.IsSet(Test::B));

    test.Set(Test::C);
    HE_EXPECT(!test.IsSet(Test::B));
    HE_EXPECT(!test.IsSet(Test::D));
    HE_EXPECT(test.AreAllSet(Test::A, Test::C));
    HE_EXPECT(!test.AreAllSet(Test::A, Test::B, Test::C));
    HE_EXPECT(!test.AreAllSet(Test::A, Test::B, Test::C, Test::D));
    HE_EXPECT(test.AreAnySet(Test::A, Test::D));

    test.Set(Test::B, Test::D);
    HE_EXPECT(test.AreAllSet(Test::A, Test::B, Test::C, Test::D));
    HE_EXPECT(test.AreAnySet(Test::A, Test::D));

    test.Unset(Test::B);
    HE_EXPECT(!test.IsSet(Test::B));
    HE_EXPECT(!test.AreAllSet(Test::A, Test::B, Test::C));
    HE_EXPECT(!test.AreAllSet(Test::A, Test::B, Test::C, Test::D));
    HE_EXPECT(test.AreAllSet(Test::A, Test::C));
    HE_EXPECT(test.AreAllSet(Test::A, Test::C, Test::D));
    HE_EXPECT(test.AreAnySet(Test::A, Test::B, Test::D));

    test.Clear();
    HE_EXPECT_EQ(test.Value(), 0);
    HE_EXPECT(!test.IsSet(Test::A));
}
