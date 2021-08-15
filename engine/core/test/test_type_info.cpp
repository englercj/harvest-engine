// Copyright Chad Engler

#include "he/core/type_info.h"

#include "he/core/string_view_fmt.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_info, GetTypeName)
{
    {
        struct Test {};

        constexpr StringView name = GetTypeName<Test>();

        static_assert(name == StringView("struct _heTestClass_core_type_info_GetTypeName::TestBody::Test"));
        HE_EXPECT_EQ(name, StringView("struct _heTestClass_core_type_info_GetTypeName::TestBody::Test"));
    }

    {
        constexpr StringView name = GetTypeName<TestFixture>();

        static_assert(name == StringView("class he::TestFixture"));
        HE_EXPECT_EQ(name, StringView("class he::TestFixture"));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_info, GetTypeId)
{
    {
        struct Test {};

        constexpr TypeId id = GetTypeId<Test>();

        static_assert(id.name == StringView("struct _heTestClass_core_type_info_GetTypeId::TestBody::Test"));
        HE_EXPECT_EQ(id.name, StringView("struct _heTestClass_core_type_info_GetTypeId::TestBody::Test"));

        static_assert(id.hash == 0x19f2d23726fdcea4);
        HE_EXPECT_EQ(id.hash, 0x19f2d23726fdcea4);
    }

    {
        constexpr TypeId id = GetTypeId<TestFixture>();

        static_assert(id.name == StringView("class he::TestFixture"));
        HE_EXPECT_EQ(id.name, StringView("class he::TestFixture"));

        static_assert(id.hash == 0x84cc2492ffe3cb3f);
        HE_EXPECT_EQ(id.hash, 0x84cc2492ffe3cb3f);
    }
}
