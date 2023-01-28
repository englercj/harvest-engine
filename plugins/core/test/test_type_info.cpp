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
        constexpr StringView name = _GetTypeName<int>();

        static_assert(name == "int");
        HE_EXPECT_EQ(name, "int");
    }

    {
        constexpr StringView name = _GetTypeName<uint32_t>();

        static_assert(name == "unsigned int");
        HE_EXPECT_EQ(name, "unsigned int");
    }

    {
        constexpr StringView name = _GetTypeName<TestFixture>();

        static_assert(name == "class he::TestFixture");
        HE_EXPECT_EQ(name, "class he::TestFixture");
    }

    {
        struct Test {};

        constexpr StringView name = _GetTypeName<Test>();

        static_assert(name == "struct _heTestClass_core_type_info__GetTypeName::TestBody::Test");
        HE_EXPECT_EQ(name, "struct _heTestClass_core_type_info__GetTypeName::TestBody::Test");
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
        constexpr TypeInfo info = TypeInfo::Get<int>();

        static_assert(info.Name() == "int");
        HE_EXPECT_EQ(info.Name(), "int");

        static_assert(info.Hash() == 0x2b9fff192bd4c83e);
        HE_EXPECT_EQ(info.Hash(), 0x2b9fff192bd4c83e);
    }

    {
        constexpr TypeInfo info = TypeInfo::Get<uint32_t>();

        static_assert(info.Name() == "unsigned int");
        HE_EXPECT_EQ(info.Name(), "unsigned int");

        static_assert(info.Hash() == 0xbaaedcff023465cb);
        HE_EXPECT_EQ(info.Hash(), 0xbaaedcff023465cb);
    }

    {
        constexpr TypeInfo info = TypeInfo::Get<TestFixture>();

        static_assert(info.Name() == "class he::TestFixture");
        HE_EXPECT_EQ(info.Name(), "class he::TestFixture");

        static_assert(info.Hash() == 0x84cc2492ffe3cb3f);
        HE_EXPECT_EQ(info.Hash(), 0x84cc2492ffe3cb3f);
    }

    {
        struct Test {};

        constexpr TypeInfo info = TypeInfo::Get<Test>();

        static_assert(info.Name() == "struct _heTestClass_core_type_info_Name_Hash::TestBody::Test");
        HE_EXPECT_EQ(info.Name(), "struct _heTestClass_core_type_info_Name_Hash::TestBody::Test");

        static_assert(info.Hash() == 0x8d098f6678e7c781);
        HE_EXPECT_EQ(info.Hash(), 0x8d098f6678e7c781);
    }
}
