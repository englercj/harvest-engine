// Copyright Chad Engler

#include "he/core/path.h"

#include "he/core/ascii.h"
#include "he/core/assert.h"
#include "he/core/directory.h"
#include "he/core/memory_ops.h"
#include "he/core/range_ops.h"
#include "he/core/string.h"
#include "he/core/string_ops.h"

#include <algorithm>

namespace he
{
    inline bool _IsSlash(char c)
    {
        return c == '/' || c == '\\';
    }

    bool IsAbsolutePath(const char* path)
    {
        if (StrEmpty(path))
            return false;

        // Leading slash is an absolute path
        if (_IsSlash(path[0]))
            return true;

        // Leading drive letter is an absolute path
        if (IsAlpha(path[0]) && path[1] == ':' && _IsSlash(path[2]))
            return true;

        return false;
    }

    bool IsAbsolutePath(StringView path)
    {
        if (path.IsEmpty())
            return false;

        // Leading slash is an absolute path
        if (_IsSlash(path[0]))
            return true;

        // Leading drive letter is an absolute path
        if (path.Size() > 2 && path[1] == ':' && _IsSlash(path[2]))
            return true;

        return false;
    }

    bool IsChildPath(StringView path, StringView parent)
    {
        if (path.IsEmpty() || parent.IsEmpty())
            return false;

        // This is intended to work only with normalized and absolute paths
        if (!IsAbsolutePath(path) || !IsAbsolutePath(parent))
            return false;

        // If the path is shorter than the parent then it can't refer to something inside it
        if (path.Size() <= parent.Size())
            return false;

        // It is a child if they have the same path up until the end of the parent
        for (uint32_t i = 0; i < parent.Size(); ++i)
        {
            const char c = path[i];
            const char p = parent[i];

            if (c != p && !_IsSlash(c) && !_IsSlash(p))
                return false;
        }

        // Any non-slash characters after we reach the end of the parent path means this is just
        // a name that is a sibling of the parent path, which is not a child.
        if (!_IsSlash(parent.Back()) && !_IsSlash(path[parent.Size()]))
            return false;

        // Check for a trailing slash as the only additional character in the child path.
        // If that's the case, then the paths are equivalent and therefore not a child
        if ((path.Size() - 1) == parent.Size() && _IsSlash(path.Back()))
            return false;

        // If we got here the paths are not equal, and path goes
        return true;
    }

    StringView GetExtension(StringView path)
    {
        if (path.IsEmpty())
            return {};

        const StringView baseName = GetBaseName(path);

        const char* begin = baseName.Begin();
        const char* end = RangeFind(begin, baseName.Size(), ':'); // Handle NTFS Alternate Data Streams

        if (end == nullptr)
            end = baseName.End();

        const char* ext = end;

        // empty
        if (begin >= ext)
            return {};

        // path is length 1, so there is no extension
        --ext;
        if (begin >= ext)
            return {};

        // possible extension here, check for special '..' case or return otherwise
        if (*ext == '.')
        {
            if (begin <= (ext - 1) && ext[-1] == '.')
                return {};
            else
                return { ext, end };
        }

        // Search for a dot that is not in the first position
        while (begin < --ext)
        {
            if (*ext == '.')
                return { ext, end };
        }

        // Here we failed to find a dot that isn't in the first position, there is no extension
        return {};
    }

    StringView GetDirectory(StringView path)
    {
        if (path.IsEmpty())
            return {};

        const uint32_t rootLen = GetRootName(path).Size();
        const char* begin = path.Begin() + rootLen;
        const char* end = path.End();

        while (_IsSlash(*begin))
            ++begin;

        while (begin < end && !_IsSlash(end[-1]))
            --end;

        while (begin < end && _IsSlash(end[-1]))
            --end;

        return { path.Begin(), end };
    }

    StringView GetBaseName(StringView path)
    {
        if (path.IsEmpty())
            return {};

        const uint32_t rootLen = GetRootName(path).Size();
        const char* begin = path.Begin() + rootLen;
        const char* end = path.End();

        while (begin < end && !_IsSlash(end[-1]))
            --end;

        return { end, path.End() };
    }

    StringView GetPathWithoutExtension(StringView path)
    {
        if (path.IsEmpty())
            return path;

        const StringView ext = GetExtension(path);
        if (ext.IsEmpty())
        {
            const StringView baseName = GetBaseName(path);
            const char* end = RangeFind(baseName.Begin(), baseName.Size(), ':'); // Handle NTFS Alternate Data Streams
            return { path.Begin(), end ? end : path.End() };
        }

        HE_ASSERT(ext.Data() >= path.Data());

        const uint32_t len = static_cast<uint32_t>(ext.Data() - path.Data());

        HE_ASSERT(len <= path.Size());

        return { path.Data(), len };
    }

    StringView GetRootName(StringView path)
    {
        if (path.IsEmpty())
            return {};

        // If the path is this short there is no root name
        if (path.Size() < 2)
            return {};

        // Leading drive letter is common and easy
        if (path.Size() >= 2 && IsAlpha(path[0]) && path[1] == ':')
            return { path.Begin(), 2 };

        // All path that are not drive-prefixed start with a slash
        if (!_IsSlash(path[0]))
            return {};

        // Handle the `\xx\$` form of windows paths, where `$` is any non-slash character.
        // Some examples: `\\?\device`, `\??\device`, `\\.\device`, `\\?\UNC\server\share`
        if (path.Size() >= 4 && _IsSlash(path[3]) && (path.Size() == 4 || !_IsSlash(path[4])))
        {
            if ((_IsSlash(path[1]) && (path[2] == '?' || path[2] == '.')) // \\?\$ or \\.\$
                || (path[1] == '?' && path[2] == '?')) // \??\$
            {
                return { path.Begin(), 3 };
            }
        }

        // Handle `\\server\share` paths.
        if (path.Size() >= 3 && _IsSlash(path[1]) && !_IsSlash(path[2]))
        {
            const char* end = path.Begin() + 3;
            while (end < path.End() && !_IsSlash(*end)) { ++end; }

            return { path.Begin(), end };
        }

        // no match
        return {};
    }

    void NormalizePath(String& path)
    {
        if (path.IsEmpty())
            return;

        const char* begin = path.Begin();
        const char* p = begin;
        char* q = path.Begin();

        const char* componentStart = p;
        uint32_t depth = 0;
        uint32_t parentDepth = 0;

        while (true)
        {
            char c = *p++;

            if (_IsSlash(c) || c == '\0')
            {
                const char* componentEnd = p - 1;
                size_t componentLen = componentEnd - componentStart;
                bool parent = false;

                // Handle special 'current directory' specifier ('.')
                if (componentLen == 1 && componentStart[0] == '.')
                {
                    componentLen = 0;
                }
                // Handle special 'parent directory' specifier ('..')
                else if (componentLen == 2 && componentStart[0] == '.' && componentStart[1] == '.')
                {
                    if (depth > 0)
                    {
                        componentLen = 0;

                        // go back to the previous component.
                        while (q > begin)
                        {
                            --q;
                            if (*q == '/')
                                break;
                        }

                        --depth;
                    }

                    parent = true;
                }

                // Skip any empty components, except at the start or end
                if (componentLen > 0 || q == begin || (q == begin + 1 && *begin == '/') || c == '\0')
                {
                    // leading slash
                    if (componentStart == begin && componentEnd == begin && c != '\0')
                    {
                        *q++ = '/';
                    }
                    // second starting slash of a UNC path
                    else if (componentStart == begin + 1 && componentEnd == begin + 1 && q == begin + 1 && *begin == '/' && c != '\0')
                    {
                        *q++ = '/';
                    }
                    else
                    {
                        // add separator between elements
                        if ((depth > 0 || parentDepth > 0))
                            *q++ = '/';

                        if (componentLen > 0)
                        {
                            if (q != componentStart)
                            {
                                MemMove(q, componentStart, componentLen);
                            }
                            q += componentLen;

                            if (parent)
                                ++parentDepth;
                            else
                                ++depth;
                        }
                    }
                }

                componentStart = p;
            }

            if (c == '\0')
                break;
        }

        path.Resize(static_cast<uint32_t>(q - begin));

        if (path.IsEmpty())
        {
            path.PushBack('.');
        }
        else if (path.Size() > 2 && path[1] == ':' && path[2] == '/')
        {
            path[0] = ToLower(path[0]);
        }
    }

    void ConcatPath(String& path, StringView components)
    {
        if (IsAbsolutePath(components))
        {
            path.Assign(components.Data(), components.Size());
            return;
        }

        if (!path.IsEmpty() && !_IsSlash(path.Back()))
            path.PushBack('/');

        path += components;
    }

    void RemoveExtension(String& path)
    {
        const StringView withoutExt = GetPathWithoutExtension(path);
        HE_ASSERT(withoutExt.Data() == path.Data());
        HE_ASSERT(withoutExt.Size() <= path.Size());
        path.Resize(withoutExt.Size());
    }

    void RemoveBaseName(String& path)
    {
        const StringView baseName = GetBaseName(path);
        HE_ASSERT(baseName.IsEmpty() || baseName.End() == path.End());
        HE_ASSERT(baseName.Size() <= path.Size());
        path.Resize(path.Size() - baseName.Size());
    }

    bool MakeRelative(String& path, StringView base)
    {
        // They need to both be absolute, or both relative
        if (IsAbsolutePath(path) != IsAbsolutePath(base))
            return false;

        const StringView pathRootName = GetRootName(path);
        const StringView baseRootName = GetRootName(base);

        // If the root names don't match then they can't be relative to eachother
        // For example a file on C: can't be relative to a path on D:
        if (pathRootName != baseRootName)
            return false;

        HE_ASSERT(pathRootName.Size() == baseRootName.Size());
        const uint32_t rootNameLen = pathRootName.Size();

        const bool pathHasRootDir = path.Size() > rootNameLen && _IsSlash(path[rootNameLen]);
        const bool baseHasRootDir = base.Size() > rootNameLen && _IsSlash(base[rootNameLen]);

        // If the path does not have a slash after the root name, but the base does then
        // they can't be relative to eachother. This handles some weird Win32 paths where
        // such as "c:dog" trying to be relative to "c:/".
        if (!pathHasRootDir && baseHasRootDir)
        {
            return false;
        }

        const PathSplitter pathSplit(path);
        const PathSplitter baseSplit(base);

        const PathIterator pathEnd = pathSplit.end();
        const PathIterator baseBegin = baseSplit.begin();
        const PathIterator baseEnd = baseSplit.end();

        const auto mismatchPair = std::mismatch(pathSplit.begin(), pathEnd, baseBegin, baseEnd);
        PathIterator pathIt = mismatchPair.first;
        PathIterator baseIt = mismatchPair.second;

        if (pathIt == pathEnd && baseIt == baseEnd)
        {
            path = ".";
            return true;
        }

        // Skip root name and directory elements
        {
            ptrdiff_t baseDist = std::distance(baseBegin, baseIt);

            const bool baseHasRootName = !baseRootName.IsEmpty();
            const ptrdiff_t baseRootDist = static_cast<ptrdiff_t>(baseHasRootName) + static_cast<ptrdiff_t>(baseHasRootDir);

            while (baseDist < baseRootDist)
            {
                ++baseIt;
                ++baseDist;
            }
        }

        int32_t num = 0;
        for (; baseIt != baseEnd; ++baseIt)
        {
            const StringView& element = *baseIt;

            if (element.IsEmpty())
            {
                // skip empty elements
            }
            else if (element == ".")
            {
                // skip elements that are single dot
            }
            else if (element == "..")
            {
                --num;
            }
            else
            {
                ++num;
            }
        }

        if (num < 0)
            return false;

        if (num == 0 && (pathIt == pathEnd || pathIt->IsEmpty()))
        {
            path = ".";
            return true;
        }

        // At this point we've validated that the path can be made relative, and we've collected
        // everything we need to know. Time to build the actual string.
        // This part has some extra complexity to allow writing the updated string inline into
        // the existing `path` buffer. This avoid allocating a new string and copying it over
        // at the cost of a bit more work and complexity.

        const char* pElm = pathIt->Data();
        const uint32_t relativeLen = static_cast<uint32_t>(path.End() - pElm);
        const uint32_t dotDotSlashLen = 3 * num; // 3 == "../"
        const uint32_t newLen = dotDotSlashLen + relativeLen;

        // If we shorten the string, we need to first save the part of the path we care about
        // by moving it to the front. The resize later could otherwise ruin our precious path.
        if (newLen < path.Size())
        {
            MemMove(path.Data(), pElm, relativeLen);
            pElm = path.Begin();
        }

        // Resize the string to the new size and copy the preserved path elements to the correct
        // location after the new "../" entries.
        path.Resize(newLen, DefaultInit);
        MemMove(path.Data() + dotDotSlashLen, pElm, relativeLen);

        // Write each of the new "../" entries to the start of the string
        for (int32_t i = 0; i < num; ++i)
        {
            const int32_t offset = i * 3;
            MemCopy(path.Data() + offset, "../", 3);
        }

        return true;
    }

    Result MakeAbsolute(String& path)
    {
        if (IsAbsolutePath(path))
            return Result::Success;

        String cwd;
        Result r = Directory::GetCurrent(cwd);
        if (!r)
            return r;

        ConcatPath(cwd, path);

        path = Move(cwd);
        return Result::Success;
    }

    PathIterator::PathIterator(StringView path)
        : m_path(path)
        , m_element()
    {
        const char* end = m_path.Begin();
        while (end < m_path.End() && !_IsSlash(*end))
            ++end;

        m_element = { m_path.Begin(), end };
    }

    bool PathIterator::operator==(const PathIterator& x) const
    {
        const bool samePath = m_path.Data() == x.m_path.Data() && m_path.Size() == x.m_path.Size();
        const bool sameElement = m_element.Data() == x.m_element.Data() && m_element.Size() == x.m_element.Size();
        return samePath && sameElement;
    }

    void PathIterator::GoToNext()
    {
        // Start at the first character of the element, skipping leading slashes
        const char* begin = m_element.End();

        if (begin < m_path.End() && _IsSlash(*begin))
            ++begin;

        // Find the next slash to end at
        const char* end = begin;

        while (end < m_path.End() && !_IsSlash(*end))
            ++end;

        // set the new element
        m_element = { begin, end };
    }
}
