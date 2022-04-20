// Copyright Chad Engler

#include "he/core/async_file.h"

#include "he/core/assert.h"
#include "he/core/cpu.h"
#include "he/core/sync.h"
#include "he/core/task_executor.h"
#include "he/core/utils.h"

#include <future>

#if defined(HE_PLATFORM_API_POSIX) && !defined(HE_PLATFORM_EMSCRIPTEN)

#include "file_helpers.posix.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace he
{
    static Mutex s_ioStartupMutex{};
    static uint32_t s_ioStartupCount{ 0 };
    static ThreadPoolExecutor s_threadPool;
    static TaskExecutor* s_executor{ nullptr };

    struct AsyncOp
    {
        int fd;
        uint64_t offset;
        uint32_t size;
        void* buffer;

        std::promise<AsyncFileResult> promise;
    };

    static void HandleCompletedOp(AsyncOp* op, Result result, uint32_t bytesTransferred)
    {
        AsyncFileResult r;
        r.result = result;
        r.bytesTransferred = bytesTransferred;

        close(op->fd);
        op->promise.set_value(r);
        Allocator::GetTemp().Delete(op);
    }

    static void ReadTask(void* data)
    {
        AsyncOp* op = static_cast<AsyncOp*>(data);

        ssize_t n = pread(op->fd, op->buffer, op->size, op->offset);

        if (n < 0)
            HandleCompletedOp(op, Result::FromLastError(), 0);
        else
            HandleCompletedOp(op, Result::Success, static_cast<uint32_t>(n));
    }

    static void WriteTask(void* data)
    {
        AsyncOp* op = static_cast<AsyncOp*>(data);

        ssize_t n = pwrite(op->fd, op->buffer, op->size, op->offset);

        if (n < 0)
            HandleCompletedOp(op, Result::FromLastError(), 0);
        else
            HandleCompletedOp(op, Result::Success, static_cast<uint32_t>(n));
    }

    Result StartupAsyncFileIO(const AsyncFileIOConfig& config)
    {
        LockGuard lock(s_ioStartupMutex);

        const uint32_t count = ++s_ioStartupCount;
        if (count > 1)
            return Result::Success;

        auto countGuard = MakeScopeGuard([]() { --s_ioStartupCount; });

        s_executor = config.executor;

        if (!s_executor)
        {
            const uint32_t defaultThreadCount = Clamp<uint32_t>(GetCpuInfo().threadCount, 4, 8);

            ThreadPoolExecutor::Config poolConfig;
            poolConfig.count = config.pool.threadCount ? config.pool.threadCount : defaultThreadCount;
            poolConfig.affinity = config.pool.threadAffinity;
            poolConfig.name = "Async File IO Thread";

            s_executor = &s_threadPool;
            Result r = s_threadPool.Startup(poolConfig);
            if (!r)
                return r;
        }

        countGuard.Dismiss();
        return Result::Success;
    }

    void ShutdownAsyncFileIO()
    {
        LockGuard lock(s_ioStartupMutex);

        HE_ASSERT(s_ioStartupCount > 0);
        const uint32_t count = --s_ioStartupCount;
        if (count > 0)
            return;

        if (s_executor == &s_threadPool)
            s_threadPool->Shutdown();

        s_executor = nullptr;
    }

    AsyncFile::AsyncFile()
        : m_fd(-1)
    {}

    AsyncFile::AsyncFile(AsyncFile&& x)
        : m_fd(Exchange(x.m_fd, -1))
    {}

    AsyncFile::~AsyncFile()
    {
        Close();
    }

    AsyncFile& AsyncFile::operator=(AsyncFile&& x)
    {
        Close();
        m_fd = Exchange(x.m_fd, -1);
        return *this;
    }

    Result AsyncFile::Open(const char* path, FileOpenMode mode, FileOpenFlag flags)
    {
        HE_ASSERT(m_fd == -1);
        m_fd = PosixFileOpen(path, mode, flags, 0);
        return Result::Success;
    }

    void AsyncFile::Close()
    {
        if (m_fd != -1)
        {
            close(static_cast<int>(m_fd));
            m_fd = -1;
        }
    }

    bool AsyncFile::IsOpen() const
    {
        return m_fd != -1;
    }

    uint64_t AsyncFile::GetSize() const
    {
        struct stat buf;
        if (fstat(static_cast<int>(m_fd), &buf))
            return 0;
        return buf.st_size;
    }

    std::future<AsyncFileResult> AsyncFile::ReadAsync(void* dst, uint64_t offset, uint32_t size)
    {
        AsyncOp* op = Allocator::GetTemp().New<AsyncOp>();
        op->fd = dup(m_fd);
        op->offset = offset;
        op->size = size;
        op->buffer = dst;

        auto f = op->promise.get_future();

        if (op->fd == -1)
            HandleCompletedOp(op, Result::FromLastError(), 0);
        else
            s_executor->Add(ReadTask, op);

        return f;
    }

    std::future<AsyncFileResult> AsyncFile::WriteAsync(const void* src, uint64_t offset, uint32_t size)
    {
        AsyncOp* op = Allocator::GetTemp().New<AsyncOp>();
        op->fd = dup(m_fd);
        op->offset = offset;
        op->size = size;
        op->buffer = const_cast<void*>(src);

        auto f = op->promise.get_future();

        if (op->fd == -1)
            HandleCompletedOp(op, Result::FromLastError(), 0);
        else
            s_executor->Add(WriteTask, op);

        return f;
    }
}

#endif
