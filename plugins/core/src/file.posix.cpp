// Copyright Chad Engler

#include "he/core/file.h"

#include "he/core/allocator.h"
#include "he/core/assert.h"
#include "he/core/macros.h"
#include "he/core/path.h"
#include "he/core/scope_guard.h"
#include "he/core/string.h"
#include "he/core/system.h"
#include "he/core/utils.h"

#include "fmt/core.h"

#include <limits>

#if defined(HE_PLATFORM_API_POSIX) && !defined(HE_PLATFORM_EMSCRIPTEN)

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

    Result File::Open(const char* path, FileOpenMode mode, FileOpenFlag openFlags)
    {
        if (!HE_VERIFY(m_fd == -1))
            return Result::InvalidParameter;

        m_fd = PosixFileOpen(path, mode, openFlags, 0);

        if (m_fd == -1)
            return Result::FromLastError();

        if (mode == FileOpenMode::WriteAppend || mode == FileOpenMode::ReadWriteAppend)
            lseek(m_fd, 0, SEEK_END);

        return Result::Success;
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

    MemoryMap::MemoryMap() noexcept
        : m_data(nullptr)
        , m_size(0)
    {}

    MemoryMap::MemoryMap(MemoryMap&& x) noexcept
        : m_data(Exchange(x.m_data, nullptr))
        , m_size(Exchange(x.m_size, 0))
    {}

    MemoryMap::~MemoryMap() noexcept
    {
        Unmap();
    }

    MemoryMap& MemoryMap::operator=(MemoryMap&& x) noexcept
    {
        Unmap();
        m_data = Exchange(x.m_data, nullptr);
        m_size = Exchange(x.m_size, 0);
        return *this;
    }

    Result MemoryMap::Map(const File& file, MemoryMapMode mode, uint64_t offset, uint32_t size)
    {
        HE_ASSERT(m_data == nullptr);
        HE_ASSERT(IsAligned(offset, GetSystemInfo().pageSize), HE_KV(offset, offset), HE_KV(page_size, GetSystemInfo().pageSize));

        if (!file.IsOpen())
            return Result::InvalidParameter;

        if (size == 0)
        {
            const uint64_t fileSize = file.GetSize();
            HE_ASSERT(fileSize <= std::numeric_limits<uint32_t>::max());
            size = static_cast<uint32_t>(file.GetSize());
        }

        int prot = PROT_READ;

        if (mode == MemoryMapMode::ReadWrite)
            prot |= PROT_WRITE;

        int flags = MAP_SHARED | MAP_FILE;

        m_data = mmap(nullptr, size, prot, flags, file.m_fd, offset);

        if (m_data == MAP_FAILED)
        {
            Result r = Result::FromLastError();
            Unmap();
            return r;
        }
        m_size = size;

    #ifdef MADV_DONTFORK
        if (madvise(m_data, m_size, MADV_DONTFORK) != 0)
            return Result::FromLastError();
    #endif

    #ifdef MADV_NOHUGEPAGE
        madvise(m_data, m_size, MADV_NOHUGEPAGE);
    #endif

        return Result::Success;
    }

    bool MemoryMap::IsMapped() const
    {
        return m_data != nullptr && m_data != MAP_FAILED;
    }

    void MemoryMap::Unmap()
    {
        if (IsMapped())
            munmap(m_data, m_size);

        m_data = nullptr;
        m_size = 0;
    }

    Result MemoryMap::Flush(uint64_t offset, uint32_t size, bool async)
    {
        HE_ASSERT(IsMapped());

        uint8_t* ptr = static_cast<uint8_t*>(m_data) + offset;
        const int flags = async ? MS_ASYNC : MS_SYNC;
        const int rc = msync(ptr, size, flags);
        return rc ? Result::FromLastError() : Result::Success;
    }
}

#endif
