// Copyright Chad Engler

#include "he/core/file.h"

#include "he/core/allocator.h"
#include "he/core/ascii.h"
#include "he/core/assert.h"
#include "he/core/clock.h"
#include "he/core/enum_ops.h"
#include "he/core/macros.h"
#include "he/core/memory_ops.h"
#include "he/core/scope_guard.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/wstr.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "file_helpers.win32.h"

#include "he/core/win32_min.h"

#include <fileapi.h>

namespace he
{
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
        : m_fd(Win32InvalidFd)
    {}

    File::File(File&& x)
        : m_fd(Exchange(x.m_fd, Win32InvalidFd))
    {}

    File::~File()
    {
        Close();
    }

    File& File::operator=(File&& x)
    {
        Close();
        m_fd = Exchange(x.m_fd, Win32InvalidFd);
        return *this;
    }

    Result File::Open(const char* path, FileOpenMode mode, FileOpenFlag flags)
    {
        HE_ASSERT(m_fd == Win32InvalidFd);
        HANDLE handle = Win32FileOpen(path, mode, flags, 0);

        if (handle == INVALID_HANDLE_VALUE)
            return Result::FromLastError();

        if (mode == FileOpenMode::WriteAppend || mode == FileOpenMode::ReadWriteAppend)
            ::SetFilePointer(handle, 0, nullptr, FILE_END);

        m_fd = reinterpret_cast<intptr_t>(handle);

        return Result::Success;
    }

    void File::Close()
    {
        if (m_fd != Win32InvalidFd)
        {
            const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);
            ::CloseHandle(handle);
            m_fd = Win32InvalidFd;
        }
    }

    bool File::IsOpen() const
    {
        return m_fd != Win32InvalidFd;
    }

    uint64_t File::GetSize() const
    {
        const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);

        LARGE_INTEGER fileSize;
        if (!::GetFileSizeEx(handle, &fileSize))
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

        const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);

        if (!::SetEndOfFile(handle))
            return Result::FromLastError();

        if (size > curFileSize)
        {
            // Writing a zero byte to the end of the expanded file forces Windows to zero out
            // all the new space in the file. This can take non-trivial time for large files,
            // but makes the behavior of this file consistent with the posix ftruncate impl.
            // See: https://devblogs.microsoft.com/oldnewthing/20110922-00/?p=9573
            BYTE b = 0;
            return WriteAt(&b, size - 1, 1);
        }

        return Result::Success;
    }

    uint64_t File::GetPos() const
    {
        const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);

        LARGE_INTEGER distanceToMove;
        LARGE_INTEGER result;
        distanceToMove.QuadPart = 0;
        ::SetFilePointerEx(handle, distanceToMove, &result, FILE_CURRENT);
        return result.QuadPart;
    }

    Result File::SetPos(uint64_t offset)
    {
        const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);

        LARGE_INTEGER distanceToMove;
        distanceToMove.QuadPart = offset;
        if (!::SetFilePointerEx(handle, distanceToMove, nullptr, FILE_BEGIN))
            return Result::FromLastError();
        return Result::Success;
    }

    Result File::Read(void* dst, uint32_t bytesToRead, uint32_t* bytesRead)
    {
        const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);

        DWORD dwBytesRead;
        if (!::ReadFile(handle, dst, bytesToRead, &dwBytesRead, nullptr))
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

    Result File::ReadAt(void* dst, uint64_t offset, uint32_t size, uint32_t* bytesRead)
    {
        const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);

        OVERLAPPED o{};
        o.Offset = (DWORD)offset;
        o.OffsetHigh = (DWORD)(offset >> 32);

        DWORD dwBytesRead;
        if (!::ReadFile(handle, dst, size, &dwBytesRead, &o))
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
        const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);

        DWORD dwBytesWritten;
        if (!::WriteFile(handle, src, bytesToWrite, &dwBytesWritten, nullptr))
        {
            if (bytesWritten)
                *bytesWritten = 0;
            return Result::FromLastError();
        }

        if (bytesWritten)
            *bytesWritten = dwBytesWritten;
        return Result::Success;
    }

    Result File::WriteAt(const void* src, uint64_t offset, uint32_t size, uint32_t* bytesWritten)
    {
        const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);

        OVERLAPPED o{};
        o.Offset = (DWORD)offset;
        o.OffsetHigh = (DWORD)(offset >> 32);

        DWORD dwBytesWritten;
        if (!::WriteFile(handle, src, size, &dwBytesWritten, &o))
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
        const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);

        if (!::FlushFileBuffers(handle))
            return Result::FromLastError();

        return Result::Success;
    }

    Result File::Lock(uint64_t offset, uint64_t size, FileLockFlag flags)
    {
        const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);

        DWORD dwFlags = 0;

        if (HasFlag(flags, FileLockFlag::Exclusive))
            dwFlags |= LOCKFILE_EXCLUSIVE_LOCK;

        if (HasFlag(flags, FileLockFlag::NonBlocking))
            dwFlags |= LOCKFILE_FAIL_IMMEDIATELY;

        OVERLAPPED o;
        MemZero(&o, sizeof(o));
        o.Offset = (DWORD)offset;
        o.OffsetHigh = (DWORD)(offset >> 32);

        const DWORD sizeLow = (DWORD)size;
        const DWORD sizeHigh = (DWORD)(size >> 32);

        const BOOL rc = ::LockFileEx(handle, dwFlags, 0, sizeLow, sizeHigh, &o);

        return rc ? Result::Success : Result::FromLastError();
    }

    Result File::Unlock(uint64_t offset, uint64_t size)
    {
        const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);

        const DWORD offsetLow = (DWORD)offset;
        const DWORD offsetHigh = (DWORD)(offset >> 32);

        const DWORD sizeLow = (DWORD)size;
        const DWORD sizeHigh = (DWORD)(size >> 32);

        const BOOL rc = ::UnlockFile(handle, offsetLow, offsetHigh, sizeLow, sizeHigh);

        return rc ? Result::Success : Result::FromLastError();
    }

    Result File::GetAttributes(FileAttributes& outAttributes) const
    {
        const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);

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
        const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);

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

            wchar_t* buf = path.GetAllocator().Malloc<wchar_t>(wideRequiredLen);
            HE_AT_SCOPE_EXIT([&]()
            {
                path.GetAllocator().Free(buf);
            });

            DWORD widePathLen = ::GetFinalPathNameByHandleW(handle, buf, wideRequiredLen, flags);
            if (widePathLen == 0 || widePathLen > wideRequiredLen)
                return Result::FromLastError();

            WCToMBStr(path, buf);
            break;
        }

        if (flags == VOLUME_NAME_DOS)
        {
            constexpr StringView Prefix = "\\\\?\\";
            constexpr StringView UNCPrefix = "\\\\?\\UNC\\";

            // Check if path starts with a \\?\X: prefix, and if so remove the \\?\ prefix
            if (path.Size() >= 6
                && String::EqualN(path.Data(), Prefix.Data(), Prefix.Size())
                && IsAlpha(path[4])
                && path[5] == ':'
                && path[6] == '\\')
            {
                path.Erase(0, Prefix.Size());
            }
            // Check if path starts with a \\?\UNC\ prefix, and if so replace it with simpler \\ prefix
            else if (String::EqualN(path.Data(), UNCPrefix.Data(), UNCPrefix.Size()))
            {
                path.Erase(2, 6);
            }
        }
        else
        {
            // result is in the NT namespace, so apply the DOS to NT namespace prefix
            path.Insert(0, "\\\\?\\GLOBALROOT");
        }

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

    MemoryMap::MemoryMap()
        : m_data(nullptr)
        , m_size(0)
        , m_handle(nullptr)
        , m_fileHandle(INVALID_HANDLE_VALUE)
    {}

    MemoryMap::MemoryMap(MemoryMap&& x)
        : m_data(Exchange(x.m_data, nullptr))
        , m_size(Exchange(x.m_size, 0))
        , m_handle(Exchange(x.m_handle, nullptr))
        , m_fileHandle(Exchange(x.m_fileHandle, INVALID_HANDLE_VALUE))
    {}

    MemoryMap::~MemoryMap()
    {
        Unmap();
    }

    MemoryMap& MemoryMap::operator=(MemoryMap&& x)
    {
        Unmap();
        m_data = Exchange(x.m_data, nullptr);
        m_size = Exchange(x.m_size, 0);
        m_handle = Exchange(x.m_handle, nullptr);
        m_fileHandle = Exchange(x.m_fileHandle, INVALID_HANDLE_VALUE);
        return *this;
    }

    Result MemoryMap::Map(File& file, MemoryMapMode mode, uint64_t offset, uint32_t size)
    {
        HE_ASSERT(m_handle == nullptr && m_fileHandle == INVALID_HANDLE_VALUE);
        // TODO: Validate offset is multiple of allocation granularity

        const HANDLE fileHandle = reinterpret_cast<HANDLE>(file.m_fd);

        DWORD prot = mode == MemoryMapMode::Read ? PAGE_READONLY : PAGE_READWRITE;

        m_handle = ::CreateFileMappingW(fileHandle, nullptr, prot, 0, (DWORD)size, nullptr);

        if (m_handle == nullptr)
            return Result::FromLastError();

        DWORD access = mode == MemoryMapMode::Read ? FILE_MAP_READ : FILE_MAP_WRITE;

        m_data = ::MapViewOfFile(m_handle, access, (DWORD)(offset >> 32), (DWORD)offset, size);

        if (m_data == nullptr)
        {
            Result r = Result::FromLastError();
            Unmap();
            return r;
        }

        m_size = size;

        // Allows the memory map to keep the file 'open'
        ::DuplicateHandle(::GetCurrentProcess(), fileHandle, ::GetCurrentProcess(), &m_fileHandle, 0, FALSE, DUPLICATE_SAME_ACCESS);

        return Result::Success;
    }

    bool MemoryMap::IsMapped() const
    {
        return m_data != nullptr;
    }

    void MemoryMap::Unmap()
    {
        if (m_data != nullptr)
            ::UnmapViewOfFile(m_data);

        if (m_handle != nullptr)
            ::CloseHandle(m_handle);

        if (m_fileHandle != INVALID_HANDLE_VALUE)
            ::CloseHandle(m_fileHandle);

        m_data = nullptr;
        m_size = 0;
        m_handle = nullptr;
        m_fileHandle = INVALID_HANDLE_VALUE;
    }

    Result MemoryMap::Flush(uint64_t offset, uint32_t size, bool async)
    {
        HE_ASSERT(m_handle != nullptr && m_fileHandle != INVALID_HANDLE_VALUE);

        uint8_t* ptr = static_cast<uint8_t*>(m_data) + offset;

        if (::FlushViewOfFile(ptr, size) && (async || ::FlushFileBuffers(m_fileHandle)))
            return Result::Success;

        return Result::FromLastError();
    }
}

#endif
