// Copyright Chad Engler

#include "he/core/string_pool.h"
#include "he/core/string_pool_fmt.h"

#include "he/core/fmt.h"
#include "he/core/log.h"
#include "he/core/memory_ops.h"
#include "he/core/string.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_pool, StringPoolId)
{
    StringPoolId empty{};
    StringPoolId zero{ 0 };
    StringPoolId one{ 1 };
    StringPoolId two{ 2 };

    HE_EXPECT(!empty);
    HE_EXPECT(!zero);
    HE_EXPECT(one);
    HE_EXPECT(two);

    HE_EXPECT(one == one);
    HE_EXPECT(one != two);
    HE_EXPECT(one < two);
    HE_EXPECT(!(two == one));
    HE_EXPECT(!(two != two));
    HE_EXPECT(!(two < one));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_pool, Construct)
{
    {
        StringPool p;
        HE_EXPECT_EQ_PTR(&p.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT_EQ(p.PageSize(), StringPool::DefaultPageSize);
        HE_EXPECT_EQ(p.Size(), 0);
    }

    {
        CrtAllocator a;
        StringPool p(a);
        HE_EXPECT_EQ_PTR(&p.GetAllocator(), &a);
        HE_EXPECT_EQ(p.PageSize(), StringPool::DefaultPageSize);
        HE_EXPECT_EQ(p.Size(), 0);
    }

    {
        StringPool p(Allocator::GetDefault(), 1024);
        HE_EXPECT_EQ_PTR(&p.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT_EQ(p.PageSize(), 1024);
        HE_EXPECT_EQ(p.Size(), 0);
    }

    {
        StringPool& p = StringPool::GetDefault();
        HE_EXPECT_EQ_PTR(&p.GetAllocator(), &Allocator::GetDefault());
        HE_EXPECT_EQ(p.PageSize(), StringPool::DefaultPageSize);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_pool, basic_usage)
{
    constexpr const char* TestString = "Hello, world!";

    StringPool p;
    HE_EXPECT_EQ(p.Size(), 0);

    const StringPoolId id = p.Add(TestString);
    HE_EXPECT(id);
    HE_EXPECT_GT(id.val, 0);
    HE_EXPECT_EQ(p.Size(), 1);

    const StringPoolId id2 = p.Find(TestString);
    HE_EXPECT(id2);
    HE_EXPECT_GT(id2.val, 0);
    HE_EXPECT_EQ(id2, id);
    HE_EXPECT_EQ(p.Size(), 1);

    const StringPoolId id3 = p.Find("Nope");
    HE_EXPECT(!id3);
    HE_EXPECT_EQ(id3.val, 0);
    HE_EXPECT_EQ(p.Size(), 1);

    const StringPoolId id4 = p.Add(TestString);
    HE_EXPECT(id4);
    HE_EXPECT_GT(id4.val, 0);
    HE_EXPECT_EQ(id4, id);
    HE_EXPECT_EQ(p.Size(), 1);

    const char* str = p.Get(id);
    HE_EXPECT_EQ_STR(str, TestString);
    HE_EXPECT_EQ(p.Size(), 1);

    const char* str3 = p.Get(id3);
    HE_EXPECT_EQ(str3, nullptr);
    HE_EXPECT_EQ(p.Size(), 1);

    for (auto it = p.begin(); it != p.end(); ++it)
    {
        HE_EXPECT_EQ(it.Id().val, 1);
        HE_EXPECT_EQ_STR(it.String(), TestString);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_pool, stress_usage)
{
    constexpr uint32_t StringCount = 100000;

    String buf;
    StringPool p;

    // Add many unique strings to the pool
    for (uint32_t i = 1; i <= StringCount; ++i)
    {
        buf.Clear();
        FormatTo(buf, "{}", i);
        StringPoolId id = p.Add(buf.Data());
        HE_EXPECT_EQ(id.val, i);
    }
    HE_EXPECT_EQ(p.Size(), StringCount);

    // test idempotency of add the same strings again
    for (uint32_t i = 1; i <= StringCount; ++i)
    {
        buf.Clear();
        FormatTo(buf, "{}", i);
        StringPoolId id = p.Add(buf.Data());
        HE_EXPECT_EQ(id.val, i);
    }
    HE_EXPECT_EQ(p.Size(), StringCount);

    // test idempotency of add the same strings again, but as StringViews
    for (uint32_t i = 1; i <= StringCount; ++i)
    {
        buf.Clear();
        FormatTo(buf, "{}", i);
        StringPoolId id = p.Add(buf);
        HE_EXPECT_EQ(id.val, i);
    }
    HE_EXPECT_EQ(p.Size(), StringCount);

    // Find each of the strings in the pool
    for (uint32_t i = 1; i <= StringCount; ++i)
    {
        buf.Clear();
        FormatTo(buf, "{}", i);
        StringPoolId id = p.Find(buf.Data());
        HE_EXPECT_EQ(id.val, i);
    }

    // Find each of the strings in the pool, but as StringViews
    for (uint32_t i = 1; i <= StringCount; ++i)
    {
        buf.Clear();
        FormatTo(buf, "{}", i);
        StringPoolId id = p.Find(buf);
        HE_EXPECT_EQ(id.val, i);
    }

    // Get each of the strings from the pool
    for (uint32_t i = 1; i <= StringCount; ++i)
    {
        buf.Clear();
        FormatTo(buf, "{}", i);
        const char* str = p.Get({ i });
        HE_EXPECT_EQ_STR(str, buf.Data());
    }

    // Iterate through each of the strings in the pool
    uint32_t i = 1;
    for (auto it = p.begin(); it != p.end(); ++it)
    {
        buf.Clear();
        FormatTo(buf, "{}", i);

        HE_EXPECT_EQ(it.Id().val, i);
        HE_EXPECT_EQ_STR(it.String(), buf.Data());
        ++i;
    }
    HE_EXPECT_EQ(p.Size(), (i - 1));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_pool, larger_than_page_size)
{
    StringPool p;

    const uint32_t pageSize = p.PageSize();
    char* largeStr = Allocator::GetDefault().Malloc<char>(pageSize + 1);
    MemSet(largeStr, 'x', pageSize);
    largeStr[pageSize] = '\0';

    HE_EXPECT_VERIFY({
        StringPoolId id = p.Add(largeStr);
        HE_EXPECT(!id);
        HE_EXPECT_EQ(id.val, 0);
    });

    Allocator::GetDefault().Free(largeStr);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_pool, zero_size_string)
{
    StringPool p;

    StringPoolId id = p.Add("");
    HE_EXPECT(!id);
    HE_EXPECT_EQ(id.val, 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_pool, add_str_view)
{
    StringPool p;

    const StringPoolId id1 = p.Add("Hello!");
    HE_EXPECT(id1);

    const StringPoolId id2 = p.Add(StringView("Hello!"));
    HE_EXPECT(id2);
    HE_EXPECT_EQ(id1, id2);

    const char* str1 = p.Get(id1);
    HE_EXPECT(str1);

    const char* str2 = p.Get(id2);
    HE_EXPECT(str2);
    HE_EXPECT_EQ_STR(str1, str2);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_pool, find_str_view)
{
    StringPool p;

    p.Add("Hello!");

    const StringPoolId id1 = p.Find("Hello!");
    HE_EXPECT(id1);

    const StringPoolId id2 = p.Find(StringView("Hello!"));
    HE_EXPECT(id2);
    HE_EXPECT_EQ(id1, id2);

    const StringPoolId id3 = p.Find("nope");
    HE_EXPECT(!id3);

    const StringPoolId id4 = p.Find(StringView("nope"));
    HE_EXPECT(!id4);
    HE_EXPECT_EQ(id3, id4);

    const char* str1 = p.Get(id1);
    HE_EXPECT(str1);

    const char* str2 = p.Get(id2);
    HE_EXPECT(str2);
    HE_EXPECT_EQ_STR(str1, str2);

    const char* str3 = p.Get(id3);
    HE_EXPECT_EQ(str3, nullptr);

    const char* str4 = p.Get(id4);
    HE_EXPECT_EQ(str4, nullptr);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_pool, fmt)
{
    {
        const StringPoolId id;
        const String str = Format("{}", id);
        HE_EXPECT_EQ(str, "0");
    }

    {
        const StringPoolId id{ 50 };
        const String str = Format("{}", id);
        HE_EXPECT_EQ(str, "50");
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, string_pool, RangeBasedFor)
{
    // TODO
}
