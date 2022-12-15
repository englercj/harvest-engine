// Copyright Chad Engler

// TODO: Support rhi devices and allow gpu resources as targets for the loads

#pragma once

#include "he/core/allocator.h"
#include "he/core/delegate.h"
#include "he/core/file.h"
#include "he/core/result.h"
#include "he/core/task_executor.h"
#include "he/core/types.h"

namespace he
{
    /// Token for tracking an open file in the bulk file loader.
    struct BulkFileId { uintptr_t val; };

    /// Token for tracking the status of a single asynchronous file operation.
    struct BulkReadId { uintptr_t val; };

    /// Descriptor for a read request that can be enqueued on a bulk file queue.
    struct BulkReadRequest
    {
        /// Compression formats supported by the loader. Not all of these work on all platforms.
        enum class CompressionFormat : uint8_t
        {
            None,       ///< No compression
            Zlib,       ///< Compressed with zlib.
            GDeflate,   ///< Compressed using the GDeflate algorithm for GPU decompression.
        };

        /// The file to perform this read request from.
        BulkFileId file{};

        /// The offset, in bytes, in the file to start the read request at.
        uint64_t offset{ 0 };

        /// Number of bytes to read from the file.
        /// If using compression this is the compressed size of the data being read.
        uint32_t size{ 0 };

        /// Address of the buffer to write the result of the request into.
        void* dst{ nullptr };

        /// Number of bytes in the destination buffer.
        /// If using compression this is the uncompressed size of the data being read.
        uint32_t dstSize{ 0 };

        /// Indicates how the data is compressed in the file.
        CompressionFormat compression{ CompressionFormat::None };

        /// The debug name of the request.
        /// If specified, the string must be accessible until the request completes.
        const char* name{ nullptr };
    };

    /// A bulk file queue is a queue of file read requests that can be submitted and tracked in
    /// batches. A queue is not thread safe and should only be used from a single thread at a time.
    class BulkFileQueue
    {
    public:
        /// Configuration options for creating a bulk file queue.
        struct Config
        {
            /// Priority of requests that are send on this queue.
            enum class Priority : uint8_t
            {
                Low,
                Normal,
                High,
                Realtime,
            };

            /// The priority of the requests in this queue.
            Priority priority{ Priority::Normal };

            /// The maximum number of requests the queue can hold. The Enqueue functions will
            /// stall until an additional slot is available if this capacity is reached.
            /// Value must be in the range [128, 8192].
            uint16_t capacity{ 1024 };

            /// The debug name of the queue.
            const char* name{ nullptr };
        };

        /// Delegate for a callback that can be called when requests complete.
        using LoadDelegate = Delegate<void(BulkReadId id)>;

    public:
        /// Destructs a bulk file queue.
        virtual ~BulkFileQueue() = default;

        /// Enqueues a read request to the queue. The request remains in the queue until \ref Submit
        /// is called, or until the queue is half full. If there are no free entries in the queue
        /// the enqueue operation will block until one becomes available.
        ///
        /// \param[in] req The request to enqueue.
        virtual void Enqueue(const BulkReadRequest& req) = 0;

        /// Submits enqueued requests and returns a token that can be used to poll the status
        /// of the pending reads since the last call to Submit. An optional callback can also be
        /// passed which will be invoked when the requests have completed.
        ///
        /// \note You must call \ref GetResult with the token returned here, or passed into the
        /// callback, in order to check if the loads succeeded and to cleanup internal resources.
        ///
        /// \note It is safe to close a file while there is a pending read operation in progress.
        ///
        /// \param[in] callback Optional. Callback to invoke once loading has completed.
        /// \return A tracking token that can be used to poll the status of the read requests.
        virtual BulkReadId Submit(LoadDelegate callback = {}) = 0;

        /// Returns true if the requests the token is tracking have completed.
        ///
        /// \param[in] token The token to check.
        /// \return True if the requests the token is tracking have completed, false otherwise.
        [[nodiscard]] virtual bool IsComplete(BulkReadId token) const = 0;

        /// Returns the result of the requests the token is tracking. If the requests have not
        /// yet completed, this function will stall until they do. Calling this function
        /// invalidates the token and it can no longer be used. As such, this function should be
        /// called exactly once for any token either during a polling loop or from the callback
        /// passed into \ref Submit; but not both.
        ///
        /// \note Calling this function cleans up internal tracking resources and therefore must be
        /// called exactly once for each \ref Submit call.
        ///
        /// \param[in] token The token to get the result of.
        /// \return The result of the requests. If there was a failure, this result is the first
        ///     failure encountered.
        virtual Result GetResult(BulkReadId token) const = 0;
    };

    /// The bulk file loader is a pipelined asynchronous file loading interface that can make
    /// better use of high-speed storage devices when loading many pieces of data at once.
    class BulkFileLoader
    {
    public:
        /// Configuration options for creating a bulk file loader.
        struct Config
        {
            /// The allocator to use for all allocations done by the loader, including
            /// the creation of the loader itself.
            Allocator* allocator{ &Allocator::GetDefault() };

            /// Configuration for the DirectStorage backend.
            struct
            {
                /// Sets the number of threads to use for submitting IO operations.
                /// Specifying zero (default) means the system will use a best guess at a good value.
                uint32_t submitThreadCount{ 0 };

                /// Sets the number of threads to be used by the DirectStorage runtime to
                /// decompress data using the CPU for built-in compressed formats that cannot
                /// be decompressed using the GPU.
                /// Specifying zero (default) means the system will use a best guess at a good value.
                uint32_t decompressThreadCount{ 0 };
            } dstorage{};
        };

    public:
        /// Creates an bulk file loader instance.
        ///
        /// \param[in] config Configuration to initialize the loader with.
        /// \param[out] out A pointer to the newly created loader.
        /// \return The result of the operation.
        static Result Create(const Config& config, BulkFileLoader*& out);

        /// Destroys an instance that was created with \ref Create
        ///
        /// \param[in] loader The loader to destroy.
        static void Destroy(BulkFileLoader* loader);

    public:
        /// Destructs the bulk file loader.
        virtual ~BulkFileLoader() = default;

        /// Gets the allocator used for all allocations in this device.
        ///
        /// \return The allocator.
        Allocator& GetAllocator() { return m_allocator; }

        /// Opens a file for use with the bulk file loader.
        ///
        /// \param[in] path The path to the file to open
        /// \param[out] fd The file ID for the file that was opened. Only valid when
        ///     `Result::Success` is returned.
        /// \return The result of opening the file.
        virtual Result OpenFile(const char* path, BulkFileId& fd) = 0;

        /// Closes a file ID so it can no longer used by the loader.
        ///
        /// \note Loads that were in progress before the file was closed will still complete normally.
        ///
        /// \param[in] fd The file ID to close.
        virtual void CloseFile(BulkFileId fd) = 0;

        /// Gets the attributes of an open file ID.
        ///
        /// \param[in] fd The file ID to get the attributes for.
        /// \param[out] outAttributes The attributes structure to fill with data.
        /// \return The result of the operation.
        virtual Result GetAttributes(BulkFileId fd, FileAttributes& outAttributes) = 0;

        /// Creates a queue that can be used to issue file load requests.
        ///
        /// \param[in] config The configuration for creating the queue.
        /// \param[out] out The queue that was created. Only valid when `Result::Success` is returned.
        /// \return The result of the create operation.
        virtual Result CreateQueue(const BulkFileQueue::Config& config, BulkFileQueue*& out) = 0;

        /// Destroys a queue that was created with \ref CreateQueue
        ///
        /// \param[in] queue The queue to destroy.
        virtual void DestroyQueue(BulkFileQueue* queue) = 0;

    protected:
        /// Constructs a bulk file loader.
        /// Only intended to be called by platform implementations.
        BulkFileLoader(Allocator& allocator) : m_allocator(allocator) {}

        /// Called to initialize the loader.
        /// Only intended to be called by Create()
        virtual Result Initialize(const Config& config) = 0;

    protected:
        /// Allocator to be used for all allocations of the loader.
        Allocator& m_allocator;
    };
}
