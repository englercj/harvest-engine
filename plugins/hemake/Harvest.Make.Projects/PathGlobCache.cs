// Copyright Chad Engler

using Microsoft.Extensions.FileSystemGlobbing;

namespace Harvest.Make.Projects;

public class PathGlobCache
{
    private readonly Dictionary<(string Path, string BaseDir), List<string>> _cache = [];

    public IReadOnlyList<string> ExpandPath(string? path, string baseDir)
    {
        if (string.IsNullOrEmpty(path))
        {
            return [];
        }

        string searchBaseDir = baseDir;
        string searchPattern = path;

        if (Path.IsPathRooted(path))
        {
            int wildcardIndex = path.IndexOfAny(['*', '?']);
            int separatorSearchIndex = wildcardIndex >= 0 ? wildcardIndex : path.Length - 1;
            int separatorIndex = path.LastIndexOfAny([Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar], separatorSearchIndex);
            if (separatorIndex >= 0)
            {
                searchBaseDir = path[..separatorIndex];
                searchPattern = path[(separatorIndex + 1)..];
            }
            else
            {
                searchBaseDir = Path.GetPathRoot(path) ?? baseDir;
                searchPattern = Path.GetRelativePath(searchBaseDir, path);
            }
        }

        (string Path, string BaseDir) cacheKey = (searchPattern, searchBaseDir);
        if (_cache.TryGetValue(cacheKey, out List<string>? cachedPaths))
        {
            return cachedPaths;
        }

        Matcher matcher = new();
        matcher.AddInclude(searchPattern.Replace('\\', '/'));
        return _cache[cacheKey] = [.. matcher.GetResultsInFullPath(searchBaseDir)];
    }
}
