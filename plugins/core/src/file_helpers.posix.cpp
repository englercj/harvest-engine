// Copyright Chad Engler

#include "file_helpers.win32.h"

#include "he/core/enum_ops.h"
#include "he/core/wstr.h"

#if defined(HE_PLATFORM_API_POSIX) && !defined(HE_PLATFORM_EMSCRIPTEN)

#include <errno.h>
#include <fcntl.h>

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
}

#endif
