// Copyright Chad Engler

#include "file_helpers.posix.h"

#include "he/core/fmt.h"
#include "he/core/string_view.h"
#include "he/core/path.h"
#include "he/core/utils.h"
#include "he/core/wstr.h"

#if defined(HE_PLATFORM_API_POSIX) && !defined(HE_PLATFORM_EMSCRIPTEN)

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace he
{
    int PosixFileOpen(const char* path, FileOpenMode mode, FileOpenFlag openFlags, int extraFlags)
    {
        int flags = extraFlags;
        switch (mode)
        {
            case FileOpenMode::Write:
                flags = O_WRONLY | O_CREAT;
                break;
            case FileOpenMode::ReadWrite:
                flags = O_RDWR | O_CREAT;
                break;
            case FileOpenMode::ReadExisting:
                flags = O_RDONLY;
                break;
            case FileOpenMode::ReadWriteExisting:
                flags = O_RDWR;
                break;
            case FileOpenMode::WriteTruncate:
                flags = O_WRONLY | O_CREAT | O_TRUNC;
                break;
            case FileOpenMode::ReadWriteTruncate:
                flags = O_RDWR | O_CREAT | O_TRUNC;
                break;
            case FileOpenMode::WriteAppend:
                flags = O_WRONLY | O_APPEND | O_CREAT;
                break;
            case FileOpenMode::ReadWriteAppend:
                flags = O_RDWR | O_APPEND | O_CREAT;
                break;
            default:
                errno = EINVAL;
                return -1;
        }

        flags |= O_CLOEXEC; // do not inherit descriptor for child process

        if (HasFlags(openFlags, FileOpenFlag::NoBuffering))
            flags |= O_DIRECT;

        int fd = open(path, flags, 0666);
        if (fd < 0)
            return fd;

        int advice = POSIX_FADV_NORMAL;
        if (HasFlags(openFlags, FileOpenFlag::RandomAccess))
            advice = POSIX_FADV_RANDOM;
        else if (HasFlags(openFlags, FileOpenFlag::SequentialScan))
            advice = POSIX_FADV_SEQUENTIAL;

        if (advice != POSIX_FADV_NORMAL)
        {
            // Not checking the error case because the file should continue
            // to operate correctly even if this fails.
            posix_fadvise(fd, 0, 0, advice);
        }

        return fd;
    }

    void PosixFileGatherAttributes(const struct stat& sb, const char* path, FileAttributes& attribs)
    {
        attribs.flags = FileAttributeFlag::None;

        if (path != nullptr)
        {
            const StringView baseName = GetBaseName(path);
            if (baseName[0] == '.')
                attribs.flags |= FileAttributeFlag::Hidden;
        }

        mode_t test = S_IWOTH;

        if (getuid() == sb.st_uid)
            test = S_IWUSR;
        else if (getgid() == sb.st_gid)
            test = S_IXGRP;

        if ((sb.st_mode & test) == 0)
            attribs.flags |= FileAttributeFlag::ReadOnly;

        if (S_ISDIR(sb.st_mode))
        {
            attribs.flags |= FileAttributeFlag::Directory;
            attribs.size = 0;
        }
        else
        {
            attribs.size = static_cast<uint64_t>(sb.st_size);
        }

        attribs.createTime = { 0 };
    #if _POSIX_VERSION < 200809L
        timespec ts{};
        ts.tv_sec = sb.st_atime;
        attribs.accessTime = PosixTimeToSystemTime(ts);
        ts.tv_sec = sb.st_mtime;
        attribs.writeTime = PosixTimeToSystemTime(ts);
    #else
        attribs.accessTime = PosixTimeToSystemTime(sb.st_atim);
        attribs.writeTime = PosixTimeToSystemTime(sb.st_mtim);
    #endif
    }

    Result PosixFileGetAttributes(int fd, FileAttributes& outAttributes)
    {
        struct stat sb;
        if (fstat(fd, &sb))
            return Result::FromLastError();

        // A bit of a hidden allocator here but is necessary to avoid PATH_MAX issues.
        String path;
        Result r = PosixFileGetPath(fd, path);

        PosixFileGatherAttributes(sb, r ? path.Data() : nullptr, outAttributes);

        return Result::Success;
    }

    Result PosixFileGetPath(int fd, String& outPath)
    {
        const String linkPath = Format("/proc/self/fd/{}", fd);
        Result r = PosixReadLink(linkPath.Data(), outPath);

        // Size of link name changed between lstate and readlink, and it got larger. Try reading
        // it one more time before giving up.
        if (!r)
        {
            r = PosixReadLink(linkPath.Data(), outPath);
        }

        return r;
    }

    Result PosixReadLink(const char* linkPath, String& outPath)
    {
        // POSIX says we should be able to read the size of the link here
        // but linux isn't POSIX compliant in the /proc filesystem.
        // if (lstat(linkPath, &sb) == -1)
        //     return Result::FromLastError();

        // outPath.Resize(sb.st_size + 1);

        const uint32_t offset = out.Size();
        uint32_t size = String::MaxEmbedCharacters;
        outPath.Resize(outPath.Size() + size, DefaultInit);

        do
        {
            ssize_t r = readlink(linkPath, outPath.Data() + offset, size);
            if (r < 0)
                return Result::FromLastError();

            if (r < size)
            {
                // resize to properly null terminate
                outPath.Resize(offset + r);
                return Result::Success;
            }

            size = Max(512u, size * 2);
            outPath.Resize(offset + size, DefaultInit);
        } while (true);

        return PosixResult(ENAMETOOLONG);
    }
}

#endif
