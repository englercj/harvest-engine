// Copyright Chad Engler

#include "he/core/wstr.h"

#include "he/core/alloca.h"
#include "he/core/test.h"

#include <cstring>
#include <cwchar>

using namespace he;

// ------------------------------------------------------------------------------------------------
template <uint32_t N>
static void Test_MBToWCStr(const char* src, const wchar_t (&expected)[N])
{
    uint32_t len = MBToWCStr(nullptr, 0, src);
    HE_EXPECT_EQ(len, N);

    wchar_t* dst = HE_ALLOCA(wchar_t, len);
    len = MBToWCStr(dst, len, src);
    HE_EXPECT(wcscmp(dst, expected) == 0);
    HE_EXPECT_EQ(len, N);
}

template <uint32_t N>
static void Test_WCToMBStr(const wchar_t* src, const char (&expected)[N])
{
    uint32_t len = WCToMBStr(nullptr, 0, src);
    HE_EXPECT_EQ(len, N);

    char* dst = HE_ALLOCA(char, len);
    len = WCToMBStr(dst, len, src);
    HE_EXPECT_EQ_STR(dst, expected);
    HE_EXPECT_EQ(len, N);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, wstr, MBToWCStr)
{
    Test_MBToWCStr("z\u00df\u6c34\U0001f34c", L"zß水🍌");
    Test_MBToWCStr("\x7a\xc3\x9f\xe6\xb0\xb4\xf0\x9f\x8d\x8c", L"zß水🍌");
    Test_MBToWCStr("\u041B\u0435\u043D\u0438\u043D", L"Ленин");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, wstr, WCToMBStr)
{
    Test_WCToMBStr(L"zß水🍌", "z\u00df\u6c34\U0001f34c");
    Test_WCToMBStr(L"zß水🍌", "\x7a\xc3\x9f\xe6\xb0\xb4\xf0\x9f\x8d\x8c");
    Test_WCToMBStr(L"Ленин", "\u041B\u0435\u043D\u0438\u043D");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, wstr, WCStrCmp)
{
    HE_EXPECT(WCStrCmp(L"zß水🍌", L"zß水🍌") == 0);
    HE_EXPECT(WCStrCmp(L"zß水🍌", L"Ленин") < 0);
    HE_EXPECT(WCStrCmp(L"Андропов", L"Ленин") < 0);
}
