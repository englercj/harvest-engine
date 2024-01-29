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

#include <thread>

#if defined(HE_PLATFORM_LINUX)

#include "file_helpers.posix.h"

#include <errno.h>
#include <liburing.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>

namespace he
{
    // --------------------------------------------------------------------------------------------
    static Mutex s_ioStartupMutex{};
    static uint32_t s_ioStartupCount{ 0 };
    static io_uring s_ring{};
    static std::thread s_cqThread{};

    // --------------------------------------------------------------------------------------------
    struct RequiredIORingOp { int value; const char* name; };
    constexpr RequiredIORingOp RequiredOps[] =
    {
        { IORING_OP_NOP, "IORING_OP_NOP" },
        { IORING_OP_READ, "IORING_OP_READ" },
        { IORING_OP_MSG_RING, "IORING_OP_MSG_RING" },
    };

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

        bool Reset(int fd, AsyncFile::CompleteDelegate callback)
        {
            m_fd = dup(fd);
            m_event.Reset();

            m_result = PosixResult(EINPROGRESS);
            m_bytesTransferred = 0;
            m_callback = callback;

            return m_fd != -1;
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

        void SetResult(int res)
        {
            m_result = res < 0 ? PosixResult(-res) : Result::Success;
            m_bytesTransferred = res < 0 ? 0 : res;
            m_event.Signal();

            if (m_callback)
                m_callback(*this);
        }

        operator AsyncFileOp() const { return AsyncFileOp{ reinterpret_cast<uintptr_t>(this) }; }

    private:
        int m_fd{ -1 };
        SyncEvent m_event{};

        Result m_result{};
        uint32_t m_bytesTransferred{ 0 };
        AsyncFile::CompleteDelegate m_callback{};
    };

    // --------------------------------------------------------------------------------------------
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

    // --------------------------------------------------------------------------------------------
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

    AsyncFileOp AsyncFile::ReadAsync(void* dst, uint64_t offset, uint32_t size, CompleteDelegate callback)
    {
        const int fd = static_cast<int>(m_fd);

        // TODO: Pool these allocations in an object pool.
        AsyncOp* op = Allocator::GetDefault().New<AsyncOp>();

        if (!op->Reset(fd, callback))
        {
            io_uring_sqe* sqe = GetNextSqe();
            io_uring_prep_msg_ring(sqe, s_ring.ring_fd, -static_cast<unsigned int>(errno), op, 0);
        }
        else
        {
            io_uring_sqe* sqe = GetNextSqe();
            io_uring_prep_read(sqe, op->File(), dst, size, offset);
            io_uring_sqe_set_data(sqe, op);
        }

        return *op;
    }

    AsyncFileOp AsyncFile::WriteAsync(const void* src, uint64_t offset, uint32_t size, CompleteDelegate callback)
    {
        const int fd = static_cast<int>(m_fd);

        // TODO: Pool these allocations in an object pool.
        AsyncOp* op = Allocator::GetDefault().New<AsyncOp>();

        if (!op->Reset(fd, callback))
        {
            io_uring_sqe* sqe = GetNextSqe();
            io_uring_prep_msg_ring(sqe, s_ring.ring_fd, -static_cast<unsigned int>(errno), op, 0);
        }
        else
        {
            io_uring_sqe* sqe = GetNextSqe();
            io_uring_prep_write(sqe, op->File(), src, size, offset);
            io_uring_sqe_set_data(sqe, op);
        }

        return *op;
    }

    Result AsyncFile::GetAttributes(FileAttributes& outAttributes) const
    {
        const int fd = static_cast<int>(m_fd);
        return PosixFileGetAttributes(fd, outAttributes);
    }

    Result AsyncFile::GetPath(String& outPath) const
    {
        const int fd = static_cast<int>(m_fd);
        return PosixFileGetPath(fd, outPath);
    }

    // --------------------------------------------------------------------------------------------
    static void CompletionQueueThread()
    {
        SetCurrentThreadName("[HE] Async File io_uring CQ Thread");

        while (s_ioStartupCount > 0)
        {
            io_uring_cqe* cqe = nullptr;
            const int rc = io_uring_wait_cqe(&s_ring, &cqe);

            if (rc != 0)
            {
                HE_LOG_ERROR(he_core,
                    HE_MSG("Failed to get io_uring CQ entry."),
                    HE_KV(result, PosixResult(-rc)));
                continue;
            }

            HE_ASSERT(cqe);

            AsyncOp* op = static_cast<AsyncOp*>(io_uring_cqe_get_data(cqe));
            if (op)
                op->SetResult(cqe->res);

            io_uring_cqe_seen(&s_ring, cqe);
        }
    }
}

#endif
