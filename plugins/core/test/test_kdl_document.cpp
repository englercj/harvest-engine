// Copyright Chad Engler

#include "he/core/kdl_document.h"

#include "he/core/directory.h"
#include "he/core/file.h"
#include "he/core/hash_table.h"
#include "he/core/path.h"
#include "he/core/test.h"
#include "he/core/types.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, toml_document, Construct)
{
    KdlDocument doc;
    HE_EXPECT_EQ_PTR(&doc.GetAllocator(), &Allocator::GetDefault());
    HE_EXPECT(doc.Nodes().IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, toml_document, ConstructWithAllocator)
{
    CrtAllocator allocator;
    KdlDocument doc(allocator);
    HE_EXPECT_EQ_PTR(&doc.GetAllocator(), &allocator);
    HE_EXPECT(doc.Nodes().IsEmpty());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, toml_document, Read)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, toml_document, Write)
{
    KdlDocument doc;

    KdlNode& a = doc.Nodes().EmplaceBack("a");
    a.Arguments().EmplaceBack(true);

    KdlNode& b = doc.Nodes().EmplaceBack("b");
    b.Arguments().EmplaceBack(42);

    KdlNode& c = doc.Nodes().EmplaceBack("c");
    KdlNode& d = c.Children().EmplaceBack("d");
    d.Arguments().EmplaceBack("yes");

    KdlNode& e = doc.Nodes().EmplaceBack("e");
    e.Properties().Emplace("f0", true);
    e.Properties().Emplace("f1", false);

    const String output = doc.ToString();
    HE_EXPECT_EQ(output, "a #true\nb 42\nc {\n    d yes\n}\ne f0=#true f1=#false\n");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, toml_document, Roundtrips)
{
    const StringView filePath = HE_FILE;
    const StringView testDir = GetDirectory(filePath);

    String inputDir = testDir;
    ConcatPath(inputDir, "kdl/test_cases/input");

    String expectedDir = testDir;
    ConcatPath(inputDir, "kdl/test_cases/expected_kdl");

    struct TestCase
    {
        String input;
        String expected;
    };
    HashMap<String, TestCase> testCases;

    // Read all the input and expected files for the KDL tests
    DirectoryScanner scanner;
    Result r = scanner.Open(inputDir.Data());
    HE_EXPECT(r, r);

    DirectoryScanner::Entry entry;
    while (scanner.NextEntry(entry))
    {
        if (entry.isDirectory)
            continue;

        if (GetExtension(entry.name) != ".kdl")
            continue;

        TestCase& test = testCases.Emplace(entry.name).entry.value;

        String path = inputDir;
        ConcatPath(path, entry.name);
        r = File::ReadAll(test.input, path.Data());
        HE_EXPECT(r, r);

        path = expectedDir;
        ConcatPath(path, entry.name);
        r = File::ReadAll(test.expected, path.Data());

        // Expected file may not exist, this is OK. It means that the input should fail to parse.
        if (GetFileResult(r) != FileResult::NotFound)
        {
            HE_EXPECT(r, r);
        }
    }

    // Test each file by round tripping the input and checking it against the expected output.
    for (const auto& test : testCases)
    {
        const String& input = test.value.input;
        const String& expected = test.value.expected;

        KdlDocument doc;
        const KdlReadResult r = doc.Read(input);

        if (expected.IsEmpty())
        {
            HE_EXPECT(!r.IsValid());
            continue;
        }

        HE_EXPECT(r.IsValid(), r.error, r.column, r.line);

        String output;
        doc.Write(output);

        HE_EXPECT_EQ(output, expected);
    }
}
