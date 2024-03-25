// Copyright Chad Engler

#include "stdio.h"

#include "_common.h"
#include "errno.h"
#include "limits.h"
#include "stdarg.h"
#include "string.h"

#include "wasm/libc.wasm.h"

#include "he/core/atomic.h"
#include "he/core/allocator.h"
#include "he/core/enum_ops.h"
#include "he/core/file.h"
#include "he/core/intrusive_list.h"
#include "he/core/limits.h"
#include "he/core/random.h"
#include "he/core/string_ops.h"
#include "he/core/sync.h"
#include "he/core/tsa.h"
#include "he/core/utils.h"
#include "he/core/result.h"

#undef stdin
#undef stdout
#undef stderr

extern "C"
{
    // --------------------------------------------------------------------------------------------
    enum class SeekWhence : uint32_t
    {
        Set = 0,
        Cur = 1,
        End = 2,
    };

    enum class FileFlag : uint32_t
    {
        None        = 0,
        Perm        = 1 << 0,
        NoRead      = 1 << 1,
        NoWrite     = 1 << 2,
        EndOfFile   = 1 << 3,
        Error       = 1 << 4,
    };
    HE_ENUM_FLAGS(FileFlag);

    struct _IO_FILE
    {
        he::IntrusiveListLink<_IO_FILE> link;

        int32_t fd;
        FileFlag flags;
        he::Mutex mutex;

        _IO_FILE(int32_t fd, FileFlag flags) : fd(fd), flags(flags) {}
        virtual ~_IO_FILE() = default;

        virtual size_t Read([[maybe_unused]] uint8_t* dst, [[maybe_unused]] size_t len) { return 0; }
        virtual size_t Write([[maybe_unused]] const uint8_t* src, [[maybe_unused]] size_t len) { return 0; }
        virtual size_t Write(const char* src, size_t len) { return Write(reinterpret_cast<const uint8_t*>(src), len); }
        virtual int Seek([[maybe_unused]] long offset, [[maybe_unused]] SeekWhence whence) { return 0; }
        virtual long Tell() { return 0; }
        virtual int Flush() { return 0; }
        virtual int Close() { return 0; }
    };

    static he::Atomic<uint32_t> s_nextFileId = 3;
    static he::Mutex s_filesMutex;
    static he::IntrusiveList<_IO_FILE, &_IO_FILE::link> s_files HE_TSA_GUARDED_BY(s_filesMutes);

    static constexpr int32_t FileId_Stdin = 0;
    static constexpr int32_t FileId_Stdout = 1;
    static constexpr int32_t FileId_Stderr = 2;

    constexpr bool IsStdFileId(int32_t fd)
    {
        return fd == FileId_Stdin || fd == FileId_Stdout || fd == FileId_Stderr;
    }

    HE_FORCE_INLINE bool IsStdFile(_IO_FILE* stream)
    {
        return stream && IsStdFileId(stream->fd);
    }

    HE_FORCE_INLINE void RandName6(char* name, he::Random64& rng)
    {
        uint64_t r = rng.Next();

        for (uint32_t i = 0; i < 6; ++i)
        {
            name[i] = 'A' + (r & 15) + ((r & 16) * 2);
            r >>= 5;
        }
    }

    // --------------------------------------------------------------------------------------------
    // stdin
    struct _StdIn : public _IO_FILE
    {
        _StdIn() : _IO_FILE(FileId_Stdin, FileFlag::Perm | FileFlag::NoWrite) {}

        virtual size_t Read(uint8_t* dst, size_t len) override
        {
            // TODO: What is stdin for WASM?
            (void)dst;
            (void)len;
            return 0;
        }
    };
    _StdIn __stdin_FILE;
    FILE* const stdin = &__stdin_FILE;

    // stdout
    struct _StdOut : public _IO_FILE
    {
        _StdOut() : _IO_FILE(FileId_Stdout, FileFlag::Perm | FileFlag::NoRead) {}

        virtual size_t Write(const uint8_t* src, size_t len) override
        {
            heWASM_StdIoWrite(fd, reinterpret_cast<const char*>(src), static_cast<uint32_t>(len));
            return 0;
        }
    };
    _StdOut __stdout_FILE;
    FILE* const stdout = &__stdout_FILE;

    // stderr
    struct _StdErr : public _IO_FILE
    {
        _StdErr() : _IO_FILE(FileId_Stderr, FileFlag::Perm | FileFlag::NoRead) {}

        virtual size_t Write(const uint8_t* src, size_t len) override
        {
            heWASM_StdIoWrite(fd, reinterpret_cast<const char*>(src), static_cast<uint32_t>(len));
            return 0;
        }
    };
    _StdErr __stderr_FILE;
    FILE* const stderr = &__stderr_FILE;

    // --------------------------------------------------------------------------------------------
    struct _File : public _IO_FILE
    {
        he::File file;

        _File() : _IO_FILE(s_nextFileId++, FileFlag::None) {}

        bool Open(const char* path, he::FileAccessMode access, he::FileCreateMode create, he::FileOpenFlag flags)
        {
            const he::Result rc = file.Open(path, access, create, flags);
            if (rc.IsOk())
                return true;

            errno = static_cast<int>(rc.GetCode());
            return false;
        }

        virtual size_t Read(uint8_t* dst, size_t len) override
        {
            uint32_t bytesRead = 0;
            const he::Result rc = file.Read(dst, len, &bytesRead);
            if (rc.IsOk())
                return bytesRead;

            errno = static_cast<int>(rc.GetCode());
            return 0;
        }

        virtual size_t Write(const uint8_t* src, size_t len) override
        {
            uint32_t bytesWritten = 0;
            const he::Result rc = file.Write(src, len, &bytesWritten);
            if (rc.IsOk())
                return bytesWritten;

            errno = static_cast<int>(rc.GetCode());
            return rc.IsOk() ? bytesWritten : 0;
        }

        virtual int Seek(long offset, SeekWhence whence) override
        {
            const uint64_t size = file.GetSize();
            uint64_t base = 0;
            switch (whence)
            {
                case SeekWhence::Set: base = 0; break;
                case SeekWhence::Cur: base = file.GetPos(); break;
                case SeekWhence::End: base = size; break;
            }

            if (offset < -static_cast<int64_t>(base) || offset > static_cast<int64_t>(size - base))
            {
                errno = EINVAL;
                return -1;
            }

            const he::Result rc = file.SetPos(base + offset);
            if (rc.IsOk())
                return 0;

            errno = static_cast<int>(rc.GetCode());
            return -1;
        }

        virtual long Tell() override
        {
            const uint64_t pos = file.GetPos();
            if (pos > LONG_MAX)
            {
                return -1;
            }
            return static_cast<long>(pos);
        }

        virtual int Flush() override
        {
            file.Flush();
            return 0;
        }

        virtual int Close() override
        {
            fd = -1;
            file.Close();
            return 0;
        }
    };

    // --------------------------------------------------------------------------------------------
    struct _StrFile : public _IO_FILE
    {
        char* str{ nullptr };
        size_t len{ 0 };
        size_t pos{ 0 };

        _StrFile(char* str, size_t len) : _IO_FILE(-1, FileFlag::Perm), str(str), len(len) {}
        _StrFile(const char* str) : _IO_FILE(-1, FileFlag::Perm), str(const_cast<char*>(str)), len(0xffffffff) {}

        virtual size_t Read(uint8_t* dst, size_t len) override
        {
            const size_t bytesToRead = he::Min(len, this->len - pos);
            he::MemCopy(dst, str + pos, bytesToRead);
            pos += bytesToRead;
            return bytesToRead;
        }

        virtual size_t Write(const uint8_t* src, size_t len) override
        {
            const size_t bytesToWrite = he::Min(len, this->len - pos);
            he::MemCopy(str + pos, src, bytesToWrite);
            pos += bytesToWrite;
            return bytesToWrite;
        }

        virtual int Seek(long offset, SeekWhence whence) override
        {
            size_t base = 0;
            switch (whence)
            {
                case SeekWhence::Set: base = 0; break;
                case SeekWhence::Cur: base = pos; break;
                case SeekWhence::End: base = len; break;
            }

            if (offset < -static_cast<ssize_t>(base) || offset > static_cast<ssize_t>(len - base))
            {
                errno = EINVAL;
                return -1;
            }

            pos = base + offset;
            return 0;
        }

        virtual long Tell() override
        {
            return static_cast<long>(pos);
        }
    };

    // --------------------------------------------------------------------------------------------
    FILE* fopen(const char* __restrict path, const char* __restrict mode)
    {
        _File* file = he::Allocator::GetDefault().New<_File>();
        if (!file)
        {
            errno = ENOMEM;
            return nullptr;
        }

        he::FileAccessMode access = he::FileAccessMode::Read;
        he::FileCreateMode create = he::FileCreateMode::OpenExisting;
        he::FileOpenFlag flags = he::FileOpenFlag::None;
        const bool hasPlus = he::StrFind(mode, '+');
        switch (mode[0])
        {
            case 'r':
                access = hasPlus ? he::FileAccessMode::ReadWrite : he::FileAccessMode::Read;
                create = he::FileCreateMode::OpenExisting;
                break;
            case 'w':
                access = hasPlus ? he::FileAccessMode::ReadWrite : he::FileAccessMode::Write;
                create = he::FileCreateMode::CreateAlways;
                break;
            case 'a':
                access = hasPlus ? he::FileAccessMode::ReadWrite : he::FileAccessMode::Write;
                create = he::FileCreateMode::OpenAlways;
                flags |= he::FileOpenFlag::Append;
                break;
            default:
                he::Allocator::GetDefault().Delete(file);
                errno = EINVAL;
                return nullptr;
        }

        if (!file->Open(path, access, create, flags))
        {
            he::Allocator::GetDefault().Delete(file);
            return nullptr;
        }

        s_filesMutex.Acquire();
        s_files.PushBack(file);
        s_filesMutex.Release();

        return file;
    }

    FILE* freopen(const char* __restrict path, const char* __restrict mode, FILE* __restrict stream)
    {
        // Do nothing for stdin/stdout/stderr
        if (IsStdFile(stream))
            return stream;

        if (fclose(stream) != 0)
            return nullptr;

        return fopen(path, mode);
    }

    int fclose(FILE* stream)
    {
        if (!stream)
            return 0;

        stream->mutex.Acquire();
        stream->Flush();
        const int rc = stream->Close();
        stream->mutex.Release();

        if (he::HasFlag(stream->flags, FileFlag::Perm))
            return rc;

        s_filesMutex.Acquire();
        s_files.Remove(stream);
        s_filesMutex.Release();

        he::Allocator::GetDefault().Delete(stream);
        return rc;
    }

    int remove(const char* path)
    {
        const he::Result rc = he::File::Remove(path);
        if (rc.IsOk())
            return 0;

        errno = static_cast<int>(rc.GetCode());
        return -1;
    }

    int rename(const char* path, const char* newPath)
    {
        const he::Result rc = he::File::Rename(path, newPath);
        if (rc.IsOk())
            return 0;

        errno = static_cast<int>(rc.GetCode());
        return -1;
    }

    int feof(FILE* stream)
    {
        he::LockGuard lock(stream->mutex);
        return static_cast<int>(he::HasFlag(stream->flags, FileFlag::EndOfFile));
    }

    int ferror(FILE* stream)
    {
        he::LockGuard lock(stream->mutex);
        return static_cast<int>(he::HasFlag(stream->flags, FileFlag::Error));
    }

    int fflush(FILE* stream)
    {
        if (!stream)
        {
            int rc = 0;
            rc |= fflush(stdout);
            rc |= fflush(stderr);

            he::LockGuard lock(s_filesMutex);
            for (FILE& file : s_files)
                rc |= fflush(&file);

            return rc;
        }

        he::LockGuard lock(stream->mutex);
        return stream->Flush();
    }

    void clearerr(FILE* stream)
    {
        he::LockGuard lock(stream->mutex);
        stream->flags &= ~(FileFlag::EndOfFile | FileFlag::Error);
    }

    int fseek(FILE* stream, long offset, int whence)
    {
        SeekWhence wh = SeekWhence::Set;
        switch (whence)
        {
            case SEEK_SET: wh = SeekWhence::Set; break;
            case SEEK_CUR: wh = SeekWhence::Cur; break;
            case SEEK_END: wh = SeekWhence::End; break;
            default: errno = EINVAL; return -1;
        }

        he::LockGuard lock(stream->mutex);
        const int rc = stream->Seek(offset, wh);

        if (rc == 0)
        {
            // remove EOF flag if it was set
            stream->flags &= ~FileFlag::EndOfFile;
            return 0;
        }

        errno = rc;
        return -1;
    }

    long ftell(FILE* stream)
    {
        he::LockGuard lock(stream->mutex);
        const long pos = stream->Tell();
        if (pos < 0)
        {
            errno = EOVERFLOW;
            return -1;
        }
        return static_cast<off_t>(pos);
    }

    void rewind(FILE* stream)
    {
        he::LockGuard lock(stream->mutex);
        stream->Seek(0, SeekWhence::Set);
        stream->flags &= ~FileFlag::Error;
    }

    int fgetpos(FILE* __restrict stream, fpos_t* __restrict pos)
    {
        const off_t off = ftell(stream);
        if (off < 0)
            return -1;
        pos->__lldata = off;
        return 0;
    }

    int fsetpos(FILE* stream, const fpos_t* pos)
    {
        return fseek(stream, pos->__lldata, SEEK_SET);
    }

    size_t fread(void* __restrict ptr, size_t size, size_t nmemb, FILE* __restrict stream)
    {
        if (size == 0 || nmemb == 0)
            return 0;

        he::LockGuard lock(stream->mutex);
        if (he::HasFlag(stream->flags, FileFlag::NoRead))
        {
            stream->flags |= FileFlag::Error;
            errno = EBADF;
            return 0;
        }

        uint8_t* dst = static_cast<uint8_t*>(ptr);
        const size_t len = size * nmemb;
        const size_t bytesRead = stream->Read(dst, len);
        return bytesRead / size;
    }

    size_t fwrite(const void* __restrict ptr, size_t size, size_t nmemb, FILE* __restrict stream)
    {
        if (size == 0 || nmemb == 0)
            return 0;

        he::LockGuard lock(stream->mutex);
        if (he::HasFlag(stream->flags, FileFlag::NoWrite))
        {
            stream->flags |= FileFlag::Error;
            errno = EBADF;
            return 0;
        }

        const uint8_t* src = static_cast<const uint8_t*>(ptr);
        const size_t len = size * nmemb;
        const size_t bytesWritten = stream->Write(src, len);
        return bytesWritten / size;
    }

    int fgetc(FILE* stream)
    {
        he::LockGuard lock(stream->mutex);
        if (he::HasFlag(stream->flags, FileFlag::NoRead))
        {
            stream->flags |= FileFlag::Error;
            errno = EBADF;
            return EOF;
        }

        uint8_t c;
        const size_t bytesRead = stream->Read(&c, 1);
        return bytesRead == 1 ? static_cast<int>(c) : EOF;
    }

    int getc(FILE* stream)
    {
        return fgetc(stream);
    }

    int getchar()
    {
        return fgetc(stdin);
    }

    // int ungetc(int c, FILE* stream)
    // {
    //     // TODO: Implementing this requires userland buffers
    //     return EOF;
    // }

    int fputc(int c, FILE* stream)
    {
        he::LockGuard lock(stream->mutex);
        if (he::HasFlag(stream->flags, FileFlag::NoWrite))
        {
            stream->flags |= FileFlag::Error;
            errno = EBADF;
            return EOF;
        }

        const uint8_t ch = static_cast<uint8_t>(c);
        const size_t bytesWritten = stream->Write(&ch, 1);
        return bytesWritten == 1 ? c : EOF;
    }

    int putc(int c, FILE* stream)
    {
        return fputc(c, stream);
    }

    int putchar(int c)
    {
        return fputc(c, stdout);
    }

    // char* fgets(char* __restrict s, int size, FILE* __restrict stream)
    // {
    //     // TODO: Implementing this requires userland buffers
    // }

    // char* gets(char* s)
    // {
    //     // TODO: Implementing this requires userland buffers
    // }

    int fputs(const char* __restrict s, FILE* __restrict stream)
    {
        const uint32_t len = he::StrLen(s);
        const size_t bytesWritten = fwrite(s, 1, len, stream);
        return bytesWritten == len ? 0 : EOF;
    }

    int puts(const char* s)
    {
        const uint32_t len = he::StrLen(s);

        he::LockGuard lock(stdout->mutex);
        const size_t bytesWritten = stdout->Write(s, len);
        if (bytesWritten != len)
            return EOF;

        const size_t newlineWritten = stdout->Write("\n", 1);
        return newlineWritten == 1 ? 0 : EOF;
    }

    int printf(const char* __restrict format, ...)
    {
        va_list args;
        va_start(args, format);
        const int rc = vfprintf(stdout, format, args);
        va_end(args);
        return rc;
    }

    int fprintf(FILE* __restrict stream, const char* __restrict format, ...)
    {
        va_list args;
        va_start(args, format);
        const int rc = vfprintf(stream, format, args);
        va_end(args);
        return rc;
    }

    int sprintf(char* __restrict s, const char* __restrict format, ...)
    {
        va_list args;
        va_start(args, format);
        const int rc = vsprintf(s, format, args);
        va_end(args);
        return rc;
    }

    int snprintf(char* __restrict s, size_t size, const char* __restrict format, ...)
    {
        va_list args;
        va_start(args, format);
        const int rc = vsnprintf(s, size, format, args);
        va_end(args);
        return rc;
    }

    int vprintf(const char* __restrict format, __builtin_va_list args)
    {
        return vfprintf(stdout, format, args);
    }

    int vfprintf(FILE* __restrict stream, const char* __restrict format, __builtin_va_list args)
    {
        // TODO: What a fucking nightmare
        (void)stream;
        (void)format;
        (void)args;
        return -1;
    }

    int vsprintf(char* __restrict s, const char* __restrict format, __builtin_va_list args)
    {
        return vsnprintf(s, INT_MAX, format, args);
    }

    int vsnprintf(char* __restrict s, size_t size, const char* __restrict format, __builtin_va_list args)
    {
        if (size > INT_MAX)
        {
            errno = EOVERFLOW;
            return -1;
        }

        _StrFile f(s, size);
        return vfprintf(&f, format, args);
    }

    int scanf(const char* __restrict format, ...)
    {
        va_list args;
        va_start(args, format);
        const int rc = vscanf(format, args);
        va_end(args);
        return rc;
    }

    int fscanf(FILE* __restrict stream, const char* __restrict format, ...)
    {
        va_list args;
        va_start(args, format);
        const int rc = vfscanf(stream, format, args);
        va_end(args);
        return rc;
    }

    int sscanf(const char* __restrict s, const char* __restrict format, ...)
    {
        va_list args;
        va_start(args, format);
        const int rc = vsscanf(s, format, args);
        va_end(args);
        return rc;
    }

    int vscanf(const char* __restrict format, __builtin_va_list args)
    {
        return vfscanf(stdin, format, args);
    }

    int vfscanf(FILE* __restrict stream, const char* __restrict format, __builtin_va_list args)
    {
        // TODO: What a fucking nightmare
        (void)stream;
        (void)format;
        (void)args;
        return EOF;
    }

    int vsscanf(const char* __restrict s, const char* __restrict format, __builtin_va_list args)
    {
        _StrFile f(s);
        return vfscanf(&f, format, args);
    }

    void perror(const char* msg)
    {
        const char* errStr = strerror(errno);

        he::LockGuard lock(stderr->mutex);
        if (msg && *msg)
        {
            stderr->Write(msg, he::StrLen(msg));
            stderr->Write(": ", 2);
        }
        stderr->Write(errStr, he::StrLen(errStr));
        stderr->Write("\n", 1);
    }

    // int setvbuf(FILE* __restrict stream, char* __restrict buf, int mode, size_t size)
    // {
    //     // TODO: Implementing this requires userland buffers
    // }

    // void setbuf(FILE* __restrict stream, char* __restrict buf)
    // {
    //     // TODO: Implementing this requires userland buffers
    // }

    char* tmpnam(char* buf)
    {
        static constexpr uint32_t MaxTries = 100;
        static char s_buf[L_tmpnam];

        char name[] = "/tmp/tmpnam_XXXXXX";
        he::Random64 rng;

        for (uint32_t i = 0; i < MaxTries; ++i)
        {
            RandName6(name + 12, rng);

            if (!he::File::Exists(name))
            {
                char* dst = buf ? buf : s_buf;
                he::StrCopy(dst, L_tmpnam, name);
                return dst;
            }
        }

        return nullptr;
    }

    FILE* tmpfile()
    {
        static constexpr uint32_t MaxTries = 100;

        char name[] = "/tmp/tmpfile_XXXXXX";
        he::Random64 rng;

        _File* file = he::Allocator::GetDefault().New<_File>();
        if (!file)
        {
            errno = ENOMEM;
            return nullptr;
        }

        for (uint32_t i = 0; i < MaxTries; ++i)
        {
            RandName6(name + 13, rng);

            if (!file->Open(name, he::FileAccessMode::ReadWrite, he::FileCreateMode::CreateNew, he::FileOpenFlag::None))
            {
                continue;
            }

            s_filesMutex.Acquire();
            s_files.PushBack(file);
            s_filesMutex.Release();

            return file;
        }

        he::Allocator::GetDefault().Delete(file);
        return nullptr;
    }
}
