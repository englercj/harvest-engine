// Copyright Chad Engler

#include "he/core/file.h"

#include "he/core/allocator.h"
#include "he/core/ascii.h"
#include "he/core/assert.h"
#include "he/core/clock.h"
#include "he/core/macros.h"
#include "he/core/memory_ops.h"
#include "he/core/scope_guard.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/system.h"
#include "he/core/utils.h"
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
        DWORD attr = ::GetFileAttributesW(HE_TO_WCSTR(path));
        return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
    }

    Result File::Remove(const char* path)
    {
        if (!::DeleteFileW(HE_TO_WCSTR(path)))
            return Result::FromLastError();
        return Result::Success;
    }

    Result File::Rename(const char* oldPath, const char* newPath)
    {
        if (!::MoveFileW(HE_TO_WCSTR(oldPath), HE_TO_WCSTR(newPath)))
            return Result::FromLastError();
        return Result::Success;
    }

    Result File::Copy(const char* oldPath, const char* newPath, bool clobber)
    {
        if (!::CopyFileW(HE_TO_WCSTR(oldPath), HE_TO_WCSTR(newPath), !clobber))
            return Result::FromLastError();
        return Result::Success;
    }

    Result File::GetAttributes(const char* path, FileAttributes& outAttributes)
    {
        WIN32_FILE_ATTRIBUTE_DATA attrData{};
        if (!::GetFileAttributesExW(HE_TO_WCSTR(path), GetFileExInfoStandard, &attrData))
            return Result::FromLastError();

        Win32ParseFileAttributes(attrData, outAttributes);
        return Result::Success;
    }

    File::File() noexcept
        : m_fd(Win32InvalidFd)
    {}

    File::File(File&& x) noexcept
        : m_fd(Exchange(x.m_fd, Win32InvalidFd))
    {}

    File::~File() noexcept
    {
        Close();
    }

    File& File::operator=(File&& x) noexcept
    {
        Close();
        m_fd = Exchange(x.m_fd, Win32InvalidFd);
        return *this;
    }

    Result File::Open(const char* path, FileAccessMode access, FileCreateMode create, FileOpenFlag flags)
    {
        if (!HE_VERIFY(m_fd == Win32InvalidFd))
            return Result::InvalidParameter;

        HANDLE handle = Win32FileOpen(path, access, create, flags, 0);

        if (handle == INVALID_HANDLE_VALUE)
            return Result::FromLastError();

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

        const DWORD sizeLow = size == 0 ? MAXDWORD : (DWORD)size;
        const DWORD sizeHigh = size == 0 ? MAXDWORD : (DWORD)(size >> 32);

        const BOOL rc = ::LockFileEx(handle, dwFlags, 0, sizeLow, sizeHigh, &o);

        return rc ? Result::Success : Result::FromLastError();
    }

    Result File::Unlock(uint64_t offset, uint64_t size)
    {
        const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);

        const DWORD offsetLow = (DWORD)offset;
        const DWORD offsetHigh = (DWORD)(offset >> 32);

        const DWORD sizeLow = size == 0 ? MAXDWORD : (DWORD)size;
        const DWORD sizeHigh = size == 0 ? MAXDWORD : (DWORD)(size >> 32);

        const BOOL rc = ::UnlockFile(handle, offsetLow, offsetHigh, sizeLow, sizeHigh);

        return rc ? Result::Success : Result::FromLastError();
    }

    Result File::GetAttributes(FileAttributes& outAttributes) const
    {
        const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);
        return Win32FileGetAttributes(handle, outAttributes);
    }

    Result File::GetPath(String& outPath) const
    {
        const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);
        return Win32FileGetPath(handle, outPath);
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

    MemoryMappedFile::MemoryMappedFile() noexcept
        : m_fileHandle(Win32InvalidFd)
        , m_mappingHandle(0)
    {}

    MemoryMappedFile::MemoryMappedFile(MemoryMappedFile&& x) noexcept
        : m_fileHandle(Exchange(x.m_fileHandle, Win32InvalidFd))
        , m_mappingHandle(Exchange(x.m_mappingHandle, 0))
    {}

    MemoryMappedFile::~MemoryMappedFile() noexcept
    {
        Close();
    }

    MemoryMappedFile& MemoryMappedFile::operator=(MemoryMappedFile&& x) noexcept
    {
        m_fileHandle = Exchange(x.m_fileHandle, Win32InvalidFd);
        m_mappingHandle = Exchange(x.m_mappingHandle, 0);

        m_fileSize = Exchange(x.m_fileSize, 0);
        m_accessMode = Exchange(x.m_accessMode, FileAccessMode::Read);

    #if HE_ENABLE_ASSERTIONS
        HE_VERIFY(m_openRegionsCount == 0, HE_MSG("MemoryMappedFile begin moved into with open regions. This is a leak."), HE_VAL(m_openRegionsCount));
        m_openRegionsCount = Exchange(x.m_openRegionsCount, 0);
    #endif
        return *this;
    }

    Result MemoryMappedFile::Open(const char* path, FileAccessMode access, FileCreateMode create, FileOpenFlag flags)
    {
        File file;
        Result rc = file.Open(path, access, create, flags);
        if (rc != Result::Success)
            return rc;

        return Open(file, access);
    }

    Result MemoryMappedFile::Open(const File& file, FileAccessMode mode)
    {
        if (!file.IsOpen())
            return Result::InvalidParameter;

        if (!HE_VERIFY(!IsOpen(),
            HE_MSG("MemoryMappedFile is already open, will close first")))
        {
            Close();
        }

        if (!HE_VERIFY(mode != FileAccessMode::Write,
            HE_MSG("Files can not be memory mapped for write-only access")))
        {
            return Result::InvalidParameter;
        }

        const HANDLE fileHandle = reinterpret_cast<HANDLE>(file.m_fd);

        DWORD prot = 0;
        switch (mode)
        {
            case FileAccessMode::Read:
                prot = PAGE_READONLY;
                break;
            case FileAccessMode::Write:
                break;
            case FileAccessMode::ReadWrite:
                prot = PAGE_READWRITE;
                break;
        }

        if (!HE_VERIFY(prot != 0))
        {
            return Result::InvalidParameter;
        }

        HANDLE mappingHandle = ::CreateFileMappingW(fileHandle, nullptr, prot, 0, 0, nullptr);

        if (mappingHandle == nullptr)
            return Result::FromLastError();

        // Allows the memory map to keep the file 'open'
        HANDLE fileHandleDup = nullptr;
        ::DuplicateHandle(::GetCurrentProcess(), fileHandle, ::GetCurrentProcess(), &fileHandleDup, 0, FALSE, DUPLICATE_SAME_ACCESS);

        m_fileHandle = reinterpret_cast<intptr_t>(fileHandleDup);
        m_mappingHandle = reinterpret_cast<intptr_t>(mappingHandle);
        m_fileSize = file.GetSize();
        m_accessMode = mode;

        return Result::Success;
    }

    void MemoryMappedFile::Close()
    {
    #if HE_ENABLE_ASSERTIONS
        HE_VERIFY(m_openRegionsCount == 0, HE_MSG("MemoryMappedFile closed with open regions. This is a leak."), HE_VAL(m_openRegionsCount));
    #endif

        const HANDLE mappingHandle = reinterpret_cast<HANDLE>(m_mappingHandle);
        if (mappingHandle != nullptr)
        {
            ::CloseHandle(mappingHandle);
        }

        const HANDLE fileHandle = reinterpret_cast<HANDLE>(m_fileHandle);
        if (fileHandle != INVALID_HANDLE_VALUE)
        {
            ::CloseHandle(fileHandle);
        }

        m_fileHandle = Win32InvalidFd;
        m_mappingHandle = 0;
        m_fileSize = 0;

    #if HE_ENABLE_ASSERTIONS
        m_openRegionsCount = 0;
    #endif
    }

    bool MemoryMappedFile::IsOpen() const
    {
        return m_mappingHandle != 0 && m_fileHandle != Win32InvalidFd;
    }

    Result MemoryMappedFile::MapRegion(MemoryMappedRegion& region, uint64_t offset, uint32_t size, bool preload)
    {
        if (!HE_VERIFY(IsOpen(), HE_MSG("MemoryMappedFile is not open")))
        {
            return Result::InvalidParameter;
        }

        if (!HE_VERIFY(region.data == nullptr, HE_MSG("MemoryMappedRegion is already mapped")))
        {
            return Result::InvalidParameter;
        }

        if (!HE_VERIFY(offset < m_fileSize, HE_MSG("Offset is beyond the end of the file")))
        {
            return Result::InvalidParameter;
        }

        DWORD access = 0;
        switch (m_accessMode)
        {
            case FileAccessMode::Read:
                access = FILE_MAP_READ;
                break;
            case FileAccessMode::Write:
                access = FILE_MAP_WRITE;
                break;
            case FileAccessMode::ReadWrite:
                access = FILE_MAP_READ | FILE_MAP_WRITE;
                break;
        }

        if (!HE_VERIFY(access != 0))
        {
            return Result::InvalidParameter;
        }

        const uint32_t allocationGranularity = GetSystemInfo().allocationGranularity;
        const uint64_t alignedOffset = AlignDown(offset, allocationGranularity);
        const uint64_t alignedSize = AlignUp(size + (offset - alignedOffset), allocationGranularity);

        if (!HE_VERIFY(alignedSize <= Limits<uint32_t>::Max,
            HE_MSG("Region size is too large for a 32-bit size value."),
            HE_VAL(offset),
            HE_VAL(size),
            HE_KV(aligned_offset, alignedOffset),
            HE_KV(aligned_size, alignedSize)))
        {
            return Result::InvalidParameter;
        }

        ULARGE_INTEGER uliOffset;
        uliOffset.QuadPart = alignedOffset;

        HANDLE mappingHandle = reinterpret_cast<HANDLE>(m_mappingHandle);
        region.alignedData = ::MapViewOfFile(mappingHandle, access, uliOffset.HighPart, uliOffset.LowPart, alignedSize + alignedOffset > m_fileSize ? 0 : alignedSize);

        if (region.alignedData == nullptr)
            return Result::FromLastError();

        region.alignedSize = static_cast<uint32_t>(alignedSize);
        region.data = static_cast<uint8_t*>(region.alignedData) + (offset - alignedOffset);
        region.size = size;

    #if HE_ENABLE_ASSERTIONS
        region.parentHandle = m_mappingHandle;
        ++m_openRegionsCount;
    #endif

        if (preload)
        {
            WIN32_MEMORY_RANGE_ENTRY Range = { region.alignedData, region.alignedSize };
            ::PrefetchVirtualMemory(GetCurrentProcess(), 1, &Range, 0);
        }

        return Result::Success;
    }

    Result MemoryMappedFile::UnmapRegion(MemoryMappedRegion& region)
    {
        if (!HE_VERIFY(IsOpen(), HE_MSG("MemoryMappedFile is not open")))
        {
            return Result::InvalidParameter;
        }

    #if HE_ENABLE_ASSERTIONS
        if (!HE_VERIFY(region.parentHandle == m_mappingHandle, HE_MSG("MemoryMappedRegion is not from this MemoryMappedFile")))
        {
            return Result::InvalidParameter;
        }
    #endif

        if (region.alignedData != nullptr)
        {
            if (!::UnmapViewOfFile(region.alignedData))
                return Result::FromLastError();
        }

        region.data = nullptr;
        region.size = 0;
        region.alignedData = nullptr;
        region.alignedSize = 0;

    #if HE_ENABLE_ASSERTIONS
        region.parentHandle = 0;

        if (HE_VERIFY(m_openRegionsCount > 0, HE_MSG("MemoryMappedFile has no open regions to unmap.")))
        {
            --m_openRegionsCount;
        }
    #endif
        return Result::Success;
    }

    Result MemoryMappedFile::FlushRegion(MemoryMappedRegion& region, uint64_t offset, uint32_t size, bool async)
    {
        if (!HE_VERIFY(IsOpen(), HE_MSG("MemoryMappedFile is not open")))
        {
            return Result::InvalidParameter;
        }

        if (!HE_VERIFY(m_accessMode != FileAccessMode::Read, HE_MSG("MemoryMappedFile is read-only and cannot be flushed")))
        {
            return Result::InvalidParameter;
        }

        if (!HE_VERIFY(region.data != nullptr, HE_MSG("MemoryMappedRegion is not mapped")))
        {
            return Result::InvalidParameter;
        }

        if (!HE_VERIFY(offset < region.size,
            HE_MSG("Offset is beyond the end of the region"),
            HE_VAL(offset),
            HE_VAL(size),
            HE_KV(region_size, region.size)))
        {
            return Result::InvalidParameter;
        }

    #if HE_ENABLE_ASSERTIONS
        if (!HE_VERIFY(region.parentHandle == m_mappingHandle, HE_MSG("MemoryMappedRegion is not from this MemoryMappedFile")))
        {
            return Result::InvalidParameter;
        }
    #endif

        uint8_t* ptr = static_cast<uint8_t*>(region.data) + offset;
        HE_ASSERT(ptr >= static_cast<uint8_t*>(region.alignedData));

        if (!HE_VERIFY(ptr < static_cast<uint8_t*>(region.alignedData) + region.alignedSize,
            HE_MSG("Offset is beyond the end of the region"),
            HE_VAL(offset),
            HE_VAL(size),
            HE_KV(region_size, region.size)))
        {
            return Result::InvalidParameter;
        }

        if (!HE_VERIFY(ptr + size <= static_cast<uint8_t*>(region.alignedData) + region.alignedSize,
            HE_MSG("Offset + size is beyond the end of the region"),
            HE_VAL(offset),
            HE_VAL(size),
            HE_KV(region_size, region.size)))
        {
            return Result::InvalidParameter;
        }

        if (!::FlushViewOfFile(ptr, size))
            return Result::FromLastError();

        if (!async)
        {
            HANDLE fileHandle = reinterpret_cast<HANDLE>(m_fileHandle);
            if (!::FlushFileBuffers(fileHandle))
                return Result::FromLastError();
        }

        return Result::Success;
    }
}

#endif
