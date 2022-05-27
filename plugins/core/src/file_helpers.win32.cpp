// Copyright Chad Engler

#include "file_helpers.win32.h"

#include "he/core/ascii.h"
#include "he/core/scope_guard.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
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

    Result Win32FileGetAttributes(HANDLE handle, FileAttributes& outAttributes)
    {
        FILE_BASIC_INFO basicInfo{};
        if (!::GetFileInformationByHandleEx(handle, FileBasicInfo, &basicInfo, sizeof(FILE_BASIC_INFO)))
            return Result::FromLastError();

        FILE_STANDARD_INFO standardInfo{};
        if (!::GetFileInformationByHandleEx(handle, FileStandardInfo, &standardInfo, sizeof(FILE_STANDARD_INFO)))
            return Result::FromLastError();

        outAttributes.flags = FileAttributeFlag::None;

        if ((basicInfo.FileAttributes & FILE_ATTRIBUTE_HIDDEN) == FILE_ATTRIBUTE_HIDDEN)
        {
            outAttributes.flags |= FileAttributeFlag::Hidden;
        }

        if ((basicInfo.FileAttributes & FILE_ATTRIBUTE_READONLY) == FILE_ATTRIBUTE_READONLY)
        {
            outAttributes.flags |= FileAttributeFlag::ReadOnly;
        }

        if (standardInfo.Directory == TRUE)
        {
            outAttributes.flags |= FileAttributeFlag::Directory;
            outAttributes.size = 0;
        }
        else
        {
            outAttributes.size = standardInfo.EndOfFile.QuadPart;
        }

        outAttributes.createTime = Win32FileTimeToSystemTime(basicInfo.CreationTime.QuadPart);
        outAttributes.accessTime = Win32FileTimeToSystemTime(basicInfo.LastAccessTime.QuadPart);
        outAttributes.writeTime = Win32FileTimeToSystemTime(basicInfo.LastWriteTime.QuadPart);

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
