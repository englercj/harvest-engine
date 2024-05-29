// Copyright Chad Engler

#include "he/core/file.h"

#include "he/core/allocator.h"
#include "he/core/assert.h"
#include "he/core/limits.h"
#include "he/core/macros.h"
#include "he/core/path.h"
#include "he/core/scope_guard.h"
#include "he/core/string.h"
#include "he/core/system.h"
#include "he/core/utils.h"

#if defined(HE_PLATFORM_API_POSIX)

#include "file_helpers.posix.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>

namespace he
{
    FileResult GetFileResult(Result result)
    {
        // EAGAIN and EWOULDBLOCK are the same on most modern *nix platforms
        static_assert(EAGAIN == EWOULDBLOCK, "Expected EGAIN and EWOULDBLOCK to be the same value.");

        switch (static_cast<int>(result.GetCode()))
        {
            case 0:         return FileResult::Success;
            case ENOSPC:    return FileResult::DiskFull;
            case EACCES:    return FileResult::AccessDenied;
            case EEXIST:    return FileResult::AlreadyExists;
            case ENOENT:    return FileResult::NotFound;
            case EAGAIN:    return FileResult::NoData;
            default:
                return FileResult::Failure;
        }
    }

    bool File::Exists(const char* path)
    {
        struct stat sb;
        return !stat(path, &sb) && S_ISREG(sb.st_mode);
    }

    Result File::Remove(const char* path)
    {
        if (unlink(path))
            return Result::FromLastError();
        return Result::Success;
    }

    Result File::Rename(const char* oldPath, const char* newPath)
    {
        if (rename(oldPath, newPath))
            return Result::FromLastError();
        return Result::Success;
    }

    Result File::Copy(const char* oldPath, const char* newPath, bool clobber)
    {
        if (!clobber && File::Exists(newPath))
            return Result::InvalidParameter;

        Result result = Result::Success;

        File oldFile;
        result = oldFile.Open(oldPath, FileOpenMode::ReadExisting);
        if (!result)
            return result;

        File newFile;
        result = newFile.Open(newPath, FileOpenMode::WriteTruncate);
        if (!result)
            return result;

        uint64_t oldFileSize = oldFile.GetSize();

        constexpr uint32_t BufferSize = 65536;
        void* buffer = Allocator::GetDefault().Malloc(BufferSize);
        uint32_t amountRead = 0;
        uint64_t totalRead = 0;
        while (totalRead < oldFileSize)
        {
            result = oldFile.Read(buffer, BufferSize, &amountRead);
            if (!result || amountRead == 0)
                break;

            uint32_t amountWritten = 0;
            result = newFile.Write(buffer, amountRead, &amountWritten);
            if (!result || amountWritten == 0)
                break;

            totalRead += amountRead;
        }

        Allocator::GetDefault().Free(buffer);
        return result;
    }

    Result File::GetAttributes(const char* path, FileAttributes& outAttributes)
    {
        struct stat sb;
        if (stat(path, &sb))
            return Result::FromLastError();

        PosixFileGatherAttributes(sb, path, outAttributes);
        return Result::Success;
    }

    File::File() noexcept
        : m_fd(-1)
    {}

    File::File(File&& x) noexcept
        : m_fd(Exchange(x.m_fd, -1))
    {}

    File::~File() noexcept
    {
        Close();
    }

    File& File::operator=(File&& x) noexcept
    {
        Close();
        m_fd = Exchange(x.m_fd, -1);
        return *this;
    }

    Result File::Open(const char* path, FileAccessMode access, FileCreateMode create, FileOpenFlag openFlags)
    {
        if (!HE_VERIFY(m_fd == -1))
            return Result::InvalidParameter;

        m_fd = PosixFileOpen(path, access, create, openFlags, 0);
        return m_fd == -1 ? Result::FromLastError() : Result::Success;
    }

    void File::Close()
    {
        if (m_fd != -1)
        {
            close(static_cast<int>(m_fd));
            m_fd = -1;
        }
    }

    bool File::IsOpen() const
    {
        return m_fd != -1;
    }

    uint64_t File::GetSize() const
    {
        struct stat buf;
        if (fstat(static_cast<int>(m_fd), &buf))
            return 0;
        return buf.st_size;
    }

    Result File::SetSize(uint64_t size)
    {
        if (ftruncate(static_cast<int>(m_fd), size))
            return Result::FromLastError();
        return Result::Success;
    }

    uint64_t File::GetPos() const
    {
        off_t offset = lseek(static_cast<int>(m_fd), 0, SEEK_CUR);
        return offset;
    }

    Result File::SetPos(uint64_t offset)
    {
        off_t rc = lseek(static_cast<int>(m_fd), offset, SEEK_SET);
        if (rc < 0)
            return Result::FromLastError();
        return Result::Success;
    }

    Result File::Read(void* dst, uint32_t bytesToRead, uint32_t* bytesRead)
    {
        ssize_t n = read(static_cast<int>(m_fd), dst, bytesToRead);
        if (n < 0)
        {
            if (bytesRead)
                *bytesRead = 0;
            return Result::FromLastError();
        }

        if (bytesRead)
            *bytesRead = static_cast<uint32_t>(n);
        return Result::Success;
    }

    Result File::ReadAt(void* dst, uint64_t offset, uint32_t size, uint32_t* bytesRead)
    {
        ssize_t n = pread(static_cast<int>(m_fd), dst, size, offset);
        if (n < 0)
        {
            if (bytesRead)
                *bytesRead = 0;
            return Result::FromLastError();
        }

        if (bytesRead)
            *bytesRead = static_cast<uint32_t>(n);
        return Result::Success;
    }

    Result File::Write(const void* src, uint32_t size, uint32_t* bytesWritten)
    {
        ssize_t n = write(static_cast<int>(m_fd), src, size);
        if (n < 0)
        {
            if (bytesWritten)
                *bytesWritten = 0;
            return Result::FromLastError();
        }

        if (bytesWritten)
            *bytesWritten = static_cast<uint32_t>(n);
        return Result::Success;
    }

    Result File::WriteAt(const void* src, uint64_t offset, uint32_t size, uint32_t* bytesWritten)
    {
        ssize_t n = pwrite(static_cast<int>(m_fd), src, size, offset);
        if (n < 0)
        {
            if (bytesWritten)
                *bytesWritten = 0;
            return Result::FromLastError();
        }

        if (bytesWritten)
            *bytesWritten = static_cast<uint32_t>(n);
        return Result::Success;
    }

    Result File::Flush()
    {
        if (fsync(static_cast<int>(m_fd)))
            return Result::FromLastError();

        return Result::Success;
    }

    Result File::Lock(uint64_t offset, uint64_t size, FileLockFlag flags)
    {
        short type = F_RDLCK;
        if (HasFlag(flags, FileLockFlag::Exclusive))
            type = F_WRLCK;

        int cmd = F_SETLKW;
        if (HasFlag(flags, FileLockFlag::NonBlocking))
            cmd = F_SETLK;

        flock l;
        l.l_type = type;
        l.l_whence = SEEK_SET;
        l.l_start = offset;
        l.l_len = size;

        const int rc = fcntl(m_fd, cmd, &l);
        return rc ? PosixResult(rc) : Result::Success;
    }

    Result File::Unlock(uint64_t offset, uint64_t size)
    {
        flock l;
        l.l_type = F_UNLCK;
        l.l_whence = SEEK_SET;
        l.l_start = offset;
        l.l_len = size;

        const int rc = fcntl(m_fd, F_SETLKW, &l);
        return rc ? PosixResult(rc) : Result::Success;
    }

    Result File::GetAttributes(FileAttributes& outAttributes) const
    {
        return PosixFileGetAttributes(static_cast<int>(m_fd), outAttributes);
    }

    Result File::GetPath(String& path) const
    {
        return PosixFileGetPath(static_cast<int>(m_fd), path);
    }

    Result File::SetTimes(const SystemTime* accessTime, const SystemTime* writeTime)
    {
        struct timespec times[2]{};
        times[0].tv_nsec = UTIME_OMIT;
        times[1].tv_nsec = UTIME_OMIT;

        if (accessTime != nullptr)
            times[0] = PosixTimeFromSystemTime(*accessTime);
        if (writeTime != nullptr)
            times[1] = PosixTimeFromSystemTime(*writeTime);
        if (futimens(static_cast<int>(m_fd), times) != 0)
            return Result::FromLastError();

        return Result::Success;
    }

    MemoryMappedFile::MemoryMappedFile() noexcept
        : m_fileHandle(-1)
        , m_mappingHandle(0)
    {}

    MemoryMappedFile::MemoryMappedFile(MemoryMappedFile&& x) noexcept
        : m_fileHandle(Exchange(x.m_fileHandle, -1))
        , m_mappingHandle(Exchange(x.m_handle, 0))
    {}

    MemoryMappedFile::~MemoryMappedFile() noexcept
    {
        Close();
    }

    MemoryMappedFile& MemoryMappedFile::operator=(MemoryMappedFile&& x) noexcept
    {
        m_fileHandle = Exchange(x.m_fileHandle, -1);
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

    Result MemoryMappedFile::Open(const File& file, MemoryMapMode mode)
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

        m_fileHandle = dup(static_cast<int>(file.m_fd));
        m_fileSize = file.GetSize();
        m_accessMode = mode;

        return Result::Success;
    }

    void MemoryMappedFile::Close()
    {
    #if HE_ENABLE_ASSERTIONS
        HE_VERIFY(m_openRegionsCount == 0, HE_MSG("MemoryMappedFile closed with open regions. This is a leak."), HE_VAL(m_openRegionsCount));
    #endif

        if (m_fileHandle != -1)
        {
            close(static_cast<int>(m_fileHandle));
        }

        m_fileHandle = -1;
        m_fileSize = 0;

    #if HE_ENABLE_ASSERTIONS
        m_openRegionsCount = 0;
    #endif
    }

    bool MemoryMappedFile::IsOpen() const
    {
        return m_fileHandle != -1;
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

        size = Min(size, static_cast<uint32_t>(m_fileSize - offset));
        if (!HE_VERIFY(size > 0, HE_MSG("Cannot map zero bytes of a MemoryMappedFile")))
        {
            return Result::InvalidParameter;
        }

        int access = 0;
        switch (m_accessMode)
        {
            case FileAccessMode::Read:
                access = PROT_READ;
                break;
            case FileAccessMode::Write:
                access = PROT_WRITE;
                break;
            case FileAccessMode::ReadWrite:
                access = PROT_READ | PROT_WRITE;
                break;
        }

        if (!HE_VERIFY(access != 0))
        {
            return Result::InvalidParameter;
        }

        int flags = MAP_FILE;

        if (HasFlag(access, PROT_WRITE))
        {
            flags |= MAP_SHARED;
        }
        else
        {
            flags |= MAP_PRIVATE;
        }

        if (preload)
        {
            flags |= MAP_POPULATE;
        }

        const uint32_t pageSize = GetSystemInfo().pageSize;
        const uint64_t alignedOffset = AlignDown(offset, pageSize);
        const uint64_t alignedSize = AlignUp(size + (offset - alignedOffset), pageSize);

        if (!HE_VERIFY(alignedSize <= Limits<uint32_t>::Max,
            HE_MSG("Region size is too large for a 32-bit size value."),
            HE_VAL(offset),
            HE_VAL(size),
            HE_VAL(alignedOffset),
            HE_VAL(alignedSize)))
        {
            return Result::InvalidParameter;
        }

        region.alignedData = mmap(nullptr, static_cast<size_t>(alignedSize), access, flags, static_cast<int>(m_fileHandle), alignedOffset);

        if (region.alignedData == MAP_FAILED || region.alignedData == nullptr)
            return Result::FromLastError();

        region.alignedSize = static_cast<uint32_t>(alignedSize);
        region.data = static_cast<uint8_t*>(region.alignedData) + (offset - alignedOffset);
        region.size = size;

    #if HE_ENABLE_ASSERTIONS
        region.parentHandle = m_fileHandle;
        ++m_openRegionsCount;
    #endif

    #if defined(MADV_DONTFORK)
        madvise(region.alignedData, region.alignedSize, MADV_DONTFORK);
    #endif

    // TODO: Need to test the effect of avoiding huge pages and see if that's something we want to do.
    #if defined(MADV_NOHUGEPAGE) && 0
        madvise(region.alignedData, region.alignedSize, MADV_NOHUGEPAGE);
    #endif

        return Result::Success;
    }

    Result MemoryMappedFile::UnmapRegion(MemoryMappedRegion& region)
    {
        if (!HE_VERIFY(IsOpen(), HE_MSG("MemoryMappedFile is not open")))
        {
            return Result::InvalidParameter;
        }

    #if HE_ENABLE_ASSERTIONS
        if (!HE_VERIFY(region.parentHandle == m_fileHandle, HE_MSG("MemoryMappedRegion is not from this MemoryMappedFile")))
        {
            return Result::InvalidParameter;
        }
    #endif

        if (region.alignedData != nullptr)
        {
            const int rc = munmap(region.alignedData, region.alignedSize);
            if (rc != 0)
            {
                return Result::FromLastError();
            }
        }

        region.data = nullptr;
        region.size = 0;
        region.alignedData = nullptr;
        region.alignedSize = 0;

    #if HE_ENABLE_ASSERTIONS
        region.parentHandle = nullptr;

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
        if (!HE_VERIFY(region.parentHandle == m_fileHandle, HE_MSG("MemoryMappedRegion is not from this MemoryMappedFile")))
        {
            return Result::InvalidParameter;
        }
    #endif

        // The pointer for msync must be page aligned, so we need to fixup our pointer and sync
        // size to accommodate.
        const uint32_t pageSize = GetSystemInfo().pageSize;
        uint8_t* ptr = static_cast<uint8_t*>(region.data) + offset;
        uint8_t* alignedPtr = AlignDown(ptr, pageSize);
        HE_ASSERT(alignedPtr >= static_cast<uint8_t*>(region.alignedData));

        if (!HE_VERIFY(alignedPtr < static_cast<uint8_t*>(region.alignedData) + region.alignedSize,
            HE_MSG("Offset is beyond the end of the region"),
            HE_VAL(offset),
            HE_VAL(size),
            HE_KV(region_size, region.size)))
        {
            return Result::InvalidParameter;
        }

        const uint64_t alignedSize = size + (ptr - alignedPtr);
        if (!HE_VERIFY(alignedPtr + alignedSize <= static_cast<uint8_t*>(region.alignedData) + region.alignedSize,
            HE_MSG("Offset + size is beyond the end of the region"),
            HE_VAL(offset),
            HE_VAL(size),
            HE_KV(region_size, region.size)))
        {
            return Result::InvalidParameter;
        }

        const int flags = async ? MS_ASYNC : MS_SYNC;
        const int rc = msync(alignedPtr, alignedSize, flags);
        return rc ? Result::FromLastError() : Result::Success;
    }
}

#endif
