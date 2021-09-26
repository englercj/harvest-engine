// Copyright Chad Engler

#include "he/core/file.h"

#include "he/core/allocator.h"
#include "he/core/assert.h"
#include "he/core/clock.h"
#include "he/core/macros.h"
#include "he/core/memory_ops.h"
#include "he/core/scope_guard.h"
#include "he/core/string.h"
#include "he/core/wstr.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

#include <fileapi.h>

namespace he
{
    constexpr intptr_t InvalidFd = reinterpret_cast<intptr_t>(INVALID_HANDLE_VALUE);

    FileResult GetFileResult(Result result)
    {
        switch (result.GetCode())
        {
            case ERROR_SUCCESS:         return FileResult::Success;
            case ERROR_DISK_FULL:       return FileResult::DiskFull;
            case ERROR_ACCESS_DENIED:   return FileResult::AccessDenied;
            case ERROR_ALREADY_EXISTS:  return FileResult::AlreadyExists;
            case ERROR_FILE_NOT_FOUND:  return FileResult::NotFound;
            case ERROR_PATH_NOT_FOUND:  return FileResult::NotFound;
            case ERROR_NO_DATA:         return FileResult::NoData;
            default:
                return FileResult::Failure;
        }
    }

    bool File::Exists(const char* path)
    {
        DWORD attr = ::GetFileAttributesW(HE_TO_WSTR(path));
        return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
    }

    Result File::Remove(const char* path)
    {
        if (!::DeleteFileW(HE_TO_WSTR(path)))
            return Result::FromLastError();
        return Result::Success;
    }

    Result File::Rename(const char* oldPath, const char* newPath)
    {
        if (!::MoveFileW(HE_TO_WSTR(oldPath), HE_TO_WSTR(newPath)))
            return Result::FromLastError();
        return Result::Success;
    }

    Result File::Copy(const char* oldPath, const char* newPath, bool clobber)
    {
        if (!::CopyFileW(HE_TO_WSTR(oldPath), HE_TO_WSTR(newPath), !clobber))
            return Result::FromLastError();
        return Result::Success;
    }

    Result File::GetAttributes(const char* path, FileAttributes& outAttributes)
    {
        WIN32_FILE_ATTRIBUTE_DATA attrData{};
        if (!::GetFileAttributesExW(HE_TO_WSTR(path), GetFileExInfoStandard, &attrData))
            return Result::FromLastError();

        outAttributes.flags = FileAttributeFlag::None;

        if ((attrData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == FILE_ATTRIBUTE_HIDDEN)
        {
            outAttributes.flags |= FileAttributeFlag::Hidden;
        }

        if ((attrData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) == FILE_ATTRIBUTE_READONLY)
        {
            outAttributes.flags |= FileAttributeFlag::ReadOnly;
        }

        if ((attrData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
        {
            outAttributes.flags |= FileAttributeFlag::Directory;
            outAttributes.size = 0;
        }
        else
        {
            ULARGE_INTEGER size;
            size.HighPart = attrData.nFileSizeHigh;
            size.LowPart = attrData.nFileSizeLow;
            outAttributes.size = size.QuadPart;
        }

        ULARGE_INTEGER i;

        i.HighPart = attrData.ftCreationTime.dwHighDateTime;
        i.LowPart = attrData.ftCreationTime.dwLowDateTime;
        outAttributes.createTime = Win32FileTimeToSystemTime(i.QuadPart);

        i.HighPart = attrData.ftLastAccessTime.dwHighDateTime;
        i.LowPart = attrData.ftLastAccessTime.dwLowDateTime;
        outAttributes.accessTime = Win32FileTimeToSystemTime(i.QuadPart);

        i.HighPart = attrData.ftLastWriteTime.dwHighDateTime;
        i.LowPart = attrData.ftLastWriteTime.dwLowDateTime;
        outAttributes.writeTime = Win32FileTimeToSystemTime(i.QuadPart);

        return Result::Success;
    }

    File::File()
        : m_fd(InvalidFd)
    {}

    File::File(File&& x)
        : m_fd(Exchange(x.m_fd, InvalidFd))
    {}

    File::~File()
    {
        Close();
    }

    File& File::operator=(File&& x)
    {
        if (m_fd != InvalidFd)
            Close();
        m_fd = Exchange(x.m_fd, InvalidFd);
        return *this;
    }

    Result File::Open(const char* path, FileOpenMode mode, FileOpenFlag flags)
    {
        HE_ASSERT(m_fd == InvalidFd);

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
                return Result::InvalidParameter;
        }

        DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;

        if (HasFlags(flags, FileOpenFlag::NoBuffering))
            dwFlagsAndAttributes |= FILE_FLAG_NO_BUFFERING;

        if (HasFlags(flags, FileOpenFlag::RandomAccess))
            dwFlagsAndAttributes |= FILE_FLAG_RANDOM_ACCESS;
        else if (HasFlags(flags, FileOpenFlag::SequentialScan))
            dwFlagsAndAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;

        HANDLE handle = ::CreateFileW(
            HE_TO_WSTR(path),
            dwDesiredAccess,
            dwShareMode,
            nullptr,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            nullptr);

        if (handle == INVALID_HANDLE_VALUE)
            return Result::FromLastError();

        if (mode == FileOpenMode::WriteAppend || mode == FileOpenMode::ReadWriteAppend)
            ::SetFilePointer(handle, 0, nullptr, FILE_END);

        m_fd = reinterpret_cast<intptr_t>(handle);

        return Result::Success;
    }

    void File::Close()
    {
        if (m_fd != InvalidFd)
        {
            ::CloseHandle(reinterpret_cast<HANDLE>(m_fd));
            m_fd = InvalidFd;
        }
    }

    bool File::IsOpen() const
    {
        return m_fd != InvalidFd;
    }

    uint64_t File::GetSize() const
    {
        LARGE_INTEGER fileSize;
        if (!::GetFileSizeEx(reinterpret_cast<HANDLE>(m_fd), &fileSize))
            return 0;
        return fileSize.QuadPart;
    }

    Result File::SetSize(uint64_t size)
    {
        uint64_t curPos = GetPos();

        HE_AT_SCOPE_EXIT([&]()
        {
            SetPos(curPos);
        });

        const uint64_t curFileSize = GetSize();

        if (!SetPos(size))
            return Result::FromLastError();

        HANDLE handle = reinterpret_cast<HANDLE>(m_fd);

        if (!::SetEndOfFile(handle))
            return Result::FromLastError();

        if (size > curFileSize)
        {
            // Writing a zero byte to the end of the expanded file forces Windows to zero out
            // all the new space in the file. This can take non-trivial time for large files,
            // but makes the behavior of this file consistent with the posix ftruncate impl.
            // See: https://devblogs.microsoft.com/oldnewthing/20110922-00/?p=9573
            BYTE b = 0;
            return WriteAt(&b, 1, size - 1);
        }

        return Result::Success;
    }

    uint64_t File::GetPos() const
    {
        LARGE_INTEGER liDistanceToMove;
        LARGE_INTEGER liResult;
        liDistanceToMove.QuadPart = 0;
        ::SetFilePointerEx(reinterpret_cast<HANDLE>(m_fd), liDistanceToMove, &liResult, FILE_CURRENT);
        return liResult.QuadPart;
    }

    Result File::SetPos(uint64_t offset)
    {
        LARGE_INTEGER liDistanceToMove;
        liDistanceToMove.QuadPart = offset;
        if (!::SetFilePointerEx(reinterpret_cast<HANDLE>(m_fd), liDistanceToMove, nullptr, FILE_BEGIN))
            return Result::FromLastError();
        return Result::Success;
    }

    Result File::Read(void* dst, uint32_t bytesToRead, uint32_t* bytesRead)
    {
        DWORD dwBytesRead;
        if (!::ReadFile(reinterpret_cast<HANDLE>(m_fd), dst, bytesToRead, &dwBytesRead, nullptr))
        {
            if (bytesRead)
                *bytesRead = 0;
            DWORD lastError = ::GetLastError();
            return (lastError == ERROR_BROKEN_PIPE) ? Result::Success : Win32Result(lastError);
        }

        if (bytesRead)
            *bytesRead = dwBytesRead;
        return Result::Success;
    }

    Result File::ReadAt(void* dst, uint32_t bytesToRead, uint64_t offset, uint32_t* bytesRead)
    {
        OVERLAPPED o{};
        o.Offset = (DWORD)offset;
        o.OffsetHigh = (DWORD)(offset >> 32);

        DWORD dwBytesRead;
        if (!::ReadFile(reinterpret_cast<HANDLE>(m_fd), dst, bytesToRead, &dwBytesRead, &o))
        {
            if (::GetLastError() != ERROR_HANDLE_EOF)
            {
                if (bytesRead)
                    *bytesRead = 0;
                return Result::FromLastError();
            }
        }

        if (bytesRead)
            *bytesRead = dwBytesRead;
        return Result::Success;
    }

    Result File::Write(const void* src, uint32_t bytesToWrite, uint32_t* bytesWritten)
    {
        DWORD dwBytesWritten;
        if (!::WriteFile(reinterpret_cast<HANDLE>(m_fd), src, bytesToWrite, &dwBytesWritten, nullptr))
        {
            if (bytesWritten)
                *bytesWritten = 0;
            return Result::FromLastError();
        }

        if (bytesWritten)
            *bytesWritten = dwBytesWritten;
        return Result::Success;
    }

    Result File::WriteAt(const void* src, uint32_t bytesToWrite, uint64_t offset, uint32_t* bytesWritten)
    {
        OVERLAPPED o{};
        o.Offset = (DWORD)offset;
        o.OffsetHigh = (DWORD)(offset >> 32);

        DWORD dwBytesWritten;
        if (!::WriteFile(reinterpret_cast<HANDLE>(m_fd), src, bytesToWrite, &dwBytesWritten, &o))
        {
            if (bytesWritten)
                *bytesWritten = 0;
            return Result::FromLastError();
        }

        if (bytesWritten)
            *bytesWritten = dwBytesWritten;
        return Result::Success;
    }

    Result File::Flush()
    {
        if (!::FlushFileBuffers(reinterpret_cast<HANDLE>(m_fd)))
            return Result::FromLastError();

        return Result::Success;
    }

    Result File::GetAttributes(FileAttributes& outAttributes) const
    {
        HANDLE handle = reinterpret_cast<HANDLE>(m_fd);

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

    Result File::GetPath(String& path) const
    {
        HANDLE handle = reinterpret_cast<HANDLE>(m_fd);
        DWORD wideRequiredLen = ::GetFinalPathNameByHandleW(handle, nullptr, 0, FILE_NAME_OPENED);
        if (wideRequiredLen == 0)
            return Result::FromLastError();

        wchar_t* buf = path.GetAllocator().Malloc<wchar_t>(wideRequiredLen);
        HE_AT_SCOPE_EXIT([&]()
        {
            path.GetAllocator().Free(buf);
        });

        DWORD widePathLen = ::GetFinalPathNameByHandleW(handle, buf, wideRequiredLen, FILE_NAME_OPENED);
        if (widePathLen == 0 || widePathLen > wideRequiredLen)
            return Result::FromLastError();

        WCToMBStr(path, buf);

        return Result::Success;
    }

    Result File::SetTimes(const SystemTime* accessTime, const SystemTime* writeTime)
    {
        FILETIME win32AccessTime{};
        if (accessTime != nullptr)
        {
            auto fileTime = Win32FileTimeFromSystemTime(*accessTime);
            win32AccessTime = { DWORD(fileTime), DWORD(fileTime >> 32) };
        }

        FILETIME win32WriteTime{};
        if (writeTime != nullptr)
        {
            auto fileTime = Win32FileTimeFromSystemTime(*writeTime);
            win32WriteTime = { DWORD(fileTime), DWORD(fileTime >> 32) };
        }

        // FILETIME values set to zero are ignored, so there's no need to pass nullptr for anything.
        if (!::SetFileTime(reinterpret_cast<HANDLE>(m_fd), nullptr, &win32AccessTime, &win32WriteTime))
            return Result::FromLastError();

        return Result::Success;
    }
}

#endif
