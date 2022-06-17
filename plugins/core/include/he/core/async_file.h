// Copyright Chad Engler

#pragma once

#include "he/core/file.h"
#include "he/core/task_executor.h"
#include "he/core/types.h"

#include <future>

namespace he
{
    /// Configuration for the async file IO system.
    struct AsyncFileIOConfig
    {
        /// Used by the Posix backend to schedule IO operations. Work executed on this executor
        /// will primarily be blocking waits on system IO functions.
        TaskExecutor* executor{ nullptr };

        /// Configuration for the thread pool executor which is used on posix systems.
        /// If `executor` is set, that object is used, and these values are ignored.
        struct
        {
            uint32_t threadCount{ 0 }; ///< A value of zero will choose a number of threads between 4 and 8 based on hardware.
            uint64_t threadAffinity{ 0 }; ///< Affinity to use for spawned threads. Only considered when non-zero.
        } pool;

        /// Configuration for the IOCP backend used on win32 systems.
        struct
        {
            uint64_t threadAffinity{ 0 }; ///< Affinity to use for the IOCP thread. Only considered when non-zero.
        } iocp;
    };

    /// Result of an asynchronous file operation.
    struct AsyncFileResult
    {
        /// The result of the operation. When this represents a failure, `bytesTransferred`
        /// may contain an invalid value.
        Result result;

        /// Number of bytes read from or written to the file.
        uint32_t bytesTransferred;
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

    /// Represents a file on disk that can be read from or written to asynchronously.
    class AsyncFile
    {
    public:
        AsyncFile();
        AsyncFile(const AsyncFile&) = delete;
        AsyncFile(AsyncFile&& x);
        ~AsyncFile();

        AsyncFile& operator=(const AsyncFile&) = delete;
        AsyncFile& operator=(AsyncFile&& x);

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

        /// Reads `size` bytes from the file at `offset` into `dst`.
        ///
        /// \note It is safe to close a file while there is a pending read operation in progress.
        ///
        /// \param[out] dst The destination buffer to write data into.
        /// \param[in] offset The offset, in bytes, at which to begin reading the file.
        /// \param[in] size The number of bytes to read from the file.
        /// \return A future representing the asynchronous read operation.
        std::future<AsyncFileResult> ReadAsync(void* dst, uint64_t offset, uint32_t size);

        /// Writes `size` bytes from `src` into the file at `offset`.
        ///
        /// \note It is safe to close a file while there is a pending write operation in progress.
        ///
        /// \param[in] src The source buffer to write into the file.
        /// \param[in] offset The offset, in bytes, at which to begin writing into the file.
        /// \param[in] size The number of bytes to write to the file.
        /// \return A future representing the asynchronous write operation.
        std::future<AsyncFileResult> WriteAsync(const void* src, uint64_t offset, uint32_t size);

        // Fills `attributes` with the attributes of the file at `path`. If this returns a
        // non-successful result then `attributes` will not contain valid values.
        Result GetAttributes(FileAttributes& outAttributes) const;

        // Returns the full path to file
        Result GetPath(String& outPath) const;

    public:
        intptr_t m_fd;
    };
}
