// Copyright Chad Engler

#include "file_helpers.win32.h"

#include "he/core/wstr.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

#include <fileapi.h>

namespace he
{
    HANDLE Win32FileOpen(const char* path, FileOpenMode mode, FileOpenFlag flags, DWORD extraFlags)
    {
        DWORD dwDesiredAccess = 0;
        DWORD dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        DWORD dwCreationDisposition = 0;

        switch (mode)
        {
            case FileOpenMode::Write:
                dwDesiredAccess = GENERIC_WRITE;
                dwCreationDisposition = OPEN_ALWAYS;
                break;
            case FileOpenMode::ReadWrite:
                dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
                dwCreationDisposition = OPEN_ALWAYS;
                break;
            case FileOpenMode::ReadExisting:
                dwDesiredAccess = GENERIC_READ;
                dwCreationDisposition = OPEN_EXISTING;
                break;
            case FileOpenMode::ReadWriteExisting:
                dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
                dwCreationDisposition = OPEN_EXISTING;
                break;
            case FileOpenMode::WriteTruncate:
                dwDesiredAccess = GENERIC_WRITE;
                dwCreationDisposition = CREATE_ALWAYS;
                break;
            case FileOpenMode::ReadWriteTruncate:
                dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
                dwCreationDisposition = CREATE_ALWAYS;
                break;
            case FileOpenMode::WriteAppend:
                dwDesiredAccess = GENERIC_WRITE;
                dwCreationDisposition = OPEN_ALWAYS;
                break;
            case FileOpenMode::ReadWriteAppend:
                dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
                dwCreationDisposition = OPEN_ALWAYS;
                break;
            default:
                ::SetLastError(ERROR_INVALID_PARAMETER);
                return INVALID_HANDLE_VALUE;
        }

        DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL | extraFlags;

        if (HasFlags(flags, FileOpenFlag::NoBuffering))
            dwFlagsAndAttributes |= FILE_FLAG_NO_BUFFERING;

        if (HasFlags(flags, FileOpenFlag::RandomAccess))
            dwFlagsAndAttributes |= FILE_FLAG_RANDOM_ACCESS;
        else if (HasFlags(flags, FileOpenFlag::SequentialScan))
            dwFlagsAndAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;

        return ::CreateFileW(
            HE_TO_WSTR(path),
            dwDesiredAccess,
            dwShareMode,
            nullptr,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            nullptr);
    }
}

#endif
