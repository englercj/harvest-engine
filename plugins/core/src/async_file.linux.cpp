// Copyright Chad Engler

#include "he/core/async_file.h"

#include "he/core/assert.h"
#include "he/core/cpu.h"
#include "he/core/compiler.h"
#include "he/core/enum_ops.h"
#include "he/core/log.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/sync.h"
#include "he/core/scope_guard.h"
#include "he/core/thread.h"
#include "he/core/utils.h"

#include <atomic>
#include <future>
#include <thread>

#if defined(HE_PLATFORM_LINUX)

#include "file_helpers.posix.h"

#include <liburing.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <liburing/io_uring.h>
#include <sys/utsname.h>

// #include <errno.h>
// #include <fcntl.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <sys/stat.h>
// #include <sys/types.h>

namespace he
{
    struct RequiredIORingOp { int value; const char* name; };
    constexpr RequiredIORingOp RequiredOps[] =
    {
        { IORING_OP_NOP, "IORING_OP_NOP" },
        { IORING_OP_READ, "IORING_OP_READ" },
        { IORING_OP_MSG_RING, "IORING_OP_MSG_RING" },
    };

    static Mutex s_ioStartupMutex{};
    static uint32_t s_ioStartupCount{ 0 };
    static io_uring s_ring{};
    static std::thread s_cqThread{};

    struct AsyncOp
    {
        int fd;
        bool isRead;
        std::promise<AsyncFileResult> promise;
    };

    static void HandleCompletedOp(AsyncOp* op, Result result, uint32_t bytesTransferred);
    static void HandleCompletedOp(AsyncOp* op, int32_t res);
    static void CompletionQueueThread();

    static void UnlockedShutdownAsyncFileIO();

    static io_uring_sqe* SpinWaitForSqe()
    {
        constexpr uint32_t SpinIterations = 10;

        for (uint32_t i = 0; i < SpinIterations; ++i)
        {
            io_uring_sqe* sqe = io_uring_get_sqe(&s_ring);
            if (sqe)
                return sqe;

            HE_SPIN_WAIT_PAUSE();
        }

        return nullptr;
    }

    static io_uring_sqe* GetNextSqe()
    {
        io_uring_sqe* sqe = io_uring_get_sqe(&s_ring);

        while (sqe == nullptr) [[unlikely]]
        {
            sqe = SpinWaitForSqe();

            // If we didn't get one, wait for one entry to complete and then try again
            if (!sqe)
            {
                io_uring_submit_and_wait(&s_ring, 1);
            }
        }

        return sqe;
    }

    Result StartupAsyncFileIO(const AsyncFileIOConfig& config)
    {
        LockGuard lock(s_ioStartupMutex);

        const uint32_t count = ++s_ioStartupCount;
        if (count > 1)
            return Result::Success;

        auto failGuard = MakeScopeGuard([]() { UnlockedShutdownAsyncFileIO(); });

        // Check for required io_uring feature availability
        io_uring_probe* probe = io_uring_get_probe();
        HE_AT_SCOPE_EXIT([&]() { free(probe); });

        for (const RequiredIORingOp& op : RequiredOps)
        {
            if (!io_uring_opcode_supported(probe, op.value))
            {
                HE_LOG_ERROR(he_core,
                    HE_MSG("Required io_uring operation is not supported."),
                    HE_KV(operation, op.value),
                    HE_KV(operation_name, op.name));
                return Result::NotSupported;
            }
        }

        // Create the io_uring queue
        const int rc = io_uring_queue_init(config.iouring.capacity, &s_ring, 0);
        if (rc < 0)
            return PosixResult(-rc);

        s_cqThread = std::thread(&CompletionQueueThread);

        failGuard.Dismiss();
        return Result::Success;
    }

    static void UnlockedShutdownAsyncFileIO()
    {
        HE_ASSERT(s_ioStartupCount > 0);
        const uint32_t count = --s_ioStartupCount;
        if (count > 0)
            return;

        io_uring_sqe* sqe = GetNextSqe();
        io_uring_prep_msg_ring(sqe, s_ring.ring_fd, 0, 0, 0);

        if (s_cqThread.joinable())
            s_cqThread.join();

        io_uring_queue_exit(&s_ring);
    }

    void ShutdownAsyncFileIO()
    {
        LockGuard lock(s_ioStartupMutex);
        UnlockedShutdownAsyncFileIO();
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

        // When using io_uring the O_NONBLOCK flag actually tells io_uring to let the user choose
        // what to do in a blocking situation, rather than letting io_uring deal with it. However,
        // we want io_uring to deal with it so we don't pass this flag.
        const int file = PosixFileOpen(path, mode, flags, 0);
        if (file == -1)
            return Result::FromLastError();

        m_fd = static_cast<intptr_t>(file);
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
        AsyncOp* op = Allocator::GetDefault().New<AsyncOp>();
        op->fd = dup(static_cast<int>(m_fd));
        op->isRead = true;

        auto f = op->promise.get_future();

        if (op->fd == -1)
        {
            HandleCompletedOp(op, Result::FromLastError(), 0);
        }
        else
        {
            io_uring_sqe* sqe = GetNextSqe();
            io_uring_prep_read(sqe, op->fd, dst, size, offset);
            io_uring_sqe_set_data(sqe, op);
        }

        return f;
    }

    std::future<AsyncFileResult> AsyncFile::WriteAsync(const void* src, uint64_t offset, uint32_t size)
    {
        AsyncOp* op = Allocator::GetDefault().New<AsyncOp>();
        op->fd = dup(static_cast<int>(m_fd));
        op->isRead = false;

        auto f = op->promise.get_future();

        if (op->fd == -1)
        {
            HandleCompletedOp(op, Result::FromLastError(), 0);
        }
        else
        {
            io_uring_sqe* sqe = GetNextSqe();
            io_uring_prep_write(sqe, op->fd, src, size, offset);
            io_uring_sqe_set_data(sqe, op);
        }

        return f;
    }

    Result AsyncFile::GetAttributes(FileAttributes& outAttributes) const
    {
        return PosixFileGetAttributes(static_cast<int>(m_fd), outAttributes);
    }

    Result AsyncFile::GetPath(String& outPath) const
    {
        return PosixFileGetPath(static_cast<int>(m_fd), outPath);
    }

    static void HandleCompletedOp(AsyncOp* op, Result result, uint32_t bytesTransferred)
    {
        AsyncFileResult r;
        r.result = result;
        r.bytesTransferred = bytesTransferred;

        close(op->fd);
        op->promise.set_value(r);
        Allocator::GetDefault().Delete(op);
    }

    static void HandleCompletedOp(AsyncOp* op, int32_t res)
    {
        const Result r = res < 0 ? PosixResult(-res) : Result::Success;
        const uint32_t bytesTransferred = res < 0 ? 0 : res;
        HandleCompletedOp(op, r, bytesTransferred);
    }

    static void CompletionQueueThread()
    {
        SetCurrentThreadName("[HE] Async File io_uring CQ Thread");

        while (s_ioStartupCount > 0)
        {
            io_uring_cqe* cqe;
            const int rc = io_uring_wait_cqe(&s_ring, &cqe);

            HE_AT_SCOPE_EXIT([&]() { io_uring_cqe_seen(&s_ring, cqe); });

            if (rc != 0)
            {
                HE_LOG_ERROR(he_core,
                    HE_MSG("Failed to get io_uring CQ entry."),
                    HE_KV(result, PosixResult(-rc)));
                continue;
            }

            if (cqe->user_data == 0)
                continue;

            AsyncOp* op = static_cast<AsyncOp*>(io_uring_cqe_get_data(cqe));
            if (op)
            {
                HandleCompletedOp(op, cqe->res);
            }
        }
    }
}

#endif
