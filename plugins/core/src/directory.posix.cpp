// Copyright Chad Engler

#include "he/core/directory.h"

#include "he/core/assert.h"
#include "he/core/path.h"

#if defined(HE_PLATFORM_API_POSIX)

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace he::Directory
{
    Scanner::Scanner(Allocator& allocator)
        : m_allocator(allocator)
        , m_impl(nullptr)
    {}

    Scanner::~Scanner()
    {
        Close();
    }

    Result Scanner::Open(const char* path)
    {
        HE_ASSERT(m_impl == nullptr);

        m_impl = opendir(path);

        if (!m_impl)
            return Result::FromLastError();

        return Result::Success;
    }

    void Scanner::Close()
    {
        DIR* dir = static_cast<DIR*>(m_impl);
        if (dir)
        {
            closedir(dir);
            m_impl = nullptr;
        }
    }

    bool Scanner::NextEntry(String& outName, bool* outIsDirectory)
    {
        DIR* dir = static_cast<DIR*>(m_impl);
        HE_ASSERT(dir);

        while (true)
        {
            struct dirent* entry = readdir(dir);
            if (!entry)
                return false;

            const char* fname = entry->d_name;

            if (fname[0] == '.' && (fname[1] == '\0' || (fname[1] == '.' && fname[2] == '\0')))
                continue;

            outName = fname;

            if (outIsDirectory)
                *outIsDirectory = !!(entry->d_type & DT_DIR);

            return true;
        }
    }

    Result GetSpecial(String& dst, SpecialId dir)
    {
        switch (dir)
        {
            case SpecialId::LocalAppData:
            {
                const char* xdgDataHome = getenv("XDG_DATA_HOME");
                if (!String::IsEmpty(xdgDataHome) && IsAbsolutePath(xdgDataHome))
                {
                    dst = xdgDataHome;
                    return Result::Success;
                }

                // POSIX requires the OS to always set a value for $HOME.
                const char* homeDir = getenv("HOME");
                if (!homeDir)
                    return Result::NotSupported;

                dst = homeDir;
                ConcatPath(dst, ".local/share");
                return Result::Success;
            }
            case SpecialId::SharedAppData:
            {
                const char* xdgDataDirs = getenv("XDG_DATA_DIR");

                if (String::IsEmpty(xdgDataDirs))
                {
                    xdgDataDirs = "/usr/local/share/:/usr/share/";
                }

                // Find the first entry that exists and return it
                const char* start = xdgDataDirs;
                while (*start)
                {
                    const char* end = String::Find(start, ':');

                    if (end)
                        dst.Assign(start, (end - start));
                    else
                        dst = start;

                    if (IsAbsolutePath(dst) && Exists(dst.Data()))
                        return Result::Success;

                    start = end + 1;
                }

                // Nothing exists, so just return the first one.
                const char* end = String::Find(xdgDataDirs, ':');

                if (end)
                    dst.Assign(start, (end - start));
                else
                    dst = xdgDataDirs;

                return Result::Success;
            }
            case SpecialId::Documents:
            {
                const char* xdgDocumentsDir = getenv("XDG_DOCUMENTS_DIR");
                if (xdgDocumentsDir && IsAbsolutePath(xdgDocumentsDir))
                {
                    dst = xdgDocumentsDir;
                    return Result::Success;
                }

                // POSIX requires the OS to always set a value for $HOME.
                const char* homeDir = getenv("HOME");
                if (!homeDir)
                    return Result::NotSupported;

                dst = homeDir;
                ConcatPath(dst, ".local/documents");
                return Result::Success;
            }
            case SpecialId::Temp:
            {
                constexpr const char TempDirFallback[] = "/tmp";
                const char* src = getenv("TMPDIR");
                dst = src ? src : TempDirFallback;
                return Result::Success;
            }
        }

        HE_ASSERT(false, "Unknown special directory type");
        return Result::NotSupported;
    }

    Result GetCurrent(String& dst)
    {
        dst.Resize(PATH_MAX);

        if (!getcwd(dst.Data(), dst.Size()))
            return Result::FromLastError();

        dst.Resize(String::Length(dst.Data()));
        return Result::Success;
    }

    Result SetCurrent(const char* path)
    {
        if (chdir(path))
            return Result::FromLastError();

        return Result::Success;
    }

    Result Rename(const char* oldPath, const char* newPath)
    {
        if (rename(oldPath, newPath))
            return Result::FromLastError();

        return Result::Success;
    }

    bool Exists(const char* path)
    {
        struct stat sb;
        return !stat(path, &sb) && S_ISDIR(sb.st_mode);
    }

    Result Create(const char* path, bool parents)
    {
        if (parents)
        {
            char parentDirs[PATH_MAX];
            String::Copy(parentDirs, path);

            for (char* p = parentDirs + 1; *p; ++p)
            {
                if ((*p == '/' || *p == '\\') && p[1] && p[1] != '/' && p[1] != '\\')
                {
                    // Truncate the path at the separator, create the parent directory, then restore the separator
                    char sep = *p;
                    *p = '\0';
                    if (mkdir(parentDirs, 0777) && errno != EEXIST)
                        return Result::FromLastError();
                    *p = sep;
                }
            }
        }

        if (mkdir(path, 0777) && errno != EEXIST)
            return Result::FromLastError();

        return Result::Success;
    }

    Result Remove(const char* path)
    {
        if (rmdir(path))
            return Result::FromLastError();

        return Result::Success;
    }
}

#endif
