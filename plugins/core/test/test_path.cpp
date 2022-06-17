// Copyright Chad Engler

// Some test cases in here are from Microsoft's STL tests.
// Licensed under Apache License v2.0 with LLVM Exception
// License: https://github.com/microsoft/STL/blob/9947dd9d6d2506029ed4486bb7596bda7d70f99d/LICENSE.txt
// Source: https://github.com/microsoft/STL/blob/9947dd9d6d2506029ed4486bb7596bda7d70f99d/tests/std/tests/P0218R1_filesystem/test.cpp

#include "he/core/path.h"

#include "he/core/directory.h"
#include "he/core/file.h"
#include "he/core/result_fmt.h"
#include "he/core/string.h"
#include "he/core/string_view_fmt.h"
#include "he/core/test.h"

using namespace he;

struct DecompTestCase
{
    const char* input;
    const char* rootName;
    const char* rootDirectory;
    const char* relativePath;
    const char* parentPath;
    const char* filename;
    bool isAbsolute;
};

static DecompTestCase DecompTestCases[] =
{
    { "", "", "", "", "", "", false },
    { ".", "", "", ".", "", ".", false },
    { "..", "", "", "..", "", "..", false },
    { "c:", "c:", "", "", "c:", "", false }, // empty drive-relative path
    { "c:dog", "c:", "", "dog", "c:", "dog", false }, // non-empty drive-relative path
    { "C:", "C:", "", "", "C:", "", false }, // also test uppercase version
    { "cc:dog", "", "", "cc:dog", "", "cc:dog", false }, // malformed drive (more follow)
    { "::dog", "", "", "::dog", "", "::dog", false },
    { " :dog", "", "", " :dog", "", " :dog", false },
    { ":dog", "", "", ":dog", "", ":dog", false },
    { "\\:dog", "", "\\", ":dog", "\\", ":dog", true },
    { "\\:\\dog", "", "\\", ":\\dog", "\\:", "dog", true },
    { "1:\\dog", "", "", "1:\\dog", "1:", "dog", false },
    { "\\Dev\\msvc", "", "\\", "Dev\\msvc", "\\Dev", "msvc", true }, // current drive root relative path. NOTE: MS STL considers this non-root, Harvest considers it root.
    { "c:\\", "c:", "\\", "", "c:\\", "", true }, // absolute Win32 path
    { "c:/", "c:", "/", "", "c:/", "", true },
    { "c:\\cat\\dog\\elk", "c:", "\\", "cat\\dog\\elk", "c:\\cat\\dog", "elk", true }, // usual path form
    { "c:\\cat\\dog\\elk\\", "c:", "\\", "cat\\dog\\elk\\", "c:\\cat\\dog\\elk", "", true }, // with trailing backslash
    { "c:/cat\\dog\\elk", "c:", "/", "cat\\dog\\elk", "c:/cat\\dog", "elk", true }, // also with some /s
    { "c:\\cat\\dog/elk/", "c:", "\\", "cat\\dog/elk/", "c:\\cat\\dog/elk", "", true }, // with extra long root-directory:
    { "c:\\\\\\\\\\cat\\dog\\elk", "c:", "\\\\\\\\\\", "cat\\dog\\elk", "c:\\\\\\\\\\cat\\dog", "elk", true },
    { "\\\\?\\x", "\\\\?", "\\", "x", "\\\\?\\", "x", true }, // special case \\?\ prefix
    { "\\\\?\\", "\\\\?", "\\", "", "\\\\?\\", "", true }, // special case \\?\ prefix with no suffix

    // with extra trailing \\, not the special prefix any longer, but becomes the \\\\server form:
    { "\\\\?\\\\", "\\\\?", "\\\\", "", "\\\\?\\\\", "", true },
    { "\\\\.\\x", "\\\\.", "\\", "x", "\\\\.\\", "x", true }, // also \\.\ special prefix
    { "\\??\\x", "\\??", "\\", "x", "\\??\\", "x", true }, // also \??\ special prefix

    // adding an extra trailing \\ to this one makes it become a root relative path:
    // NOTE: In Harvest we consider "root relative" paths as absolute
    { "\\??\\\\", "", "\\", "??\\\\", "\\??", "", true },
    { "\\x?\\x", "", "\\", "x?\\x", "\\x?", "x", true }, // not special (more follow)
    { "\\?x\\x", "", "\\", "?x\\x", "\\?x", "x", true },
    { "\\\\x\\x", "\\\\x", "\\", "x", "\\\\x\\", "x", true },
    { "\\??", "", "\\", "??", "\\", "??", true },
    { "\\???", "", "\\", "???", "\\", "???", true },
    { "\\????", "", "\\", "????", "\\", "????", true },
    { "\\\\.xx", "\\\\.xx", "", "", "\\\\.xx", "", true }, // became \\server form
    { "\\\\server\\share", "\\\\server", "\\", "share", "\\\\server\\", "share", true }, // network path
    { "/\\server\\share", "/\\server", "\\", "share", "/\\server\\", "share", true },
    { "/\\server/share", "/\\server", "/", "share", "/\\server/", "share", true },
    { "//server\\share", "//server", "\\", "share", "//server\\", "share", true },
    { "//server/share", "//server", "/", "share", "//server/", "share", true },
    { "\\/server\\share", "\\/server", "\\", "share", "\\/server\\", "share", true },
    { "\\/server/share", "\\/server", "/", "share", "\\/server/", "share", true },

    // \\\ doesn't fall into \\server, becomes a current drive root relative path:
    // NOTE: In Harvest we consider "root relative" paths as absolute
    { "\\\\\\\\", "", "\\\\\\\\", "", "\\\\\\\\", "", true },
    { "\\\\\\dog", "", "\\\\\\", "dog", "\\\\\\", "dog", true },

    // document that we don't treat \\?\UNC special even if MSDN does:
    { "\\\\?\\UNC\\server\\share", "\\\\?", "\\", "UNC\\server\\share", "\\\\?\\UNC\\server", "share", true },
    { "\\\\?\\UNC", "\\\\?", "\\", "UNC", "\\\\?\\", "UNC", true }, // other similar cases
    { "\\\\?\\UNC\\server", "\\\\?", "\\", "UNC\\server", "\\\\?\\UNC", "server", true },
    { "\\\\?\\UNC\\server\\", "\\\\?", "\\", "UNC\\server\\", "\\\\?\\UNC\\server", "", true },
    { "\\\\?\\UNC\\\\", "\\\\?", "\\", "UNC\\\\", "\\\\?\\UNC", "", true },

    // document that drive letters aren't special with special prefixes:
    { "\\\\.\\C:attempt_at_relative", "\\\\.", "\\", "C:attempt_at_relative", "\\\\.\\", "C:attempt_at_relative", true },

    // other interesting user-submitted test cases:
    // NOTE: In Harvest we consider "root relative" paths as absolute
    { "\\", "", "\\", "", "\\", "", true },
    { "\\\\", "", "\\\\", "", "\\\\", "", true },
    { "\\\\\\", "", "\\\\\\", "", "\\\\\\", "", true },
    { "\\\\\\.", "", "\\\\\\", ".", "\\\\\\", ".", true },
    { "\\\\\\..", "", "\\\\\\", "..", "\\\\\\", "..", true },
    { "\\\\\\.\\", "", "\\\\\\", ".\\", "\\\\\\.", "", true },
    { "\\\\\\..\\", "", "\\\\\\", "..\\", "\\\\\\..", "", true },
    { "/", "", "/", "", "/", "", true },
    { "//", "", "//", "", "//", "", true },
    { "///", "", "///", "", "///", "", true },
    { "\\/", "", "\\/", "", "\\/", "", true },
    { "/c", "", "/", "c", "/", "c", true },
    { "/c:", "", "/", "c:", "/", "c:", true },
    { "..", "", "", "..", "", "..", false },
    { "\\.", "", "\\", ".", "\\", ".", true },
    { "/.", "", "/", ".", "/", ".", true },
    { "\\..", "", "\\", "..", "\\", "..", true },
    { "\\..\\..", "", "\\", "..\\..", "\\..", "..", true },
    { "c:\\..", "c:", "\\", "..", "c:\\", "..", true },
    { "c:..", "c:", "", "..", "c:", "..", false },
    { "c:..\\..", "c:", "", "..\\..", "c:..", "..", false },
    { "c:\\..\\..", "c:", "\\", "..\\..", "c:\\..", "..", true },
    { "\\\\server\\..", "\\\\server", "\\", "..", "\\\\server\\", "..", true },
    { "\\\\server\\share\\..\\..", "\\\\server", "\\", "share\\..\\..", "\\\\server\\share\\..", "..", true },
    { "\\\\server\\.\\share", "\\\\server", "\\", ".\\share", "\\\\server\\.", "share", true },
    { "\\\\server\\.\\..\\share", "\\\\server", "\\", ".\\..\\share", "\\\\server\\.\\..", "share", true },
    { "\\..\\../", "", "\\", "..\\../", "\\..\\..", "", true },
    { "\\\\..\\../", "\\\\..", "\\", "../", "\\\\..\\..", "", true },
    { "\\\\\\..\\../", "", "\\\\\\", "..\\../", "\\\\\\..\\..", "", true },
    { "\\\\?/", "\\\\?", "/", "", "\\\\?/", "", true },
    { "\\/?/", "\\/?", "/", "", "\\/?/", "", true },
    { "//?/", "//?", "/", "", "//?/", "", true },
    { "//server", "//server", "", "", "//server", "", true },
    { "[:/abc", "", "", "[:/abc", "[:", "abc", false }, // bug where [ was considered a drive letter

    // a few extra forward-slash cases
    { "//?/device", "//?", "/", "device", "//?/", "device", true },
    { "/??/device", "/??", "/", "device", "/??/", "device", true },
    { "//./device", "//.", "/", "device", "//./", "device", true },
    { "//?/UNC/server/share", "//?", "/", "UNC/server/share", "//?/UNC/server", "share", true },
    { "/home", "", "/", "home", "/", "home", true },

    // Some NTFS Alternate Data Stream test cases
    { ".:alternate_meow", "", "", ".:alternate_meow", "", ".:alternate_meow", false },
    { "..:alternate_dog", "", "", "..:alternate_dog", "", "..:alternate_dog", false },
    { ".config:alternate_elk", "", "", ".config:alternate_elk", "", ".config:alternate_elk", false },
    { "file.txt:$DATA", "", "", "file.txt:$DATA", "", "file.txt:$DATA", false },
};

// ------------------------------------------------------------------------------------------------
HE_TEST(core, path, IsAbsolutePath)
{
    for (const DecompTestCase& tc : DecompTestCases)
    {
        HE_EXPECT_EQ(IsAbsolutePath(tc.input), tc.isAbsolute);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, path, IsChildPath)
{
    struct TestCase
    {
        const char* child;
        const char* parent;
        const bool exp;
    } tests[] =
    {
        { "/a", "/", true },
        { "/a/b", "/a/", true },
        { "/a/b", "/a", true },
        { "/ab/c", "/ab/", true },
        { "/ab/c", "/ab", true },

        { "", "/", false },
        { "/a/b", "/ab/", false },
        { "/a/", "/ab/", false },
        { "/ab", "/ab", false },
        { "/ab", "/ab/", false },
        { "/ab/", "/ab", false },
        { "/abc", "/ab", false },
    };

    for (TestCase tc : tests)
    {
        HE_EXPECT_EQ(IsChildPath(tc.child, tc.parent), tc.exp);
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
        { ".", "" },
        { "..", "" },
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

        { ".config", "" },
        { "..config", ".config" },
        { "...config", ".config" },
        { "file.txt", ".txt" },
        { "cat.dog.elk.tar", ".tar" },
        { "cat.dog.elk", ".elk" },
        { "cat.dog", ".dog" },
        { "cat", "" },
        { "cat.", "." },
        { ".:alternate_meow", "" },
        { "..:alternate_dog", "" },
        { ".config:alternate_elk", "" },
        { "..config:alternate_other_things", ".config" },
        { "...config:alternate_other_things", ".config" },
        { "classic_textfile.txt:$DATA", ".txt" },
        { "cat.dog.elk.tar:pay_no_attention", ".tar" },
        { "cat.dog.elk:to behind this curtain", ".elk" },
        { "cat.dog:even:if:this:curtain:is:malformed", ".dog" },
        { "cat:what?", "" },
        { "cat.:alternate_fun", "." },
    };

    for (TestCase tc : tests)
    {
        const StringView ext = GetExtension(tc.s);
        HE_EXPECT_EQ(ext, tc.exp);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, path, GetDirectory)
{
    for (const DecompTestCase& tc : DecompTestCases)
    {
        const StringView dir = GetDirectory(tc.input);
        HE_EXPECT_EQ(dir, tc.parentPath);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, path, GetBaseName)
{
    for (const DecompTestCase& tc : DecompTestCases)
    {
        const StringView dir = GetBaseName(tc.input);
        HE_EXPECT_EQ(dir, tc.filename);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, path, GetPathWithoutExtension)
{
    struct TestCase
    {
        const char* s;
        const char* exp;
    } tests[] =
    {
        { "", "" },
        { ".", "." },
        { "..", ".." },
        { "a", "a" },
        { "a.b", "a" },
        { "a.b.c", "a.b" },
        { "/a.c", "/a" },
        { "a/b.c", "a/b" },
        { "a/b", "a/b" },
        { "a/b/", "a/b/" },

        { ".config", ".config" },
        { "..config", "." },
        { "...config", ".." },
        { "file.txt", "file" },
        { "cat.dog.elk.tar", "cat.dog.elk" },
        { "cat.dog.elk", "cat.dog" },
        { "cat.dog", "cat" },
        { "cat", "cat" },
        { "cat.", "cat" },
        { ".:alternate_meow", "." },
        { "..:alternate_dog", ".." },
        { ".config:alternate_elk", ".config" },
        { "..config:alternate_other_things", "." },
        { "...config:alternate_other_things", ".." },
        { "classic_textfile.txt:$DATA", "classic_textfile" },
        { "cat.dog.elk.tar:pay_no_attention", "cat.dog.elk" },
        { "cat.dog.elk:to behind this curtain", "cat.dog" },
        { "cat.dog:even:if:this:curtain:is:malformed", "cat" },
        { "cat:what?", "cat" },
        { "cat.:alternate_fun", "cat" },
    };

    for (TestCase tc : tests)
    {
        const StringView path = GetPathWithoutExtension(tc.s);
        HE_EXPECT_EQ(path, tc.exp);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, path, GetRootName)
{
    for (const DecompTestCase& tc : DecompTestCases)
    {
        const StringView dir = GetRootName(tc.input);
        HE_EXPECT_EQ(dir, tc.rootName);
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

    String buf;
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

    String buf;
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
        { "/a.c", "/a" },
        { "a/b.c", "a/b" },
        { "a/b", "a/b" },
        { "a/b/", "a/b/" },
    };

    String buf;
    for (TestCase tc : tests)
    {
        buf = tc.s;
        RemoveExtension(buf);
        HE_EXPECT_EQ_STR(buf.Data(), tc.exp);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, path, MakeRelative)
{
    struct TestCase
    {
        const char* a;
        const char* b;
        const char* exp;
        bool ret;
    } tests[] =
    {
        { "", "", ".", true },
        { "/a/d", "/a/b/c", "../../d", true },
        { "/a/b/c", "/a/d", "../b/c", true },
        { "a/b/c", "a", "b/c", true },
        { "a/b/c", "a/b/c/x/y", "../../", true },
        { "a/b/c", "a/b/c", ".", true },
        { "a/b", "c/d", "../../a/b", true },
        { "/a/d", "/a/b/c", "../../d", true },
        { "/a/b/c", "/a/d", "../b/c", true },
        { "a/b/c", "a", "b/c", true },
        { "a/b/c", "a/b/c/x/y", "../../", true },
        { "a/b/c", "a/b/c", ".", true },
        { "a/b/c", "a/b/c/", ".", true },
        { "a/b", "c/d", "../../a/b", true },

        { "C:\\Temp", "D:\\Temp", "C:\\Temp", false },
        { "C:\\Temp", "Temp", "C:\\Temp", false },
        { "Temp", "C:\\Temp", "Temp", false },
        { "one", "\\two", "one", false },
        { "cat", "..\\..\\..\\meow", "cat", false },
        { "cat", "..\\..\\..\\meow\\.\\.\\.\\.\\.", "cat", false },
        { "a\\b\\c\\x\\y\\z", "a\\b\\c\\d\\.\\e\\..\\f\\g", "../../../x\\y\\z", true },
        { "a\\b\\c\\x\\y\\z", "a\\b\\c\\d\\.\\e\\..\\f\\g\\..\\..\\..", "x\\y\\z", true },
        { "\\a:\\b:", "\\a:\\c:", "../b:", true }, // NOTE: Harvest 
    };

    String buf;
    for (TestCase tc : tests)
    {
        buf = tc.a;
        const bool ret = MakeRelative(buf, tc.b);
        HE_EXPECT_EQ(ret, tc.ret);
        HE_EXPECT_EQ_STR(buf.Data(), tc.exp);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, path, MakeAbsolute)
{
    constexpr char TestPath[] = "347ef2ad-3afe-4e11-ae99-a8ab43ce50fb";

    String expectedPath;
    Result r = Directory::GetCurrent(expectedPath);
    HE_EXPECT(r, r);
    ConcatPath(expectedPath, TestPath);
    NormalizePath(expectedPath);

    r = File::WriteAll(TestPath, HE_LENGTH_OF(TestPath), TestPath);
    HE_EXPECT(r, r);

    String path = TestPath;
    r = MakeAbsolute(path);
    HE_EXPECT(r, r);
    NormalizePath(path);

    HE_EXPECT_EQ(path, expectedPath);

    File::Remove(TestPath);
}

static void TestSplitPath(const char* path, Span<const char* const> expected)
{
    uint32_t i = 0;
    for (const StringView& element : PathSplitter(path))
    {
        HE_EXPECT_EQ(element, expected[i]);
        ++i;
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, path, __aPathSplitter)
{
    constexpr const char* Basic[] = { "", "home", "user" };
    TestSplitPath("/home/user", Basic);

    constexpr const char* Win32Absolute[] = { "C:", "a", "b", "c", "d", "e.txt:stuff" };
    TestSplitPath("C:\\a/b/c\\d\\e.txt:stuff", Win32Absolute);

    constexpr const char* RelativeTrailing[] = { "a", "c", "" };
    TestSplitPath("a/c/", RelativeTrailing);

    constexpr const char* MultiSlash[] = { "", "a", "", "", "", "b", "" };
    TestSplitPath("/a////b/", MultiSlash);
}
