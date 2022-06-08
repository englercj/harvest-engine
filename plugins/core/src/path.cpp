// Copyright Chad Engler

#include "he/core/path.h"

#include "he/core/ascii.h"
#include "he/core/assert.h"
#include "he/core/file.h"
#include "he/core/memory_ops.h"

namespace he
{
    bool IsAbsolutePath(const char* path)
    {
        if (String::IsEmpty(path))
            return false;

        // Leading slash is an absolute path
        if (path[0] == '/' || path[0] == '\\')
            return true;

        // Leading drive letter is an absolute path
        if (IsAlpha(path[0]) && path[1] == ':' && (path[2] == '/' || path[2] == '\\'))
            return true;

        return false;
    }

    bool IsAbsolutePath(StringView path)
    {
        if (path.IsEmpty())
            return false;

        // Leading slash is an absolute path
        if (path[0] == '/' || path[0] == '\\')
            return true;

        // Leading drive letter is an absolute path
        if (path.Size() > 2 && path[1] == ':' && (path[2] == '/' || path[2] == '\\'))
            return true;

        return false;
    }

    bool IsChildPath(StringView path, StringView parent)
    {
        if (path.Size() <= parent.Size())
            return false;

        if (!IsAbsolutePath(path) || !IsAbsolutePath(parent))
            return false;

        for (uint32_t i = 0; i < parent.Size(); ++i)
        {
            const char c = path[i];
            const char p = parent[i];

            if (c != p)
                return false;
        }

        return true;
    }

    StringView GetExtension(StringView path)
    {
        if (path.IsEmpty())
            return {};

        const char* p = path.End() - 1;
        const char* begin = path.Begin();

        while (p >= begin)
        {
            if (*p == '/' || *p == '\\')
                return {};

            if (*p == '.')
            {
                if (p == begin || p[-1] == '/' || p[-1] == '\\')
                    return {};
                else
                    return { p, path.End() };
            }

            --p;
        }

        return {};
    }

    StringView GetDirectory(StringView path)
    {
        if (path.IsEmpty())
            return {};

        const char* p = path.End() - 1;
        const char* begin = path.Begin();

        while (p >= begin)
        {
            if (*p == '/' || *p == '\\')
                return { path.Begin(), p };

            --p;
        }

        return {};
    }

    StringView GetBaseName(StringView path)
    {
        if (path.IsEmpty())
            return {};

        const char* p = path.End() - 1;
        const char* begin = path.Begin();

        while (p >= begin)
        {
            if (*p == '/' || *p == '\\')
                return { p + 1, path.End() };

            --p;
        }

        return path;
    }

    StringView GetPathWithoutExtension(StringView path)
    {
        if (path.IsEmpty())
            return path;

        const StringView ext = GetExtension(path);

        if (ext.IsEmpty())
            return path;

        HE_ASSERT(ext.Data() >= path.Data());

        const uint32_t len = static_cast<uint32_t>(ext.Data() - path.Data());

        HE_ASSERT(len <= path.Size());

        return { path.Data(), len };
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

            if (c == '/' || c == '\\' || c == '\0')
            {
                const char* componentEnd = p - 1;
                size_t componentLen = componentEnd - componentStart;
                bool parent = false;

                // Handle special directory specifiers (".", "..")
                if (componentLen == 1 && componentStart[0] == '.')
                {
                    componentLen = 0;
                }
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

        if (!path.IsEmpty() && path.Back() != '/' && path.Back() != '\\')
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

    Result MakeAbsolute(String& path)
    {
        if (path.IsEmpty() || IsAbsolutePath(path))
            return Result::Success;

        File f;
        Result r = f.Open(path.Data(), FileOpenMode::ReadExisting);
        if (!r)
            return r;

        return f.GetPath(path);
    }
}
