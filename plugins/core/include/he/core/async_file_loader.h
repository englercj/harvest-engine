// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/delegate.h"
#include "he/core/file.h"
#include "he/core/result.h"
#include "he/core/task_executor.h"
#include "he/core/types.h"

namespace he
{
    /// Token for tracking the status of a single asynchronous file operation.
    struct AsyncFileTracker { uint32_t val; };

    /// Token for tracking an open file in the async loader.
    struct AsyncFileId { uintptr_t val; };

    /// Request object that can be enqueue for a read.
    struct AsyncFileRequest
    {
        /// Compression formats supported by the loader. Not all of these work on all platforms.
        enum class CompressionFormat : uint8_t
        {
            None,
            Zlib,
        };

        /// The file to perform this read request from.
        AsyncFileId file;

        /// The offset, in bytes, in the file to start the read request at.
        uint64_t offset;

        /// Number of bytes to read from the file.
        uint32_t size;

        /// Address of the buffer to write the result of the request into.
        void* dst;

        /// Number of bytes to write to the destination buffer.
        uint32_t dstSize;

        /// Indicates how the data is compressed in the file.
        CompressionFormat compression{ CompressionFormat::None };

        /// An arbitrary number used for cancellation matching.
        uint64_t cancellationTag{ 0 };

        /// The debug name of the request.
        /// If specified, the string must be accessible until the request completes.
        const char* name{ nullptr };
    };

    /// Interface for a buffer that collects read commands before submitting them all at once.
    /// Sending read commands to the \ref AsyncFileLoader in a batch can improve data throughput
    /// when you have many read commands to submit.
    class AsyncFileQueue
    {
    public:
        /// Configuration options for creating an async file queue.
        struct Config
        {
            /// The maximum number of requests the queue can hold. The Enqueue functions will
            /// stall until an additional slot is available if this capacity is reached.
            /// Value must be in the range [128, 8192].
            uint16_t capacity{ 512 };

            /// The debug name of the queue.
            /// Only affects DirectStorage on Win32 platforms.
            const char* name{ nullptr };
        };

        /// Function pointer for a callback that can be called when requests complete.
        using LoadDelegate = Delegate<void(Result result)>;

    public:
        virtual ~AsyncFileQueue() = default;

        /// Enqueues a read request to the queue. The request remains in the queue until Submit
        /// is called, or until the queue is 1/2 full. If there are no free entries in the queue
        /// the enqueue operation will block until one becomes available.
        ///
        /// \param[in] req The request to enqueue.
        virtual void EnqueueRequest(const AsyncFileRequest& req) = 0;

        /// Enqueues a callback to called when all requests before the callback are complete.
        /// If there was a failure since the previous callback or tracker, then the result passed
        /// to the callback is the first failed request (in enqueued order).
        ///
        /// \note The callback is invoked from a background thread managed by the loader. You
        /// should not perform time consuming work in this callback, as it will block this
        /// background thread and prevent others from getting called.
        ///
        /// \param[in] callback The delegate to invoke when all previous requests complete.
        virtual void EnqueueDelegate(LoadDelegate callback) = 0;

        /// Enqueues a tracker token that can be polled for completion. The tracker is marked
        /// as complete once all requests before the tracker have completed.
        /// If there was a failure since the previous callback or tracker, then the result returned
        /// from GetResult() is the first failed request (in enqueued order).
        virtual AsyncFileTracker EnqueueTracker() = 0;

        /// Attempts to cancel a group of previously enqueued read requests. All previously
        /// enqueued requests whose `cancellationTag` matches the formula
        /// `(cancellationTag & mask) == value` will be cancelled.
        /// A cancelled request may or may not complete its original read request.
        /// A cancelled request is not counted as a failure.
        ///
        /// \param[in] mask The mask for the cancellation formula.
        /// \param[in] value The value for the cancellation formula.
        virtual void CancelRequestsWithTag(uint64_t mask, uint64_t value) = 0;

        /// Submits all requests enqueued so far to be executed.
        virtual void Submit() = 0;

        /// Returns true if the requests the token is tracking have completed.
        ///
        /// \param[in] token The tracker to check.
        /// \return True if the requests the token is tracking have completed, false otherwise.
        virtual [[nodiscard]] bool IsComplete(AsyncFileTracker token) const = 0;

        /// Returns the result of the requests the token is tracking. If the requests have not
        /// yet completed, this function will stall until they do.
        /// Calling this function invalidates the tracker token and it can no longer be used.
        ///
        /// \param[in] token The tracker to get the result of.
        /// \return The result of the requests. If there was a failure, this result is the first
        /// failure encountered.
        virtual Result GetResult(AsyncFileTracker token) const = 0;
    };

    /// Interface for an async file loader. This class wraps platform-specific asynchronous
    /// file loading pipelines like DirectStorage and io_uring.
    class AsyncFileLoader
    {
    public:
        /// Configuration options for creating an async file loader.
        struct Config
        {
            /// The allocator to use for all allocations done by the loader, including
            /// the creation of the loader itself.
            Allocator* allocator{ &Allocator::GetDefault() };

            /// Sets the number of system threads to use for submitting IO operations.
            /// Specifying 0 means the system will use a best guess at a good value.
            /// Only affects DirectStorage on Win32 platforms.
            uint32_t submitThreadCount{ 0 };

            /// Affinity to use for the callback thread. Only considered when non-zero.
            uint64_t callbackThreadAffinity{ 0 };

            /// Configuration for the default queue that is created for this loader.
            /// You can set the capacity to zero to skip creation of the default queue.
            AsyncFileQueue::Config defaultQueue{ 512, "Default Queue" };
        };

    public:
        /// Creates an async file loader instance.
        ///
        /// \param[in] config Configuration to initialize the loader with.
        /// \param[out] out A pointer to the newly created loader.
        /// \return The result of the operation.
        static Result Create(const AsyncFileLoader::Config& config, AsyncFileLoader*& out);

        /// Destroys an instance that was created with \ref CreateLoader
        ///
        /// \param[in] loader The loader to destroy.
        static void Destroy(AsyncFileLoader* updater);

    public:
        virtual ~AsyncFileLoader() = default;

        /// Gets the allocator used for all allocations in this device.
        ///
        /// \return The allocator.
        Allocator& GetAllocator() { return m_allocator; }

        virtual Result OpenFile(const char* path, AsyncFileId& fd) = 0;
        virtual void CloseFile(AsyncFileId fd) = 0;

        virtual Result GetAttributes(AsyncFileId fd, FileAttributes& outAttributes) = 0;

        virtual AsyncFileQueue* DefaultQueue() = 0;

        virtual Result CreateQueue(const AsyncFileQueue::Config& config, AsyncFileQueue*& out) = 0;
        virtual void DestroyQueue(AsyncFileQueue* queue) = 0;

    protected:
        AsyncFileLoader(Allocator& allocator) : m_allocator(allocator) {}

        virtual Result Initialize(const Config& config) = 0;

    protected:
        Allocator& m_allocator;
    };
}
