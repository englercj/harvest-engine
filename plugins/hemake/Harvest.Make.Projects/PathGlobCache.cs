// Copyright Chad Engler

using Microsoft.Extensions.FileSystemGlobbing;

namespace Harvest.Make.Projects;

internal class PathGlobCache
{
    private readonly Dictionary<string, List<string>> _cache = new();

    public IReadOnlyList<string> ExpandPath(string? path, string baseDir)
    {
        if (string.IsNullOrEmpty(path))
        {
            return [];
        }

        if (_cache.TryGetValue(path, out List<string>? cachedPaths))
        {
            return cachedPaths;
        }

        Matcher matcher = new();
        matcher.AddInclude(path);
        return _cache[path] = [.. matcher.GetResultsInFullPath(baseDir)];
    }
}
