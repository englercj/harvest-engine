// Copyright Chad Engler

#include "he/core/async_file_loader.h"

#include "he/core/assert.h"
#include "he/core/cpu.h"
#include "he/core/log.h"

#include <thread>
#include <utility>

#if defined(HE_PLATFORM_LINUX)

#include "file_helpers.posix.h"

#include <liburing.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <liburing/io_uring.h>
#include <sys/utsname.h>

namespace he
{
    static std::pair<int, const char*> RequiredOps[] =
    {
        { IORING_OP_NOP, "IORING_OP_NOP" },
        { IORING_OP_READ, "IORING_OP_READ" },
    };

    static bool CheckUringFeatures()
    {
        io_uring_probe* probe = io_uring_get_probe();

        for (const std::pair<int, const char*>& op : RequiredOps)
        {
            if (!io_uring_opcode_supported(probe, op.first))
            {
                HE_LOG_ERROR(he_core,
                    HE_MSG("Required io_uring operation is not supported."),
                    HE_KV(op_name, op.second),
                    HE_KV(op_code, op.first));

                free(probe);
                return false;
            }
        }

        free(probe);
        return true;
    }

    class AsyncFileQueueImpl final : public AsyncFileQueue
    {
    private:
        struct ReadGroup
        {
            uint32_t done{ 0 };
            uint32_t total{ 0 };
            Callback cb{};
        };

    public:
        ~AsyncFileQueueImpl()
        {
            m_cqRun = false;

            io_uring_sqe* sqe = GetSqe();
            io_uring_prep_msg_ring(sqe, m_ring.ring_fd, 0, 0, 0);

            if (m_cqThread.joinable())
                m_cqThread.join();

            io_uring_queue_exit(&m_ring);

            m_allocator.Free(byteCount);
        }

        Result Initialize(const Config& config)
        {
            m_capacity = config.capacity;

            const uint32_t byteCount = (config.capacity + 7) / 8;
            m_statusBits = m_allocator.Malloc<uint8_t>(byteCount);
            MemZero(m_statusBits, byteCount);

            const int rc = io_uring_queue_init(config.capacity, &m_ring, 0);
            if (rc < 0)
                return PosixResult(-rc);

            m_cqThread = std::thread(std::bind(&AsyncFileQueueImpl::PumpCompletionQueue, this));
        }

        void EnqueueRequest(const AsyncFileRequest& req) override
        {
            const int fd = static_cast<int>(req.file.val);

            HE_ASSERT(req.size <= req.dstSize, HE_MSG("Size of the destination buffer must fit the requested data"));
            HE_ASSERT(req.compression == CompressionFormat::None, HE_MSG("Compression is not yet supported for the Linux io_uring implementation."));
            HE_ASSERT(req.cancellationTag == 0, HE_MSG("Cancellation tags are not yet supported for the Linux io_uring implementation."));

            if (!m_activeGroup)
            {
                m_activeGroup = m_allocator.New<ReadGroup>();
            }

            ++m_activeGroup->total;

            io_uring_sqe* sqe = GetSqe();
            io_uring_prep_read(sqe, fd, req.dst, req.size, req.offset);
            io_uring_sqe_set_data(sqe, m_activeGroup);
        }

        void EnqueueDelegate(LoadDelegate callback) override
        {
            if (!callback)
                return;

            if (!m_activeGroup)
            {
                m_activeGroup = m_allocator.New<ReadGroup>();
            }

            m_activeGroup->cb = callback;

            io_uring_sqe* sqe = GetSqe();
            io_uring_prep_msg_ring(sqe, m_ring.ring_fd, 0, p, 0);

            m_lastIndex = m_index;
        }

        AsyncFileTracker EnqueueTracker() override
        {
            // TODO
        }

        void CancelRequestsWithTag(uint64_t mask, uint64_t value) override
        {
            // TODO
        }

        void Submit() override
        {
            io_uring_submit(ring);
        }

        bool IsComplete(AsyncFileTracker token) const override
        {
            // TODO
        }

        Result GetResult(AsyncFileTracker token) const override
        {
            // TODO
        }

    private:
        io_uring_sqe* SpinWaitForSqe()
        {
            constexpr uint32_t SpinIterations = 10;

            for (uint32_t i = 0; i < SpinIterations; ++i)
            {
                sqe = io_uring_get_sqe(&m_ring);
                if (sqe)
                    return sqe;

                HE_SPIN_WAIT_PAUSE();
            }

            return nullptr;
        }

        io_uring_sqe* GetSqe()
        {
            io_uring_sqe* sqe = io_uring_get_sqe(&m_ring);

            while (sqe == nullptr) [[unlikely]]
            {
                sqe = SpinWaitForSqe();

                // If we didn't get one, wait for one entry to complete and then try again
                if (!sqe)
                {
                    io_uring_submit_and_wait(&m_ring, 1);
                }
            }

            return sqe;
        }

        void SetStatus(uint32_t index)
        {
            uint8_t* b = m_statusBits + (index / 8);
            const uint32_t shift = index % 8;
            *b |= (1 << shift);
        }

        void ClearStatus(uint32_t index)
        {
            uint8_t* b = m_statusBits + (index / 8);
            const uint32_t shift = index % 8;
            *b &= ~(1 << shift);
        }

        void IsStatusSet(uint32_t index)
        {
            uint8_t* b = m_statusBits + (index / 8);
            const uint32_t shift = index % 8;
            return (*b & (1 << shift)) != 0;
        }

        void PumpCompletionQueue()
        {
            while (m_cqRun)
            {
                io_uring_cqe* cqe;
                const int rc = io_uring_wait_cqe(&m_ring, &cqe);

                HE_AT_SCOPE_EXIT([&]() { io_uring_cqe_seen(&m_ring, cqe); });

                if (rc != 0)
                {
                    HE_LOG_ERROR(he_core,
                        HE_MSG("Failed to get Completion Queue Entry."),
                        HE_KV(result, PosixResult(-rc)));
                    continue;
                }

                if (cqe->user_data == 0)
                    continue;

                Pending* p = static_cast<Pending*>(io_uring_cqe_get_data(cqe));
                if (p->isRead)
                {
                    // TODO
                    // if (p->res == 0)
                    SetStatus(p->read.index);
                }
                else
                {
                    // ugh I don't like this...
                }
            }
        }

    private:
        io_uring m_ring{};
        uint8_t* m_statusBits{ nullptr };
        ReadGroup* m_activeGroup{ nullptr };

        std::thread m_cqThread{};
        std::atomic<bool> m_cqRun{ false };
    };

    class AsyncFileLoaderImpl final : public AsyncFileLoader
    {
    public:
        AsyncFileLoaderImpl(Allocator& allocator) : AsyncFileLoader(allocator) {}

        ~AsyncFileLoaderImpl()
        {
            Allocator::GetDefault().Delete(m_defaultQueue);
            GlobalTerminate();
        }

        Result Initialize(const Config& config) override
        {
            if (!CheckUringFeatures())
                return Result::NotSupported;
        }

        Result OpenFile(const char* path, AsyncFileId& fd) override
        {
            const int file = PosixFileOpen(path, FileOpenMode::ReadExisting, FileOpenFlag::None, 0);
            if (file == -1)
                return Result::FromLastError();

            fd.val = static_cast<uintptr_t>(file);
            return Result::Success;
        }

        void CloseFile(AsyncFileId fd) override
        {
            const int file = static_cast<int>(fd.val);
            close(file);
        }

        Result GetAttributes(AsyncFileId fd, FileAttributes& outAttributes) override
        {
            const int file = static_cast<int>(fd.val);
            return PosixFileGetAttributes(file, outAttributes);
        }

        AsyncFileQueue* DefaultQueue() override
        {
            return m_defaultQueue;
        }

        Result CreateQueue(const AsyncFileQueue::Config& config, AsyncFileQueue*& out) override
        {
            AsyncFileQueueImpl* queue = Allocator::GetDefault().New<AsyncFileQueueImpl>();
            Result r = queue->Initialize(config);
            if (!r)
            {
                Allocator::GetDefault().Delete(queue);
                queue = nullptr;
            }

            out = queue;
            return r;
        }

        void DestroyQueue(AsyncFileQueue* queue) override
        {
            if (queue != m_defaultQueue)
            {
                Allocator::GetDefault().Delete(queue);
            }
        }

    private:
        AsyncFileQueue* m_defaultQueue{ nullptr };
    };
}

#endif
