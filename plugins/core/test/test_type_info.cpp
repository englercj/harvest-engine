// Copyright Chad Engler

#include "he/core/type_info.h"

#include "he/core/test.h"
#include "he/core/type_traits.h"
#include "he/core/type_info_fmt.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_info, _GetTypeName)
{
    {
        constexpr StringView Name = _GetTypeName<int>();
        constexpr StringView Expected = "int";

        static_assert(Name == Expected);
        HE_EXPECT_EQ(Name, Expected);
    }

    {
        constexpr StringView Name = _GetTypeName<uint32_t>();
        constexpr StringView Expected = "unsigned int";

        static_assert(Name == Expected);
        HE_EXPECT_EQ(Name, Expected);
    }

    {
        constexpr StringView Name = _GetTypeName<TestFixture>();
        constexpr StringView Expected = "class he::TestFixture";

        static_assert(Name == Expected);
        HE_EXPECT_EQ(Name, Expected);
    }

    {
        struct Test {};

        constexpr StringView Name = _GetTypeName<Test>();
        constexpr StringView Expected = "struct _heTestClass_core_type_info__GetTypeName::TestBody::Test";

        static_assert(Name == Expected);
        HE_EXPECT_EQ(Name, Expected);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_info, TypeInfo)
{
    static_assert(IsCopyConstructible<TypeInfo>);
    static_assert(IsMoveConstructible<TypeInfo>);
    static_assert(IsCopyAssignable<TypeInfo>);
    static_assert(IsMoveAssignable<TypeInfo>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_info, Operators)
{
    HE_EXPECT_EQ(TypeInfo::Get<int>(), TypeInfo::Get<int>());
    HE_EXPECT_EQ(TypeInfo::Get<int>(), TypeInfo::Get<const int>());
    HE_EXPECT_EQ(TypeInfo::Get<int>(), TypeInfo::Get<int&>());
    HE_EXPECT_EQ(TypeInfo::Get<int>(), TypeInfo::Get<const int&>());
    HE_EXPECT_EQ(TypeInfo::Get<int>(), TypeInfo::Get<int&&>());
    HE_EXPECT_EQ(TypeInfo::Get<int>(), TypeInfo::Get<const int&&>());

    HE_EXPECT_NE(TypeInfo::Get<int>(), TypeInfo::Get<char>());
    HE_EXPECT_LT(TypeInfo::Get<int>(), TypeInfo::Get<char>());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_info, Name_Hash)
{
    {
        constexpr TypeInfo Info = TypeInfo::Get<int>();
        constexpr StringView ExpectedName = "int";

        static_assert(Info.Name() == ExpectedName);
        HE_EXPECT_EQ(Info.Name(), ExpectedName);

        static_assert(Info.Hash() == 0x95e97e5e);
        HE_EXPECT_EQ(Info.Hash(), 0x95e97e5e);
    }

    {
        constexpr TypeInfo Info = TypeInfo::Get<uint32_t>();
        constexpr StringView ExpectedName = "unsigned int";

        static_assert(Info.Name() == ExpectedName);
        HE_EXPECT_EQ(Info.Name(), ExpectedName);

        static_assert(Info.Hash() == 0xf0981eeb);
        HE_EXPECT_EQ(Info.Hash(), 0xf0981eeb);
    }

    {
        constexpr TypeInfo Info = TypeInfo::Get<TestFixture>();
        constexpr StringView ExpectedName = "class he::TestFixture";

        static_assert(Info.Name() == ExpectedName);
        HE_EXPECT_EQ(Info.Name(), ExpectedName);

        static_assert(Info.Hash() == 0xf7116a3f);
        HE_EXPECT_EQ(Info.Hash(), 0xf7116a3f);
    }

    {
        struct Test {};

        constexpr TypeInfo Info = TypeInfo::Get<Test>();
        constexpr StringView ExpectedName = "struct _heTestClass_core_type_info_Name_Hash::TestBody::Test";

        static_assert(Info.Name() == ExpectedName);
        HE_EXPECT_EQ(Info.Name(), ExpectedName);

        static_assert(Info.Hash() == 0xc818af81);
        HE_EXPECT_EQ(Info.Hash(), 0xc818af81);
    }
}
