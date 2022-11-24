// Copyright Chad Engler

#include "he/core/directory_watcher.h"

#include "he/core/clock.h"
#include "he/core/directory.h"
#include "he/core/enum_ops.h"
#include "he/core/file.h"
#include "he/core/path.h"
#include "he/core/scope_guard.h"

#include <algorithm>
#include <unordered_map>

#if defined(HE_PLATFORM_LINUX)

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <sys/stat.h>

namespace he
{
    // Only reading one event at a time.
    constexpr uint32_t EventBufferSize = sizeof(inotify_event) + NAME_MAX + 1;

    constexpr uint32_t StopPipeReadIndex = 0;
    constexpr uint32_t StopPipeWriteIndex = 1;

    constexpr uint32_t WatchFlags = IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO;

    struct DirectoryWatcherImpl
    {
        std::unordered_map<int, String> wdToPathMap{};

        int stopPipeFds[2]{ -1, -1 };
        int inotifyFd{ -1 };
        int epollFd{ -1 };

        epoll_event inotifyEpollEvent{};
        epoll_event stopEpollEvent{};
        epoll_event epollEvent{};

        uint8_t* buf{ nullptr };
        uint32_t offset{ 0 };
        uint32_t validBytes{ 0 };
    };

    static Result WatchDir(DirectoryWatcherImpl* impl, const char* path)
    {
        DirectoryScanner scanner;

        if (!scanner.Open(path))
            return false;

        const uint32_t pathLen = String::Length(path);
        String fullPath;

        DirectoryScanner::Entry entry;
        while (scanner.NextEntry(entry))
        {
            fullPath.Assign(path, pathLen);
            ConcatPath(fullPath, entry.name);

            if (!entry.isDirectory)
                continue;

            int wd = inotify_add_watch(impl->inotifyFd, fullPath.Data(), WatchFlags);

            if (wd == -1)
                return Result::FromLastError();

            impl->wdToPathMap.insert({ wd, fullPath });

            Result r = WatchDir(impl, fullPath.Data());
            if (!r)
                return r;
        }

        return Result::Success;
    }

    static Result ReadEntry(DirectoryWatcherImpl* impl, DirectoryWatcher::Entry& outEntry)
    {
        const uint32_t requiredBytes = impl->offset + sizeof(inotify_event);

        if (!HE_VERIFY(impl->validBytes >= requiredBytes,
            HE_KV(required_bytes, requiredBytes),
            HE_KV(struct_size, sizeof(inotify_event)),
            HE_KV(offset, impl->offset),
            HE_KV(valid_bytes, impl->validBytes)))
        {
            return PosixResult(EINVAL);
        }

        const auto* info = reinterpret_cast<const inotify_event*>(impl->buf);

        if (!HE_VERIFY(impl->validBytes >= (requiredBytes + info->len),
            HE_KV(required_bytes, requiredBytes),
            HE_KV(struct_size, sizeof(inotify_event)),
            HE_KV(file_name_length, info->len),
            HE_KV(offset, impl->offset),
            HE_KV(valid_bytes, impl->validBytes)))
        {
            return PosixResult(EINVAL);
        }

        // Advance to the next event for the next call into here
        impl->offset += sizeof(inotify_event) + info->len;

        // If we get the ignored flag then the kernel has stopped watching this inode for some
        // reason. Usually this is because it was deleted, so we need to remove our stored wd.
        // We do this at exit because we need to do it after we build the name for the entry,
        // but also need to do it before an earlier exit to building the name.
        HE_AT_SCOPE_EXIT([&]()
        {
            if (HasFlag(info->mask, IN_IGNORED))
            {
                impl->wdToPathMap.erase(info->wd);
            }
        });

        // Ignore directory events, this is a file watcher after all!
        if (HasFlag(info->mask, IN_ISDIR))
            return PosixResult(ETIMEDOUT);

        // Build the name from the watch descriptor's stored name, and the event's file name
        if (info->len > 0)
        {
            outEntry.path = impl->wdToPathMap[info->wd];
            outEntry.path.Append(info->name, info->len - 1);
        }

        // Discover the reason for the update. If we don't find a match for one of our reasons
        // then pretend we didn't get an event by returning ETIMEDOUT.
        if (HasFlag(info->mask, IN_CREATE))
            outEntry.reason = FileChangeReason::Added;
        else if (HasFlag(info->mask, IN_DELETE))
            outEntry.reason = FileChangeReason::Removed;
        else if (HasFlag(info->mask, IN_MODIFY))
            outEntry.reason = FileChangeReason::Modified;
        else if (HasFlag(info->mask, IN_MOVED_FROM))
            outEntry.reason = FileChangeReason::Renamed_OldName;
        else if (HasFlag(info->mask, IN_MOVED_TO))
            outEntry.reason = FileChangeReason::Renamed_NewName;
        else
            return PosixResult(ETIMEDOUT);

        return Result::Success;
    }

    Result DirectoryWatcher::Open(const char* path)
    {
        if (!HE_VERIFY(m_impl == nullptr))
            return Result::InvalidParameter;

        auto failGuard = MakeScopeGuard([&]() { Close(); });

        DirectoryWatcherImpl* impl = m_allocator.New<DirectoryWatcherImpl>();
        m_impl = impl;

        int rc = pipe2(impl->stopPipeFds, O_NONBLOCK);
        if (rc == -1)
            return Result::FromLastError();

        impl->inotifyFd = inotify_init1(IN_NONBLOCK);
        if (impl->inotifyFd == -1)
            return Result::FromLastError();

        impl->epollFd = epoll_create1(0);
        if (impl->epollFd  == -1)
            return Result::FromLastError();

        impl->inotifyEpollEvent.events = EPOLLIN | EPOLLET;
        impl->inotifyEpollEvent.data.fd = impl->inotifyFd;
        rc = epoll_ctl(impl->epollFd, EPOLL_CTL_ADD, impl->inotifyFd, &impl->inotifyEpollEvent);
        if (rc == -1)
            return Result::FromLastError();

        impl->stopEpollEvent.events = EPOLLIN | EPOLLET;
        impl->stopEpollEvent.data.fd = impl->stopPipeFds[StopPipeReadIndex];
        rc = epoll_ctl(impl->epollFd, EPOLL_CTL_ADD, impl->stopPipeFds[StopPipeReadIndex], &impl->stopEpollEvent);
        if (rc == -1)
            return Result::FromLastError();

        impl->buf = static_cast<uint8_t*>(m_allocator.Malloc(EventBufferSize, alignof(inotify_event)));

        struct stat sb;
        if (stat(path, &sb))
            return Result::FromLastError();

        if (!S_ISDIR(sb.st_mode))
            return Result::InvalidParameter;

        Result r = WatchDir(impl, path);
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

        if (impl->epollFd != -1)
        {
            write(impl->stopPipeFds[StopPipeWriteIndex], "s", 1);
            epoll_ctl(impl->epollFd, EPOLL_CTL_DEL, impl->inotifyFd, 0);
            epoll_ctl(impl->epollFd, EPOLL_CTL_DEL, impl->stopPipeFds[StopPipeReadIndex], 0);
            close(impl->epollFd);
        }

        if (impl->inotifyFd != -1)
            close(impl->inotifyFd);

        if (impl->stopPipeFds[StopPipeReadIndex] != -1)
            close(impl->stopPipeFds[StopPipeReadIndex]);

        if (impl->stopPipeFds[StopPipeWriteIndex] != -1)
            close(impl->stopPipeFds[StopPipeWriteIndex]);

        m_allocator.Free(impl->buf);
        m_allocator.Delete(impl);
        m_impl = nullptr;
    }

    Result DirectoryWatcher::WaitForEntry(Entry& outEntry, Duration timeout)
    {
        if (!HE_VERIFY(m_impl))
            return false;

        DirectoryWatcherImpl* impl = static_cast<DirectoryWatcherImpl*>(m_impl);

        if (impl->offset < impl->validBytes)
            return ReadEntry(impl, outEntry);

        const uint32_t timeoutMs = ToPeriod<Milliseconds, int32_t>(timeout);
        const int rc = epoll_wait(impl->epollFd, &impl->epollEvent, 1, timeoutMs);

        if (rc == -1)
            return Result::FromLastError();

        if (rc == 0)
            return PosixResult(ETIMEDOUT);

        if (!HE_VERIFY(rc == 1, HE_KV(events, rc)))
            return PosixResult(EINVAL);

        if (impl->epollEvent.data.fd == impl->stopPipeFds[StopPipeReadIndex])
            return PosixResult(ECANCELED);

        impl->offset = 0;
        impl->validBytes = 0;
        const ssize_t len = read(impl->epollEvent.data.fd, impl->buf, EventBufferSize);
        if (len == -1)
            return Result::FromLastError();

        impl->validBytes = static_cast<uint32_t>(len);

        return ReadEntry(impl, outEntry);
    }
}

#endif
