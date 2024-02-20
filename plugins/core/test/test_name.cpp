// Copyright Chad Engler

#include "he/core/name.h"
#include "he/core/name_fmt.h"

#include "he/core/string_pool_fmt.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, name, Construct)
{
    {
        Name n;
        HE_EXPECT(!n);
        HE_EXPECT_EQ_PTR(n.String(), nullptr);
        HE_EXPECT_EQ(n.Id(), StringPoolId{});
    }

    {
        Name n("foo");
        HE_EXPECT(n);
        HE_EXPECT_EQ_STR(n.String(), "foo");
        HE_EXPECT_NE(n.Id(), StringPoolId{});
    }

    {
        Name n(StringView("foo"));
        HE_EXPECT(n);
        HE_EXPECT_EQ_STR(n.String(), "foo");
        HE_EXPECT_NE(n.Id(), StringPoolId{});
    }

    {
        const StringPoolId id = StringPool::GetDefault().Add("foo");
        Name n(id);
        HE_EXPECT(n);
        HE_EXPECT_EQ_STR(n.String(), "foo");
        HE_EXPECT_EQ(n.Id(), id);
    }

    {
        Name n({ 0 });
        HE_EXPECT(!n);
        HE_EXPECT_EQ_PTR(n.String(), nullptr);
        HE_EXPECT_EQ(n.Id(), StringPoolId{});
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, name, String)
{
    {
        Name n;
        HE_EXPECT_EQ_PTR(n.String(), nullptr);
    }

    {
        Name n("foo");
        HE_EXPECT_EQ_STR(n.String(), "foo");
    }

    {
        Name n(StringView("foo"));
        HE_EXPECT_EQ_STR(n.String(), "foo");
    }

    {
        const StringPoolId id = StringPool::GetDefault().Add("foo");
        Name n(id);
        HE_EXPECT_EQ_STR(n.String(), "foo");
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, name, Id)
{
    {
        Name n;
        HE_EXPECT_EQ(n.Id(), StringPoolId{});
    }

    {
        Name n("foo");
        HE_EXPECT_NE(n.Id(), StringPoolId{});
    }

    {
        Name n(StringView("foo"));
        HE_EXPECT_NE(n.Id(), StringPoolId{});
    }

    {
        const StringPoolId id = StringPool::GetDefault().Add("foo");
        Name n(id);
        HE_EXPECT_EQ(n.Id(), id);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, name, operator_assign)
{
    Name name;
    HE_EXPECT(!name);

    name = "foo";
    HE_EXPECT(name);
    HE_EXPECT_EQ_STR(name.String(), "foo");

    name = StringView("bar");
    HE_EXPECT(name);
    HE_EXPECT_EQ_STR(name.String(), "bar");

    name = Name("foobar");
    HE_EXPECT(name);
    HE_EXPECT_EQ_STR(name.String(), "foobar");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, name, operator_compare)
{
    {
        Name n1;
        Name n2;
        HE_EXPECT(n1 == n2);
        HE_EXPECT(!(n1 != n2));
        HE_EXPECT(!(n1 < n2));
    }

    {
        Name n1("foo");
        Name n2("foo");
        HE_EXPECT(n1 == n2);
        HE_EXPECT(!(n1 != n2));
        HE_EXPECT(!(n1 < n2));
    }

    {
        Name n1("foo");
        Name n2("bar");
        HE_EXPECT(!(n1 == n2));
        HE_EXPECT(n1 != n2);
        HE_EXPECT(n1 < n2);
    }

    {
        Name n1("foo");
        Name n2("foobar");
        HE_EXPECT(!(n1 == n2));
        HE_EXPECT(n1 != n2);
        HE_EXPECT(n1 < n2);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, name, fmt)
{
    constexpr char TestString[] = "Hello, world!";

    const Name name(TestString);

    const String str = Format("{}", name);
    HE_EXPECT_EQ(str, TestString);
}
