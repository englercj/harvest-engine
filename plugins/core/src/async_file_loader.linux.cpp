// Copyright Chad Engler

#include "he/core/async_file_loader.h"

#if defined(HE_PLATFORM_LINUX)

#include "file_helpers.posix.h"

#include <linux/io_uring.h>

namespace he
{
    class AsyncFileQueueImpl final : public AsyncFileQueue
    {
    public:
        void EnqueueRequest(const AsyncFileRequest& req) override
        {

        }

        void EnqueueDelegate(LoadDelegate callback) override
        {

        }

        AsyncFileTracker EnqueueTracker() override
        {

        }

        void CancelRequestsWithTag(uint64_t mask, uint64_t value) override
        {

        }

        void Submit() override
        {

        }

        bool IsComplete(AsyncFileTracker token) const override
        {

        }

        Result GetResult(AsyncFileTracker token) const override
        {

        }
    };

    class AsyncFileLoaderImpl final : public AsyncFileLoader
    {
    public:
        Result Initialize(const Config& config) override
        {

        }

        void Terminate() override
        {

        }

        Result OpenFile(const char* path, AsyncFileId& fd) override
        {

        }

        void CloseFile(AsyncFileId fd) override
        {

        }

        Result GetAttributes(AsyncFileId fd, FileAttributes& outAttributes) override
        {

        }

        AsyncFileQueue* DefaultQueue() override
        {

        }

        Result CreateQueue(const AsyncFileQueue::Config& config, AsyncFileQueue*& out) override
        {

        }

        void DestroyQueue(AsyncFileQueue* queue) override
        {
            Allocator::GetDefault().Delete(queue);
        }

    private:
        AsyncFileQueue* m_defaultQueue{ nullptr };
    };
}

#endif
