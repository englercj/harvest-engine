// Copyright Chad Engler

#include "he/core/enum_bitset.h"

#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, enum_bitset, Test)
{
    enum class Test { None = 0, A, B, C, D };

    EnumBitset<Test> test;

    HE_EXPECT_EQ(test.Value(), 0);
    test.Set(Test::A);
    HE_EXPECT(test.IsSet(Test::A));
    HE_EXPECT(!test.IsSet(Test::B));

    test.Set(Test::B, Test::C);
    HE_EXPECT(!test.IsSet(Test::D));
    HE_EXPECT(test.AreAllSet(Test::A, Test::B, Test::C));
    HE_EXPECT(!test.AreAllSet(Test::A, Test::B, Test::C, Test::D));
    HE_EXPECT(test.AreAnySet(Test::A, Test::D));

    test.Unset(Test::B);
    HE_EXPECT(!test.IsSet(Test::D));
    HE_EXPECT(!test.AreAllSet(Test::A, Test::B, Test::C));
    HE_EXPECT(!test.AreAllSet(Test::A, Test::B, Test::C, Test::D));
    HE_EXPECT(test.AreAllSet(Test::A, Test::C));
    HE_EXPECT(test.AreAnySet(Test::A, Test::D));

    test.Clear();
    HE_EXPECT_EQ(test.Value(), 0);
    HE_EXPECT(!test.IsSet(Test::A));
}
