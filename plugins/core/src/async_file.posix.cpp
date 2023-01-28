// Copyright Chad Engler

#include "he/core/async_file.h"

#include "he/core/assert.h"
#include "he/core/cpu_info.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/sync.h"
#include "he/core/task_executor.h"
#include "he/core/utils.h"

#if defined(HE_PLATFORM_API_POSIX) && !defined(HE_PLATFORM_LINUX) && !defined(HE_PLATFORM_EMSCRIPTEN)

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
    // --------------------------------------------------------------------------------------------
    static Mutex s_ioStartupMutex{};
    static uint32_t s_ioStartupCount{ 0 };
    static ThreadPoolExecutor s_threadPool;
    static TaskExecutor* s_executor{ nullptr };

    // --------------------------------------------------------------------------------------------
    class AsyncOp
    {
    public:
        ~AsyncOp()
        {
            Close();
        }

        int File() const { return m_fd; }

        void Close()
        {
            if (m_fd != -1)
            {
                close(m_fd);
                m_fd = -1;
            }
        }

        bool Reset(int fd, uint64_t offset, uint32_t size, void* buf, AsyncFile::CompleteDelegate callback)
        {
            m_fd = dup(fd);

            if (m_fd == -1)
            {
                m_result = Result::FromLastError();
                return false;
            }

            m_offset = offset;
            m_size = size;
            m_buffer = buf;
            m_event.Reset();

            m_result = PosixResult(EINPROGRESS);
            m_bytesTransferred = 0;
            m_callback = callback;

            return true
        }

        bool IsComplete() const
        {
            return m_event.Wait(Duration_Zero);
        }

        void Wait() const
        {
            m_event.Wait();
        }

        Result GetResult(uint32_t* bytesTransferred)
        {
            HE_ASSERT(IsComplete());

            if (bytesTransferred)
                *bytesTransferred = m_bytesTransferred;

            return m_result;
        }

        void SetResult(Result result, uint32_t bytesTransferred)
        {
            m_result = result;
            m_bytesTransferred = bytesTransferred;
            m_event.Signal();

            if (m_callback)
                m_callback(*this);
        }

        operator AsyncFileOp() const { return AsyncFileOp{ reinterpret_cast<uintptr_t>(this) }; }

    public:
        static void ReadTask(const void* data)
        {
            AsyncOp* op = const_cast<AsyncOp*>(static_cast<const AsyncOp*>(data));

            ssize_t n = pread(op->fd, op->buffer, op->size, op->offset);

            if (n < 0)
                op->SetResult(Result::FromLastError(), 0);
            else
                op->SetResult(Result::Success, static_cast<uint32_t>(n));
        }

        static void WriteTask(const void* data)
        {
            AsyncOp* op = const_cast<AsyncOp*>(static_cast<const AsyncOp*>(data));

            ssize_t n = pwrite(op->fd, op->buffer, op->size, op->offset);

            if (n < 0)
                op->SetResult(Result::FromLastError(), 0);
            else
                op->SetResult(Result::Success, static_cast<uint32_t>(n));
        }

        static void ErrorTask(const void* data)
        {
            AsyncOp* op = const_cast<AsyncOp*>(static_cast<const AsyncOp*>(data));
            HE_ASSERT(!op->m_result && op->m_result.GetCode() != EINPROGRESS, HE_KV(result, op->m_result));
            op->SetResult(op->m_result, 0);
        }

    private:
        int m_fd;
        uint64_t m_offset;
        uint32_t m_size;
        void* m_buffer;
        SyncEvent m_event{};

        Result m_result{};
        uint32_t m_bytesTransferred{ 0 };
        AsyncFile::CompleteDelegate m_callback{};
    };

    // --------------------------------------------------------------------------------------------
    static void HandleCompletedOp(AsyncOp* op, Result result, uint32_t bytesTransferred)
    {
        AsyncFileResult r;
        r.result = result;
        r.bytesTransferred = bytesTransferred;

        close(op->fd);
        op->promise.set_value(r);
        Allocator::GetDefault().Delete(op);
    }

    static void UnlockedShutdownAsyncFileIO();

    // --------------------------------------------------------------------------------------------
    Result StartupAsyncFileIO(const AsyncFileIOConfig& config)
    {
        LockGuard lock(s_ioStartupMutex);

        const uint32_t count = ++s_ioStartupCount;
        if (count > 1)
            return Result::Success;

        auto failGuard = MakeScopeGuard([]() { UnlockedShutdownAsyncFileIO(); });

        s_executor = config.threadpool.executor;

        if (!s_executor)
        {
            const uint32_t defaultThreadCount = Clamp<uint32_t>(GetCpuInfo().threadCount, 4, 8);

            ThreadPoolExecutor::Config poolConfig;
            poolConfig.count = config.threadpool.threadCount ? config.threadpool.threadCount : defaultThreadCount;
            poolConfig.affinity = config.threadpool.threadAffinity;
            poolConfig.name = "Async File IO Thread";

            Result r = s_threadPool.Startup(poolConfig);
            if (!r)
                return r;

            s_executor = &s_threadPool;
        }

        failGuard.Dismiss();
        return Result::Success;
    }

    void UnlockedShutdownAsyncFileIO()
    {
        HE_ASSERT(s_ioStartupCount > 0);
        const uint32_t count = --s_ioStartupCount;
        if (count > 0)
            return;

        if (s_executor == &s_threadPool)
            s_threadPool.Shutdown();

        s_executor = nullptr;
    }

    void ShutdownAsyncFileIO()
    {
        LockGuard lock(s_ioStartupMutex);
        UnlockedShutdownAsyncFileIO();
    }

    // --------------------------------------------------------------------------------------------
    bool AsyncFile::IsComplete(AsyncFileOp token)
    {
        AsyncOp* op = reinterpret_cast<AsyncOp*>(token.val);
        return op->IsComplete();
    }

    Result AsyncFile::GetResult(AsyncFileOp token, uint32_t* bytesTransferred)
    {
        AsyncOp* op = reinterpret_cast<AsyncOp*>(token.val);
        op->Wait();
        Result r = op->GetResult(bytesTransferred);
        Allocator::GetDefault().Delete(op);
        return r;
    }

    AsyncFile::AsyncFile() noexcept
        : m_fd(-1)
    {}

    AsyncFile::AsyncFile(AsyncFile&& x) noexcept
        : m_fd(Exchange(x.m_fd, -1))
    {}

    AsyncFile::~AsyncFile() noexcept
    {
        Close();
    }

    AsyncFile& AsyncFile::operator=(AsyncFile&& x) noexcept
    {
        Close();
        m_fd = Exchange(x.m_fd, -1);
        return *this;
    }

    Result AsyncFile::Open(const char* path, FileOpenMode mode, FileOpenFlag flags)
    {
        if (!HE_VERIFY(m_fd == -1))
            return Result::InvalidParameter;

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

    AsyncFileOp AsyncFile::ReadAsync(void* dst, uint64_t offset, uint32_t size, CompleteDelegate callback)
    {
        const int fd = static_cast<int>(m_fd);

        // TODO: Pool these allocations in an object pool.
        AsyncOp* op = Allocator::GetDefault().New<AsyncOp>();

        if (!op->Reset(fd, offset, size, dst, callback))
            s_executor->Add(TaskDelegate::Make(AsyncOp::ErrorTask, op));
        else
            s_executor->Add(TaskDelegate::Make(AsyncOp::ReadTask, op));

        return *op;
    }

    AsyncFileOp AsyncFile::WriteAsync(const void* src, uint64_t offset, uint32_t size, CompleteDelegate callback)
    {
        const int fd = static_cast<int>(m_fd);

        // TODO: Pool these allocations in an object pool.
        AsyncOp* op = Allocator::GetDefault().New<AsyncOp>();

        if (!op->Reset(fd, offset, size, const_cast<void*>(src), callback))
            s_executor->Add(TaskDelegate::Make(AsyncOp::ErrorTask, op));
        else
            s_executor->Add(TaskDelegate::Make(AsyncOp::ReadTask, op));

        return *op;
    }

    Result AsyncFile::GetAttributes(FileAttributes& outAttributes) const
    {
        return PosixFileGetAttributes(static_cast<int>(m_fd), outAttributes);
    }

    Result AsyncFile::GetPath(String& outPath) const
    {
        return PosixFileGetPath(static_cast<int>(m_fd), outPath);
    }
}

#endif
