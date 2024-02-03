// Copyright Chad Engler

#include "he/core/async_file.h"

#include "he/core/assert.h"
#include "he/core/log.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/sync.h"
#include "he/core/thread.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "file_helpers.win32.h"

#include "he/core/win32_min.h"

#include <fileapi.h>

namespace he
{
    // --------------------------------------------------------------------------------------------
    static Mutex s_ioStartupMutex{};
    static uint32_t s_ioStartupCount{ 0 };
    static HANDLE s_ioPort{ nullptr };
    static Thread s_ioThread{};

    constexpr ULONG_PTR CK_Shutdown = 1;
    constexpr ULONG_PTR CK_FileComplete = 2;
    constexpr ULONG_PTR CK_FileError = 3;

    // --------------------------------------------------------------------------------------------
    class AsyncOp : public OVERLAPPED
    {
    public:
        ~AsyncOp()
        {
            Close();
        }

        HANDLE File() const { return m_file; }

        void Close()
        {
            if (m_file != INVALID_HANDLE_VALUE)
            {
                ::CloseHandle(m_file);
                m_file = INVALID_HANDLE_VALUE;
            }
        }

        void Reset(HANDLE fd, uint64_t offset, AsyncFile::CompleteDelegate callback)
        {
            Close();
            ::DuplicateHandle(::GetCurrentProcess(), fd, ::GetCurrentProcess(), &m_file, 0, FALSE, DUPLICATE_SAME_ACCESS);

            m_event.Reset();

            Offset = (DWORD)offset;
            OffsetHigh = (DWORD)(offset >> 32);

            m_result = Win32Result(ERROR_IO_PENDING);
            m_bytesTransferred = 0;
            m_callback = callback;
        }

        bool IsComplete()
        {
            return m_event.Wait(Duration_Zero);
        }

        void Wait()
        {
            m_event.Wait();
        }

        void SetResult(DWORD err, DWORD bytesTransferred)
        {
            m_result = Win32Result(err);
            m_bytesTransferred = static_cast<uint32_t>(bytesTransferred);
            m_event.Signal();

            if (m_callback)
                m_callback(*this);
        }

        Result GetResult(uint32_t* bytesTransferred)
        {
            HE_ASSERT(IsComplete());

            if (bytesTransferred)
                *bytesTransferred = m_bytesTransferred;

            return m_result;
        }

        operator AsyncFileOp() const { return AsyncFileOp{ reinterpret_cast<uintptr_t>(this) }; }

    private:
        HANDLE m_file{ INVALID_HANDLE_VALUE };
        SyncEvent m_event{};

        Result m_result{};
        uint32_t m_bytesTransferred{ 0 };
        AsyncFile::CompleteDelegate m_callback{};
    };

    // --------------------------------------------------------------------------------------------
    static void IOCompletionThread(void*);
    static void UnlockedShutdownAsyncFileIO();

    // --------------------------------------------------------------------------------------------
    Result StartupAsyncFileIO(const AsyncFileIOConfig& config)
    {
        LockGuard lock(s_ioStartupMutex);

        const uint32_t count = ++s_ioStartupCount;
        if (count > 1)
            return Result::Success;

        auto failGuard = MakeScopeGuard([]() { UnlockedShutdownAsyncFileIO(); });

        // Create the completion port
        s_ioPort = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
        if (s_ioPort == nullptr)
            return Result::FromLastError();

        // Create the thread for handling competion events
        ThreadDesc desc;
        desc.proc = &IOCompletionThread;
        desc.data = s_ioPort;
        desc.affinity = config.iocp.threadAffinity;
        const Result rc = s_ioThread.Start(desc);
        if (!rc)
            return rc;

        failGuard.Dismiss();
        return Result::Success;
    }

    static void UnlockedShutdownAsyncFileIO()
    {
        HE_ASSERT(s_ioStartupCount > 0);
        const uint32_t count = --s_ioStartupCount;
        if (count > 0)
            return;

        if (s_ioThread.IsJoinable())
        {
            HE_ASSERT(s_ioPort);

            ::PostQueuedCompletionStatus(s_ioPort, 0, CK_Shutdown, nullptr);
            s_ioThread.Join();
        }

        if (s_ioPort)
        {
            ::CloseHandle(s_ioPort);
            s_ioPort = nullptr;
        }
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
        : m_fd(Win32InvalidFd)
    {}

    AsyncFile::AsyncFile(AsyncFile&& x) noexcept
        : m_fd(Exchange(x.m_fd, Win32InvalidFd))
    {}

    AsyncFile::~AsyncFile() noexcept
    {
        Close();
    }

    AsyncFile& AsyncFile::operator=(AsyncFile&& x) noexcept
    {
        Close();
        m_fd = Exchange(x.m_fd, Win32InvalidFd);
        return *this;
    }

    Result AsyncFile::Open(const char* path, FileOpenMode mode, FileOpenFlag flags)
    {
        if (!HE_VERIFY(m_fd == Win32InvalidFd))
            return Result::InvalidParameter;

        HANDLE handle = Win32FileOpen(path, mode, flags, FILE_FLAG_OVERLAPPED);

        if (handle == INVALID_HANDLE_VALUE)
            return Result::FromLastError();

        ::CreateIoCompletionPort(handle, s_ioPort, CK_FileComplete, 0);

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

    AsyncFileOp AsyncFile::ReadAsync(void* dst, uint64_t offset, uint32_t size, CompleteDelegate callback)
    {
        const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);

        // TODO: Pool these allocations in an object pool.
        AsyncOp* op = Allocator::GetDefault().New<AsyncOp>();
        op->Reset(handle, offset, callback);

        BOOL result = ::ReadFile(op->File(), dst, size, nullptr, op);

        if (!result && ::GetLastError() != ERROR_IO_PENDING)
            ::PostQueuedCompletionStatus(s_ioPort, ::GetLastError(), CK_FileError, op);

        return *op;
    }

    AsyncFileOp AsyncFile::WriteAsync(const void* src, uint64_t offset, uint32_t size, CompleteDelegate callback)
    {
        const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);

        // TODO: Pool these allocations in an object pool.
        AsyncOp* op = Allocator::GetDefault().New<AsyncOp>();
        op->Reset(handle, offset, callback);

        BOOL result = ::WriteFile(op->File(), src, size, nullptr, op);

        if (!result && ::GetLastError() != ERROR_IO_PENDING)
            ::PostQueuedCompletionStatus(s_ioPort, ::GetLastError(), CK_FileError, op);

        return *op;
    }

    Result AsyncFile::GetAttributes(FileAttributes& outAttributes) const
    {
        const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);
        return Win32FileGetAttributes(handle, outAttributes);
    }

    Result AsyncFile::GetPath(String& outPath) const
    {
        const HANDLE handle = reinterpret_cast<HANDLE>(m_fd);
        return Win32FileGetPath(handle, outPath);
    }

    // --------------------------------------------------------------------------------------------
    static void IOCompletionThread(void*)
    {
        HE_ASSERT(s_ioPort);

        Thread::SetName("[HE] Async File IOCP Thread");

        OVERLAPPED_ENTRY entries[16];

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

                switch (entry.lpCompletionKey)
                {
                    case CK_Shutdown:
                    {
                        // Wake event to tell us to shutdown. Return out of the thread function.
                        return;
                    }
                    case CK_FileComplete:
                    {
                        // Event sent by windows when a file operation completes normally.
                        HE_ASSERT(entry.lpOverlapped);
                        AsyncOp* op = static_cast<AsyncOp*>(entry.lpOverlapped);

                        DWORD bytesTransferred = 0;
                        const BOOL result = ::GetOverlappedResult(op->File(), op, &bytesTransferred, false);
                        HE_ASSERT(result || ::GetLastError() != ERROR_IO_PENDING, HE_KV(result, Result::FromLastError()));

                        const DWORD err = result ? ERROR_SUCCESS : ::GetLastError();
                        op->SetResult(err, bytesTransferred);
                        break;
                    }
                    case CK_FileError:
                    {
                        // When ReadFile/WriteFile return an error we send an event here so we can
                        // call the callback from this thread. We pass the error through the bytes
                        // transferred parameter, which is why the SetResult params seem reversed.
                        HE_ASSERT(entry.lpOverlapped);
                        AsyncOp* op = static_cast<AsyncOp*>(entry.lpOverlapped);
                        op->SetResult(entry.dwNumberOfBytesTransferred, 0);
                        break;
                    }
                }
            }
        }
    }
}

#endif
