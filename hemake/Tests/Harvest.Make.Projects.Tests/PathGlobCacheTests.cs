// Copyright Chad Engler

namespace Harvest.Make.Projects.Tests;

public sealed class PathGlobCacheTests
{
    [Fact]
    public void ExpandPathExpandsRootedGlobPatterns()
    {
        string rootDir = Directory.CreateTempSubdirectory("hemake_pathglob_tests-").FullName;

        try
        {
            string sourceDir = Path.Join(rootDir, "sources");
            Directory.CreateDirectory(sourceDir);

            File.WriteAllText(Path.Join(sourceDir, "a.cpp"), "int a() { return 0; }");
            File.WriteAllText(Path.Join(sourceDir, "b.cpp"), "int b() { return 0; }");
            File.WriteAllText(Path.Join(sourceDir, "c.h"), "#pragma once");

            PathGlobCache cache = new();
            IReadOnlyList<string> results = cache.ExpandPath(Path.Join(sourceDir, "*.cpp"), rootDir);

            Assert.Equal(2, results.Count);
            Assert.Contains(Path.Join(sourceDir, "a.cpp"), results);
            Assert.Contains(Path.Join(sourceDir, "b.cpp"), results);
        }
        finally
        {
            if (Directory.Exists(rootDir))
            {
                Directory.Delete(rootDir, true);
            }
        }
    }
}
