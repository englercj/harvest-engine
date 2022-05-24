// Copyright Chad Engler

#include "he/core/file.h"

#include "he/core/allocator.h"
#include "he/core/assert.h"
#include "he/core/enum_ops.h"
#include "he/core/macros.h"
#include "he/core/path.h"
#include "he/core/scope_guard.h"
#include "he/core/string.h"
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
    static void GatherFileAttributes(const struct stat& sb, const char* path, FileAttributes& attribs)
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

    static Result ReadLink(const char* linkPath, String& path)
    {
        // POSIX says we should be able to read the size of the link here
        // but linux isn't POSIX compliant in the /proc filesystem.
        // if (lstat(linkPath, &sb) == -1)
        //     return Result::FromLastError();

        // path.Resize(sb.st_size + 1);

        path.Resize(String::MaxEmbedCharacters, he::DefaultInit);

        do
        {
            ssize_t r = readlink(linkPath, path.Data(), path.Size());
            if (r < 0)
                return Result::FromLastError();

            if (r < path.Size())
            {
                // resize to properly null terminate
                path.Resize(r);
                return Result::Success;
            }

            path.Resize(he::Max(512u, path.Size() * 2));
        } while (true);

        return PosixResult(ENAMETOOLONG);
    }

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
        // TODO: REDO WITH BIG OL STRINGS FOR BUFFERS

        if (!clobber && File::Exists(newPath))
            return Result::InvalidParameter;

        File oldFile;
        Result oldResult = oldFile.Open(oldPath, FileOpenMode::ReadExisting);
        if (!oldResult)
        {
            return oldResult;
        }

        File newFile;
        Result newResult = newFile.Open(newPath, FileOpenMode::WriteTruncate);
        if (!newResult)
        {
            return newResult;
        }

        uint64_t oldFileSize = oldFile.GetSize();

        char buffer[4096];
        uint32_t amountRead = 0;
        uint64_t totalRead = 0;
        Result result = Result::Success;
        while (totalRead < oldFileSize)
        {
            result = oldFile.Read(buffer, HE_LENGTH_OF(buffer), &amountRead);
            if (!result || amountRead == 0)
            {
                break;
            }

            uint32_t amountWritten = 0;
            result = newFile.Write(buffer, amountRead, &amountWritten);
            if (!result || amountWritten == 0)
            {
                break;
            }
            totalRead += amountRead;
        }

        return result;
    }

    Result File::GetAttributes(const char* path, FileAttributes& outAttributes)
    {
        struct stat sb;
        if (stat(path, &sb))
            return Result::FromLastError();

        GatherFileAttributes(sb, path, outAttributes);

        return Result::Success;
    }

    File::File()
        : m_fd(-1)
    {}

    File::File(File&& x)
        : m_fd(Exchange(x.m_fd, -1))
    {}

    File::~File()
    {
        Close();
    }

    File& File::operator=(File&& x)
    {
        Close();
        m_fd = Exchange(x.m_fd, -1);
        return *this;
    }

    Result File::Open(const char* path, FileOpenMode mode, FileOpenFlag openFlags)
    {
        HE_ASSERT(m_fd == -1);
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
        struct stat sb;
        if (fstat(static_cast<int>(m_fd), &sb))
            return Result::FromLastError();

        // TODO: Hidden allocator here, necessary to avoid PATH_MAX issues but probably should
        // expose this so callers know it can allocate.
        String path;
        Result r = GetPath(path);

        GatherFileAttributes(sb, r ? path.Data() : nullptr, outAttributes);

        return Result::Success;
    }

    Result File::GetPath(String& path) const
    {
        char buf[64];
        auto res = fmt::format_to_n(buf, HE_LENGTH_OF(buf), "/proc/self/fd/{}", static_cast<int>(m_fd));
        buf[res.size] = '\0';

        Result r = ReadLink(buf, path);

        // Size of link name changed between lstate and readlink, and it got larger. Try reading
        // it one more time before giving up.
        if (!r)
        {
            r = ReadLink(buf, path);
        }

        return r;
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

    MemoryMap::MemoryMap()
        : m_data(nullptr)
        , m_size(0)
    {}

    MemoryMap::MemoryMap(MemoryMap&& x)
        : m_data(Exchange(x.m_data, nullptr))
        , m_size(Exchange(x.m_size, 0))
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
        return *this;
    }

    Result MemoryMap::Map(File& file, MemoryMapMode mode, uint64_t offset, uint32_t size)
    {
        HE_ASSERT(m_data == nullptr);
        HE_ASSERT(IsAligned(offset, sysconf(_SC_PAGE_SIZE)), HE_KV(offset, offset), HE_KV(page_size, sysconf(_SC_PAGE_SIZE)));

        if (size == 0)
        {
            const uint64_t fileSize = file.GetSize();
            HE_ASSERT(fileSize <= std::numeric_limits<uint32_t>::max());
            size = static_cast<uint32_t>(file.GetSize());
        }

        int prot = PROT_READ;

        if (mode == MemoryMapMode::Write)
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
