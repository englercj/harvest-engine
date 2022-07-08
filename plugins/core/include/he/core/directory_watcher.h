// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/file.h"
#include "he/core/result.h"
#include "he/core/string.h"
#include "he/core/types.h"
#include "he/core/utils.h"

namespace he
{
    enum class FileChangeReason : uint8_t
    {
        Added,
        Removed,
        Modified,
        Renamed_OldName,
        Renamed_NewName,
    };

    enum class FileWatchResult : uint8_t
    {
        Success,
        Failure,

        Timeout,
    };

    FileWatchResult GetFileWatchResult(Result result);

    class DirectoryWatcher
    {
    public:
        /// Structure representing an entry in a recursive directory scan.
        struct Entry
        {
            Entry(Allocator& allocator = Allocator::GetDefault()) noexcept
                : path(allocator)
                , reason(FileChangeReason::Added)
            {}

            String path;
            FileChangeReason reason;
        };

    public:
        DirectoryWatcher(Allocator& allocator = Allocator::GetDefault()) noexcept
            : m_allocator(allocator)
            , m_impl(nullptr)
        {}

        DirectoryWatcher(DirectoryWatcher&& x) noexcept
            : m_allocator(x.m_allocator)
            , m_impl(Exchange(x.m_impl, nullptr))
        {}

        ~DirectoryWatcher() noexcept { Close(); }

        DirectoryWatcher(const DirectoryWatcher&) = delete;
        DirectoryWatcher& operator=(const DirectoryWatcher&) = delete;
        DirectoryWatcher& operator=(DirectoryWatcher&&) = delete;

        Result Open(const char* path);

        void Close();

        Result WaitForEntry(Entry& outEntry, Duration timeout = Duration_Max);

    private:
        Allocator& m_allocator;
        void* m_impl;
    };
}
