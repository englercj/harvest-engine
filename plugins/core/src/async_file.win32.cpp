// Copyright Chad Engler

#include "he/core/async_file.h"

#include "he/core/assert.h"
#include "he/core/log.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/thread.h"

#include <future>
#include <mutex>

#if defined(HE_PLATFORM_API_WIN32)

#include "file_helpers.win32.h"

#include "he/core/win32_min.h"

#include <fileapi.h>

namespace he
{
    static std::mutex s_ioStartupMutex{};
    static uint32_t s_ioStartupCount{ 0 };
    static HANDLE s_ioThread{ nullptr };
    static HANDLE s_ioPort{ nullptr };

    struct AsyncOp
    {
        HANDLE file{ INVALID_HANDLE_VALUE };
        OVERLAPPED overlap{};
        std::promise<AsyncFileResult> promise{};
    };

    static void HandleCompletedOp(AsyncOp* op, Result result, uint32_t bytesTransferred);
    static void HandleCompletedOverlap(AsyncOp* op);
    static DWORD IOCompletionThread(LPVOID);

    Result StartupAsyncFileIO(const AsyncFileIOConfig& config)
    {
        std::lock_guard<std::mutex> lock(s_ioStartupMutex);

        const uint32_t count = ++s_ioStartupCount;
        if (count > 1)
            return Result::Success;

        auto countGuard = MakeScopeGuard([]() { --s_ioStartupCount; });

        // Create the completion port
        s_ioPort = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);

        if (s_ioPort == nullptr)
            return Result::FromLastError();

        auto portGuard = MakeScopeGuard([&]() { CloseHandle(s_ioPort); });

        // Create the thread for handling competion events
        s_ioThread = ::CreateThread(nullptr, 0, IOCompletionThread, s_ioPort, 0, nullptr);

        if (s_ioThread == nullptr)
            return Result::FromLastError();

        Result affinityResult = SetThreadAffinity(reinterpret_cast<uintptr_t>(s_ioThread), config.iocp.threadAffinity);
        if (!affinityResult)
            return affinityResult;

        ::SetThreadDescription(s_ioThread, L"Async File IOCP Thread");

        portGuard.Dismiss();
        countGuard.Dismiss();
        return Result::Success;
    }

    void ShutdownAsyncFileIO()
    {
        std::lock_guard<std::mutex> lock(s_ioStartupMutex);

        HE_ASSERT(s_ioStartupCount > 0);
        const uint32_t count = --s_ioStartupCount;
        if (count > 0)
            return;

        if (s_ioThread)
        {
            HE_ASSERT(s_ioPort);

            ::PostQueuedCompletionStatus(s_ioPort, 0, 0, nullptr);
            ::WaitForSingleObject(s_ioThread, INFINITE);
            ::CloseHandle(s_ioThread);
            s_ioThread = nullptr;
        }

        if (s_ioPort)
        {
            ::CloseHandle(s_ioPort);
            s_ioPort = nullptr;
        }
    }

    AsyncFile::AsyncFile()
        : m_fd(Win32InvalidFd)
    {}

    AsyncFile::AsyncFile(AsyncFile&& x)
        : m_fd(Exchange(x.m_fd, Win32InvalidFd))
    {}

    AsyncFile::~AsyncFile()
    {
        Close();
    }

    AsyncFile& AsyncFile::operator=(AsyncFile&& x)
    {
        Close();
        m_fd = Exchange(x.m_fd, Win32InvalidFd);
        return *this;
    }

    Result AsyncFile::Open(const char* path, FileOpenMode mode, FileOpenFlag flags)
    {
        HE_ASSERT(m_fd == Win32InvalidFd);
        HANDLE handle = Win32FileOpen(path, mode, flags, FILE_FLAG_OVERLAPPED);

        if (handle == INVALID_HANDLE_VALUE)
            return Result::FromLastError();

        ::CreateIoCompletionPort(handle, s_ioPort, 0, 0);

        m_fd = reinterpret_cast<intptr_t>(handle);

        return Result::Success;
    }

    void AsyncFile::Close()
    {
        if (m_fd != Win32InvalidFd)
        {
            const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);
            ::CloseHandle(handle);
            m_fd = Win32InvalidFd;
        }
    }

    bool AsyncFile::IsOpen() const
    {
        return m_fd != Win32InvalidFd;
    }

    uint64_t AsyncFile::GetSize() const
    {
        const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);

        LARGE_INTEGER fileSize;
        if (!::GetFileSizeEx(handle, &fileSize))
            return 0;
        return fileSize.QuadPart;
    }

    std::future<AsyncFileResult> AsyncFile::ReadAsync(void* dst, uint64_t offset, uint32_t size)
    {
        const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);

        AsyncOp* op = Allocator::GetTemp().New<AsyncOp>();
        ::DuplicateHandle(::GetCurrentProcess(), handle, ::GetCurrentProcess(), &op->file, 0, FALSE, DUPLICATE_SAME_ACCESS);

        op->overlap.Offset = (DWORD)offset;
        op->overlap.OffsetHigh = (DWORD)(offset >> 32);
        BOOL result = ::ReadFile(op->file, dst, size, nullptr, &op->overlap);

        auto f = op->promise.get_future();

        if (result)
            HandleCompletedOverlap(op);
        else if (::GetLastError() != ERROR_IO_PENDING)
            HandleCompletedOp(op, Result::FromLastError(), 0);

        return f;
    }

    std::future<AsyncFileResult> AsyncFile::WriteAsync(const void* src, uint64_t offset, uint32_t size)
    {
        const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);

        AsyncOp* op = Allocator::GetTemp().New<AsyncOp>();
        ::DuplicateHandle(::GetCurrentProcess(), handle, ::GetCurrentProcess(), &op->file, 0, FALSE, DUPLICATE_SAME_ACCESS);

        op->overlap.Offset = (DWORD)offset;
        op->overlap.OffsetHigh = (DWORD)(offset >> 32);
        BOOL result = ::WriteFile(op->file, src, size, nullptr, &op->overlap);

        auto f = op->promise.get_future();

        if (result)
            HandleCompletedOverlap(op);
        else if (::GetLastError() != ERROR_IO_PENDING)
            HandleCompletedOp(op, Result::FromLastError(), 0);

        return f;
    }

    static void HandleCompletedOp(AsyncOp* op, Result result, uint32_t bytesTransferred)
    {
        AsyncFileResult r;
        r.result = result;
        r.bytesTransferred = bytesTransferred;

        ::CloseHandle(op->file);
        op->promise.set_value(r);
        Allocator::GetTemp().Delete(op);
    }

    static void HandleCompletedOverlap(AsyncOp* op)
    {
        DWORD bytesTransferred;
        BOOL result = ::GetOverlappedResult(op->file, &op->overlap, &bytesTransferred, true);

        if (result)
        {
            HandleCompletedOp(op, Result::Success, bytesTransferred);
        }
        else
        {
            Result r = Result::FromLastError();
            HE_LOG_ERROR(async_file, HE_MSG("Failed to get result of overlapped file IO"), HE_KV(error, r));
            HandleCompletedOp(op, r, 0);
        }
    }

    static DWORD IOCompletionThread(LPVOID)
    {
        HE_ASSERT(s_ioPort);

        OVERLAPPED_ENTRY entries[32];

        while (s_ioStartupCount > 0)
        {
            // Wait for a completion notification
            ULONG count = 0;
            BOOL status = ::GetQueuedCompletionStatusEx(s_ioPort, entries, HE_LENGTH_OF(entries), &count, INFINITE, false);

            if (!status)
            {
                Result r = Result::FromLastError();
                if (r.GetCode() != WAIT_TIMEOUT)
                {
                    HE_LOG_ERROR(async_file, HE_MSG("Failed to get completion status of async file io."), HE_KV(error, r));
                }
                continue;
            }

            for (ULONG i = 0; i < count; ++i)
            {
                OVERLAPPED_ENTRY& entry = entries[i];

                if (entry.lpOverlapped == nullptr)
                    continue;

                AsyncOp* op = CONTAINING_RECORD(entry.lpOverlapped, AsyncOp, overlap);
                HandleCompletedOverlap(op);
            }
        }

        return 0;
    }
}

#endif
