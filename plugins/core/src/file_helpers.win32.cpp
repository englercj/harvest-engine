// Copyright Chad Engler

#include "file_helpers.win32.h"

#include "he/core/ascii.h"
#include "he/core/enum_ops.h"
#include "he/core/scope_guard.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/wstr.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

namespace he
{
    inline uint64_t Win32ToUint64(DWORD high, DWORD low)
    {
        ULARGE_INTEGER value{ .LowPart = low, .HighPart = high };
        return value.QuadPart;
    }

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
            HE_TO_WCSTR(path),
            dwDesiredAccess,
            dwShareMode,
            nullptr,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            nullptr);
    }

    FileAttributeFlag Win32ParseFileAttributeFlags(DWORD dwFileAttributes)
    {
        FileAttributeFlag flags = FileAttributeFlag::None;

        if (HasFlag(dwFileAttributes, FILE_ATTRIBUTE_HIDDEN))
            flags |= FileAttributeFlag::Hidden;

        if (HasFlag(dwFileAttributes, FILE_ATTRIBUTE_READONLY))
            flags |= FileAttributeFlag::ReadOnly;

        if (HasFlag(dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
            flags |= FileAttributeFlag::Directory;

        return flags;
    }

    void Win32ParseFileAttributes(const WIN32_FILE_ATTRIBUTE_DATA& info, FileAttributes& outAttributes)
    {
        ULARGE_INTEGER i{};

        outAttributes.flags = Win32ParseFileAttributeFlags(info.dwFileAttributes);

        if (HasFlag(outAttributes.flags, FileAttributeFlag::Directory))
        {
            outAttributes.size = 0;
        }
        else
        {
            i.HighPart = info.nFileSizeHigh;
            i.LowPart = info.nFileSizeLow;
            outAttributes.size = i.QuadPart;
        }

        i.HighPart = info.ftCreationTime.dwHighDateTime;
        i.LowPart = info.ftCreationTime.dwLowDateTime;
        outAttributes.createTime = Win32FileTimeToSystemTime(i.QuadPart);

        i.HighPart = info.ftLastAccessTime.dwHighDateTime;
        i.LowPart = info.ftLastAccessTime.dwLowDateTime;
        outAttributes.accessTime = Win32FileTimeToSystemTime(i.QuadPart);

        i.HighPart = info.ftLastWriteTime.dwHighDateTime;
        i.LowPart = info.ftLastWriteTime.dwLowDateTime;
        outAttributes.writeTime = Win32FileTimeToSystemTime(i.QuadPart);
    }

    void Win32ParseFileAttributes(const BY_HANDLE_FILE_INFORMATION& info, FileAttributes& outAttributes)
    {
        WIN32_FILE_ATTRIBUTE_DATA attrData{};
        attrData.dwFileAttributes = info.dwFileAttributes;
        attrData.ftCreationTime = info.ftCreationTime;
        attrData.ftLastAccessTime = info.ftLastAccessTime;
        attrData.ftLastWriteTime = info.ftLastWriteTime;
        attrData.nFileSizeHigh = info.nFileSizeHigh;
        attrData.nFileSizeLow = info.nFileSizeLow;
        Win32ParseFileAttributes(attrData, outAttributes);
    }

    Result Win32FileGetAttributes(HANDLE handle, FileAttributes& outAttributes)
    {
        BY_HANDLE_FILE_INFORMATION fileInfo{};
        if (!::GetFileInformationByHandle(handle, &fileInfo))
            return Result::FromLastError();

        Win32ParseFileAttributes(fileInfo, outAttributes);
        return Result::Success;
    }

    Result Win32FileGetPath(HANDLE handle, String& outPath)
    {
        DWORD flags = VOLUME_NAME_DOS;

        while (true)
        {
            DWORD wideRequiredLen = ::GetFinalPathNameByHandleW(handle, nullptr, 0, flags);
            if (wideRequiredLen == 0)
            {
                // Its possible there is no DOS name for the file, so try to get the NT path
                if (::GetLastError() == ERROR_PATH_NOT_FOUND && flags == VOLUME_NAME_DOS)
                {
                    flags = VOLUME_NAME_NT;
                    continue;
                }

                return Result::FromLastError();
            }

            wchar_t* buf = outPath.GetAllocator().Malloc<wchar_t>(wideRequiredLen);
            HE_AT_SCOPE_EXIT([&]()
            {
                outPath.GetAllocator().Free(buf);
            });

            DWORD widePathLen = ::GetFinalPathNameByHandleW(handle, buf, wideRequiredLen, flags);
            if (widePathLen == 0 || widePathLen > wideRequiredLen)
                return Result::FromLastError();

            WCToMBStr(outPath, buf);
            break;
        }

        if (flags == VOLUME_NAME_DOS)
        {
            constexpr StringView Prefix = "\\\\?\\";
            constexpr StringView UNCPrefix = "\\\\?\\UNC\\";

            // Check if path starts with a \\?\X: prefix, and if so remove the \\?\ prefix
            if (outPath.Size() >= 6
                && String::EqualN(outPath.Data(), Prefix.Data(), Prefix.Size())
                && IsAlpha(outPath[4])
                && outPath[5] == ':'
                && outPath[6] == '\\')
            {
                outPath.Erase(0, Prefix.Size());
            }
            // Check if path starts with a \\?\UNC\ prefix, and if so replace it with simpler \\ prefix
            else if (String::EqualN(outPath.Data(), UNCPrefix.Data(), UNCPrefix.Size()))
            {
                outPath.Erase(2, 6);
            }
        }
        else
        {
            // result is in the NT namespace, so apply the DOS to NT namespace prefix
            outPath.Insert(0, "\\\\?\\GLOBALROOT");
        }

        return Result::Success;
    }
}

#endif
