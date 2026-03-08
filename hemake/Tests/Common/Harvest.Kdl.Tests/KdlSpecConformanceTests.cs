// Copyright Chad Engler

using Harvest.Kdl.Exceptions;

namespace Harvest.Kdl.Tests;

public sealed class KdlSpecConformanceTests
{
    [Fact]
    public void RoundTripsKdlSpecTestCases()
    {
        string testCasesRoot = ResolveTestCasesRoot();
        string inputDir = Path.Combine(testCasesRoot, "input");
        string expectedDir = Path.Combine(testCasesRoot, "expected_kdl");

        List<string> failures = [];

        foreach (string inputPath in Directory.GetFiles(inputDir, "*.kdl", SearchOption.TopDirectoryOnly).OrderBy(Path.GetFileName))
        {
            string fileName = Path.GetFileName(inputPath);
            string expectedPath = Path.Combine(expectedDir, fileName);
            bool hasExpected = File.Exists(expectedPath);

            string input = File.ReadAllText(inputPath);

            KdlDocument? document = null;
            Exception? parseException = null;

            try
            {
                document = KdlDocument.FromString(fileName, input);
            }
            catch (Exception ex)
            {
                parseException = ex;
            }

            if (!hasExpected)
            {
                if (parseException is null)
                {
                    failures.Add($"{fileName}: expected parse failure but parsed successfully.");
                }

                continue;
            }

            if (parseException is not null)
            {
                failures.Add($"{fileName}: expected parse success but received exception '{parseException.GetType().Name}': {parseException.Message}");
                continue;
            }

            string expected = File.ReadAllText(expectedPath);
            string actual = ToKdlString(document!, GetSpecWriteOptions());

            if (!string.Equals(actual, expected, StringComparison.Ordinal))
            {
                failures.Add($"{fileName}: round-trip mismatch.\nexpected: {ToSingleLine(expected)}\nactual:   {ToSingleLine(actual)}");
            }
        }

        Assert.True(
            failures.Count == 0,
            string.Join("\n\n", failures));
    }

    [Fact]
    public void ParsesVersionMarkerFromSpec()
    {
        KdlDocument doc = KdlDocument.FromString(
            filePath: "version.kdl",
            str: "/- kdl-version 2\nnode #true\n");

        Assert.Single(doc.Nodes);
        Assert.Equal("node", doc.Nodes[0].Name);
    }

    [Fact]
    public void ThrowsOnInvalidVersionMarker()
    {
        KdlParseException ex = Assert.Throws<KdlParseException>(() =>
            KdlDocument.FromString("version_fail.kdl", "/- kdl-version 1\nnode #true\n"));

        Assert.Contains("Only KDL v2 is supported", ex.Message, StringComparison.Ordinal);
    }

    private static KdlWriteOptions GetSpecWriteOptions()
    {
        return new KdlWriteOptions
        {
            PrintEmptyChildren = false,
        };
    }

    private static string ToKdlString(IKdlObject obj, KdlWriteOptions options)
    {
        using StringWriter writer = new();
        obj.WriteKdl(writer, options);
        writer.Flush();
        return writer.ToString();
    }

    private static string ToSingleLine(string str)
    {
        return str
            .Replace("\r", "\\r", StringComparison.Ordinal)
            .Replace("\n", "\\n", StringComparison.Ordinal);
    }

    private static string ResolveTestCasesRoot()
    {
        string? current = AppContext.BaseDirectory;

        while (current is not null)
        {
            string repoRootMarker = Path.Combine(current, "he_project.kdl");
            if (File.Exists(repoRootMarker))
            {
                string kdlRepoTests = Path.GetFullPath(Path.Combine(current, "..", "kdl", "tests", "test_cases"));
                if (Directory.Exists(kdlRepoTests))
                {
                    return kdlRepoTests;
                }

                string vendoredTests = Path.Combine(current, "plugins", "core", "test", "fixtures", "kdl", "test_cases");
                if (Directory.Exists(vendoredTests))
                {
                    return vendoredTests;
                }

                break;
            }

            current = Directory.GetParent(current)?.FullName;
        }

        throw new DirectoryNotFoundException("Unable to resolve KDL test_cases directory. Expected ../kdl/tests/test_cases or plugins/core/test/fixtures/kdl/test_cases.");
    }
}
