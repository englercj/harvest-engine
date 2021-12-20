// Copyright Chad Engler

#pragma once

#include "he/core/file.h"
#include "he/core/task_executor.h"
#include "he/core/types.h"

#include <future>

namespace he
{
    // Configuration for the async file IO system.
    struct AsyncFileIOConfig
    {
        /// Used by the Posix backend to schedule IO operations. Work executed on this executor
        /// will primarily be blocking waits on system IO functions.
        TaskExecutor* executor{ nullptr };

        // TODO: Thread priority and affinity
    };

    struct AsyncFileResult
    {
        Result result;
        uint32_t bytesTransferred;
    };

    // Must be called before performing any asynchronous file operations.
    Result StartupAsyncFileIO(const AsyncFileIOConfig& config);

    // Shuts down the asynchronous file operation system. Safe to call while pending file
    // operations are in progress, but will cause a stall until all of them complete.
    void ShutdownAsyncFileIO();

    // Represents a file that is used for asynchronous read/write operations.
    class AsyncFile
    {
    public:
        AsyncFile();
        AsyncFile(const AsyncFile&) = delete;
        AsyncFile(AsyncFile&& x);
        ~AsyncFile();

        AsyncFile& operator=(const AsyncFile&) = delete;
        AsyncFile& operator=(AsyncFile&& x);

        // Opens the file from the `path` in the given `mode`.
        Result Open(const char* path, FileOpenMode mode, FileOpenFlag flags = FileOpenFlag::None);

        // Closes the file.
        void Close();

        // Returns true if the file is currently open.
        bool IsOpen() const;

        // Returns the size of the file.
        uint64_t GetSize() const;

        // Reads `size` bytes from the file at `offset` into `dst` and stores the count of
        // `bytesRead`.
        // Note: It is safe to close a file while there is a pending read operation in progress.
        std::future<AsyncFileResult> ReadAsync(void* dst, uint64_t offset, uint32_t size);

        // Writes `size` bytes from `src` at `offset` and stores the count of `bytesWritten`.
        // Note: It is safe to close a file while there is a pending write operation in progress.
        std::future<AsyncFileResult> WriteAsync(void* src, uint64_t offset, uint32_t size);

    public:
        intptr_t m_fd;
    };
}
