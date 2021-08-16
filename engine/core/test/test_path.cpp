#include "he/core/path.h"

#include "he/core/allocator.h"
#include "he/core/string.h"
#include "he/core/string_view_fmt.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, path, IsAbsolutePath)
{
    struct TestCase
    {
        const char* s;
        const bool exp;
    } tests[] =
    {
        { "", false },
        { "a", false },
        { "a/", false },
        { "a/b", false },
        { "a/b/", false },
        { "C:", false },
        { "/a", true },
        { "/a/", true },
        { "c:/", true },
        { "c:\\foo", true },
        { "\\thing", true },
    };

    for (TestCase tc : tests)
    {
        HE_EXPECT_EQ(IsAbsolutePath(tc.s), tc.exp);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, path, GetExtension)
{
    struct TestCase
    {
        const char* s;
        const char* exp;
    } tests[] =
    {
        { "", "" },
        { "a", "" },
        { "a.b", ".b" },
        { "a.b.c", ".c" },
        { "a/b.c", ".c" },
        { "a.b/c", "" },
        { "a/b", "" },
        { ".abcd", "" },
        { ".ab.cd", ".cd" },
        { "a/b/.abcd", "" },
        { "a/b/.ab.cd", ".cd" },
    };

    for (TestCase tc : tests)
    {
        const StringView ext = GetExtension(tc.s);
        HE_EXPECT_EQ(ext, tc.exp);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, path, GetDirName)
{
    struct TestCase
    {
        const char* s;
        const char* exp;
    } tests[] =
    {
        { "", "" },
        { "a", "" },
        { "a/", "a" },
        { "a/b", "a" },
        { "a/b/", "a/b" },
        { "/a", "" },
        { "/a/", "/a" },
        { "c:/", "c:" },
        { "c:/foo", "c:" },
    };

    for (TestCase tc : tests)
    {
        const StringView dir = GetDirName(tc.s);
        HE_EXPECT_EQ(dir, tc.exp);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, path, GetBaseName)
{
    struct TestCase
    {
        const char* s;
        const char* exp;
    } tests[] =
    {
        { "", "" },
        { "a", "a" },
        { "a/", "" },
        { "a/b", "b" },
        { "a/b/", "" },
        { "/a", "a" },
        { "/a/", "" },
        { "c:/", "" },
        { "c:/foo", "foo" },
    };

    for (TestCase tc : tests)
    {
        const StringView dir = GetBaseName(tc.s);
        HE_EXPECT_EQ(dir, tc.exp);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, path, NormalizePath)
{
    struct TestCase
    {
        const char* s;
        const char* exp;
    } tests[] =
    {
        { "", "" },
        { "/", "/" },
        { "/a", "/a" },
        { "a", "a" },
        { "a/", "a/" },
        { "a/b", "a/b" },
        { "a/b/", "a/b/" },
        { "a//b", "a/b" },
        { ".", "." },
        { "./", "." },
        { "./a", "a" },
        { "/.", "/" },
        { "a/.", "a/" },
        { "a/./", "a/" },
        { "a/./b", "a/b" },
        { "a/b/.", "a/b/" },
        { "a/./b/.", "a/b/" },
        { "..", ".." },
        { "/..", "/.." },
        { "../..", "../.." },
        { "../../a", "../../a" },
        { "../../a/..", "../../" },
        { "../../a/../..", "../../.." },
        { "../../a/b/../../", "../../" },
        { "../../a/b/../c", "../../a/c" },
        { "../../a/b/../c/../../", "../../" },
        { "a/..", "." },
        { "a/b/..", "a/" },
        { "a/../../b", "../b" },
        { "/a/../../b", "../b" },
        { "../a/../../b", "../../b" },
        { "a/../b", "b" },
        { "a/b/../c", "a/c" },
        { "a/../b/./c", "b/c" },
        { "a/b//../c/", "a/c/" },
        { "a\\b\\c", "a/b/c" },
        { "C:\\foo.txt", "c:/foo.txt" },

        // UNC paths
        { "//a/b", "//a/b" },
        { "///a//b", "//a/b" },
        { "//", "//" },
        { "///", "//" },
        { "\\\\a\\b", "//a/b" },
        { "\\\\\\a\\\\b", "//a/b" },
        { "\\\\", "//" },
        { "\\\\\\", "//" },
    };

    String buf(CrtAllocator::Get());
    for (TestCase tc : tests)
    {
        buf = tc.s;
        NormalizePath(buf);
        HE_EXPECT_EQ_STR(buf.Data(), tc.exp);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, path, ConcatPath)
{
    struct TestCase
    {
        const char* a;
        const char* b;
        const char* exp;
    } tests[] =
    {
        { "", "", "" },
        { "a", "", "a/" },
        { "a", "b", "a/b" },
        { "a/", "", "a/" },
        { "a\\", "", "a\\" },
        { "a/", "b", "a/b" },
        { "a\\", "b", "a\\b" },
        { "a/b", "", "a/b/" },
        { "a/b", "c", "a/b/c" },
        { "a/b", "/c", "/c" },
    };

    String buf(CrtAllocator::Get());
    for (TestCase tc : tests)
    {
        buf = tc.a;
        ConcatPath(buf, tc.b);
        HE_EXPECT_EQ_STR(buf.Data(), tc.exp);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, path, RemoveExtension)
{
    struct TestCase
    {
        const char* s;
        const char* exp;
    } tests[] =
    {
        { "", "" },
        { "a", "a" },
        { "a.b", "a" },
        { "a.b.c", "a.b" },
    };

    String buf(CrtAllocator::Get());
    for (TestCase tc : tests)
    {
        buf = tc.s;
        RemoveExtension(buf);
        HE_EXPECT_EQ_STR(buf.Data(), tc.exp);
    }
}
