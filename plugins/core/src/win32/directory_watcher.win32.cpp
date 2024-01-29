// Copyright Chad Engler

#include "he/core/directory_watcher.h"

#include "he/core/assert.h"
#include "he/core/clock.h"
#include "he/core/log.h"
#include "he/core/scope_guard.h"
#include "he/core/wstr.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "file_helpers.win32.h"

#include "he/core/win32_min.h"

namespace he
{
    constexpr uint32_t EventBufferSize = 16 * 1024;

    struct DirectoryWatcherImpl
    {
        HANDLE handle{ INVALID_HANDLE_VALUE };
        OVERLAPPED overlap{};

        uint8_t* buf{ nullptr };
        DWORD offset{ 0 };
    };

    static Result RequestNextChanges(DirectoryWatcherImpl* impl)
    {
        // FILE_NOTIFY_CHANGE_FILE_NAME = renaming, creating, or deleting a file
        // FILE_NOTIFY_CHANGE_LAST_WRITE = change to the last write-time of a file
        constexpr DWORD WatchFlags = FILE_NOTIFY_CHANGE_FILE_NAME
            // | FILE_NOTIFY_CHANGE_DIR_NAME
            // | FILE_NOTIFY_CHANGE_ATTRIBUTES
            // | FILE_NOTIFY_CHANGE_SIZE
            | FILE_NOTIFY_CHANGE_LAST_WRITE
            // | FILE_NOTIFY_CHANGE_LAST_ACCESS
            // | FILE_NOTIFY_CHANGE_CREATION
            // | FILE_NOTIFY_CHANGE_SECURITY
            ;

        impl->offset = 0;
        const BOOL result = ::ReadDirectoryChangesExW(
            impl->handle,
            impl->buf,
            EventBufferSize,
            true,
            WatchFlags,
            nullptr,
            &impl->overlap,
            nullptr,
            ReadDirectoryNotifyInformation);

        return result ? Result::Success : Result::FromLastError();
    }

    static Result ReadEntry(DirectoryWatcherImpl* impl, DirectoryWatcher::Event& outEvent)
    {
        DWORD validBytes = 0;
        const BOOL result = ::GetOverlappedResult(impl->handle, &impl->overlap, &validBytes, false);

        if (!result)
        {
            // If there was any error (other than pending), then request the next
            if (::GetLastError() != ERROR_IO_PENDING)
            {
                RequestNextChanges(impl);
            }

            return Result::FromLastError();
        }

        if (validBytes == 0)
        {
            RequestNextChanges(impl);
            return Win32Result(ERROR_INSUFFICIENT_BUFFER);
        }

        // We can't use sizeof(FILE_NOTIFY_INFORMATION) here because we only actually require the
        // first 3 DWORD values in that structure to continue.
        constexpr DWORD FileNotifySize = sizeof(DWORD) * 3;
        const DWORD requiredBytes = impl->offset + FileNotifySize;

        if (!HE_VERIFY(validBytes >= requiredBytes,
            HE_KV(required_bytes, requiredBytes),
            HE_KV(struct_size, sizeof(FILE_NOTIFY_INFORMATION)),
            HE_KV(offset, impl->offset),
            HE_KV(valid_bytes, validBytes)))
        {
            RequestNextChanges(impl);
            return Win32Result(ERROR_INSUFFICIENT_BUFFER);
        }

        const auto* info = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(impl->buf);

        if (!HE_VERIFY(validBytes >= (requiredBytes + info->FileNameLength),
            HE_KV(required_bytes, requiredBytes),
            HE_KV(struct_size, sizeof(FILE_NOTIFY_INFORMATION)),
            HE_KV(file_name_length, info->FileNameLength),
            HE_KV(offset, impl->offset),
            HE_KV(valid_bytes, validBytes)))
        {
            RequestNextChanges(impl);
            return Win32Result(ERROR_INSUFFICIENT_BUFFER);
        }

        // Read the path name from the end of the
        WCToMBStr(outEvent.path, info->FileName, info->FileNameLength);

        // Convert the action into a change reason
        switch (info->Action)
        {
            case FILE_ACTION_ADDED: outEvent.reason = FileChangeReason::Added; break;
            case FILE_ACTION_REMOVED: outEvent.reason = FileChangeReason::Removed; break;
            case FILE_ACTION_MODIFIED: outEvent.reason = FileChangeReason::Modified; break;
            case FILE_ACTION_RENAMED_OLD_NAME: outEvent.reason = FileChangeReason::Renamed_OldName; break;
            case FILE_ACTION_RENAMED_NEW_NAME: outEvent.reason = FileChangeReason::Renamed_NewName; break;
            default:
            {
                HE_LOG_WARN(he_core,
                    HE_MSG("Encountered unknown file action in DirectoryWatcher, assuming Modified reason."),
                    HE_KV(file_path, outEvent.path),
                    HE_KV(file_action, info->Action));
                outEvent.reason = FileChangeReason::Modified;
                break;
            }
        }

        // Update our offset into the buffer or reset if we reached the end
        if (info->NextEntryOffset)
        {
            impl->offset += info->NextEntryOffset;
        }
        else
        {
            RequestNextChanges(impl);
        }

        return Result::Success;
    }

    DirectoryWatchResult GetDirectoryWatchResult(Result result)
    {
        switch (result.GetCode())
        {
            case ERROR_SUCCESS: return DirectoryWatchResult::Success;
            case ERROR_TIMEOUT: return DirectoryWatchResult::Timeout;
            default: return DirectoryWatchResult::Failure;
        }
    }

    Result DirectoryWatcher::Open(const char* path)
    {
        if (!HE_VERIFY(m_impl == nullptr))
            return Result::InvalidParameter;

        auto failGuard = MakeScopeGuard([&]() { Close(); });

        DirectoryWatcherImpl* impl = m_allocator.New<DirectoryWatcherImpl>();
        m_impl = impl;

        impl->handle = ::CreateFileW(
            HE_TO_WCSTR(path),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
            nullptr);

        if (impl->handle == INVALID_HANDLE_VALUE)
            return Result::FromLastError();

        impl->overlap.hEvent = ::CreateEventW(nullptr, true, false, nullptr);
        if (impl->overlap.hEvent == nullptr)
            return Result::FromLastError();

        // MSDN says this buffer should be aligned to size of DWORD, which is why that is used here
        // instead of alignof(FILE_NOTIFY_INFORMATION) as you might expect.
        impl->buf = static_cast<uint8_t*>(m_allocator.Malloc(EventBufferSize, sizeof(DWORD)));

        const Result r = RequestNextChanges(impl);
        if (!r)
            return r;

        failGuard.Dismiss();
        return Result::Success;
    }

    void DirectoryWatcher::Close()
    {
        if (!m_impl)
            return;

        DirectoryWatcherImpl* impl = static_cast<DirectoryWatcherImpl*>(m_impl);

        if (impl->handle != INVALID_HANDLE_VALUE)
        {
            ::CancelIo(impl->handle);
            ::CloseHandle(impl->handle);
        }

        if (impl->overlap.hEvent != nullptr)
        {
            ::CloseHandle(impl->overlap.hEvent);
        }

        m_allocator.Free(impl->buf);
        m_allocator.Delete(impl);
        m_impl = nullptr;
    }

    Result DirectoryWatcher::WaitForEvent(Event& outEvent, Duration timeout)
    {
        if (!HE_VERIFY(m_impl))
            return Result::InvalidParameter;

        DirectoryWatcherImpl* impl = static_cast<DirectoryWatcherImpl*>(m_impl);

        const uint32_t timeoutMs = ToPeriod<Milliseconds, uint32_t>(timeout);
        const DWORD r = ::WaitForSingleObject(impl->overlap.hEvent, timeoutMs);

        if (r == WAIT_FAILED)
            return Result::FromLastError();

        if (r == WAIT_TIMEOUT)
            return Win32Result(ERROR_TIMEOUT);

        if (!HE_VERIFY(r == WAIT_OBJECT_0, HE_KV(result, r)))
            return Win32Result(ERROR_INVALID_DATA);

        return ReadEntry(impl, outEvent);
    }
}

#endif
