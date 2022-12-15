// Copyright Chad Engler

#pragma once

#include "he/core/file.h"
#include "he/core/task_executor.h"
#include "he/core/types.h"

namespace he
{
    /// Configuration for the async file IO system.
    struct AsyncFileIOConfig
    {
        /// Configuration for the IOCP backend used on win32 systems.
        struct
        {
            /// Affinity to use for the IOCP thread. This is the thread that will invoke callbacks
            /// for compelted asynchronous operations.
            /// Only considered when non-zero.
            uint64_t threadAffinity{ 0 };
        } iocp;

        /// Configuration for the io_uring backend used on linux systems.
        struct
        {
            /// The maximum number of requests the io_uring queue can hold. The ReadAsync and
            /// WriteAsync functions will stall until an additional slot is available if this
            /// capacity is reached.
            /// Value must be in the range [128, 8192].
            uint16_t capacity{ 256 };

            /// Affinity to use for the completion queue thread. This is the thread that will
            /// invoke callbacks for compelted asynchronous operations.
            /// Only considered when non-zero.
            uint64_t threadAffinity{ 0 };
        } iouring;

        /// Configuration for the thread pool backend used on posix systems.
        struct
        {
            /// Used by the backend to schedule IO operations. Work performed on this executor
            /// will primarily be blocking waits on system IO functions and executing user callbacks.
            /// If this value is set then no additional threads are spawned and the \ref threadCount
            /// and \ref threadAffinity values are ignored.
            TaskExecutor* executor{ nullptr };

            /// The number of threads to spawn for handling IO operations. Specifying zero (default)
            /// will choose a reasonable value based on available system resources.
            /// Only considered when `executor` is null.
            uint32_t threadCount{ 0 };

            /// Affinity to use for the worker threads. This is the thread that will make blocking
            /// system IO calls and invoke callbacks for compelted asynchronous operations.
            /// Only considered when non-zero and `executor` is null.
            uint64_t threadAffinity{ 0 };
        } threadpool;
    };

    /// Starts up the asynchronous file handling for the platform. Must be called before performing
    /// any asynchronous file operations. Calling this function multiple times, or from different
    /// threads, is safe as long as each call to this function has a matching call to
    /// \ref ShutdownAsyncFileIO.
    ///
    /// \note Only the configuration of the first call is used, extra calls to this function will
    /// increment a refcount but will not modify existing initialized state.
    ///
    /// \param config The configuration to use for starting up the async backend.
    /// \return The result of the startup operation.
    Result StartupAsyncFileIO(const AsyncFileIOConfig& config);

    /// Shuts down the asynchronous file operation system. Safe to call while pending file
    /// operations are in progress, but may stall until all of them complete.
    ///
    /// This should be called exactly once for each call to \ref StartupAsyncFileIO.
    void ShutdownAsyncFileIO();

    /// Token for tracking the status of a single asynchronous file operation.
    struct AsyncFileOp { uintptr_t val; };

    /// Represents a file on disk that can be read from or written to asynchronously.
    class AsyncFile
    {
    public:
        /// Delegate for a callback that is invoked when an async operation completes.
        using CompleteDelegate = Delegate<void(AsyncFileOp token)>;

        /// Returns true if the requests the token is tracking have completed.
        ///
        /// \param[in] token The token to check.
        /// \return True if the requests the token is tracking have completed, false otherwise.
        [[nodiscard]] static bool IsComplete(AsyncFileOp token);

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
        /// \param[out] bytesTransferred Optional. Set to the number of bytes transferred during
        ///     the operation.
        /// \return The result of the operation.
        static Result GetResult(AsyncFileOp token, uint32_t* bytesTransferred = nullptr);

    public:
        AsyncFile() noexcept;
        AsyncFile(const AsyncFile&) = delete;
        AsyncFile(AsyncFile&& x) noexcept;
        ~AsyncFile() noexcept;

        AsyncFile& operator=(const AsyncFile&) = delete;
        AsyncFile& operator=(AsyncFile&& x) noexcept;

        /// Opens the file at `path` in the given `mode` using behavior defined by `flags`.
        ///
        /// \param[in] path The filesystem path to open.
        /// \param[in] mode The mode to open the file in.
        /// \param[in] flags The behavior flags for operations on this file.
        /// \return The result of the operation.
        Result Open(const char* path, FileOpenMode mode, FileOpenFlag flags = FileOpenFlag::None);

        /// Closes the file.
        void Close();

        /// Checks if the file is currently open.
        ///
        /// \return True if the file is open, false otherwise.
        bool IsOpen() const;

        /// Reads the size of the file.
        ///
        /// \return The size of the file in bytes.
        uint64_t GetSize() const;

        /// Starts an asynchronous read of the open file and returns a token that can be used to
        /// poll the status of the pending operation. An optional callback can also be passed
        /// which will be invoked when the read has completed.
        ///
        /// \note You must call \ref GetResult with the token returned here, or passed into the
        /// callback, in order to check if the loads succeeded and to cleanup internal resources.
        ///
        /// \note It is safe to close a file while there is a pending read operation in progress.
        ///
        /// \param[out] dst The destination buffer to write data into.
        /// \param[in] offset The offset, in bytes, at which to begin reading the file.
        /// \param[in] size The number of bytes to read from the file.
        /// \param[in] callback Optional. Callback to invoke once the read has completed.
        /// \return A future representing the asynchronous read operation.
        AsyncFileOp ReadAsync(void* dst, uint64_t offset, uint32_t size, CompleteDelegate callback = {});

        /// Starts an asynchronous write of the open file and returns a token that can be used to
        /// poll the status of the pending operation. An optional callback can also be passed
        /// which will be invoked when the read has completed.
        ///
        /// \note You must call \ref GetResult with the token returned here, or passed into the
        /// callback, in order to check if the loads succeeded and to cleanup internal resources.
        ///
        /// \note It is safe to close a file while there is a pending read operation in progress.
        ///
        /// \param[in] src The source buffer to write into the file.
        /// \param[in] offset The offset, in bytes, at which to begin writing into the file.
        /// \param[in] size The number of bytes to write to the file.
        /// \param[in] callback Optional. Callback to invoke once the write has completed.
        /// \return A future representing the asynchronous write operation.
        AsyncFileOp WriteAsync(const void* src, uint64_t offset, uint32_t size, CompleteDelegate callback = {});

        // Fills `attributes` with the attributes of the file at `path`. If this returns a
        // non-successful result then `attributes` will not contain valid values.
        Result GetAttributes(FileAttributes& outAttributes) const;

        // Returns the full path to file
        Result GetPath(String& outPath) const;

    public:
        intptr_t m_fd;
    };
}
