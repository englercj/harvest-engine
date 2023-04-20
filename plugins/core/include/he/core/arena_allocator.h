// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/types.h"
#include "he/core/virtual_memory.h"

namespace he
{
    /// The arena allocator reserves a virtual address space at construction time and then commits
    /// pages of that virtual memory linearly as allocations are issued. Free does not release any
    /// memory, only Clear does. However, calling Free can still be useful for tracking memory
    /// regions that should no longer be accessed.
    ///
    /// The arena allocator is particularly useful when there is a high watermark of allocation that
    /// can happen, but those allocations are most easily disposed of as a single unit. For example,
    /// a buffer that is used for per-frame allocations that are disposed of at the end of a frame.
    class ArenaAllocator : public Allocator
    {
    public:
        static constexpr size_t DefaultSize = 32 * 1024 * 1024; // 32 MB

    public:
        /// Constructs a memory arena in which to make allocations by reserving at least `size`
        /// bytes of virtual address space.
        ///
        /// \param[in] size The number of bytes of address space to allocate.
        explicit ArenaAllocator(size_t size = DefaultSize) noexcept;

        /// Constructs an arena allocator by moving the data from `x`.
        ArenaAllocator(ArenaAllocator&& x) noexcept;

        /// Invalidates all issued pointers and releases all memory pages that have been
        /// committed by this allocator.
        ~ArenaAllocator() noexcept;

        /// \copydoc Allocator::Malloc(size_t, size_t)
        [[nodiscard]] void* Malloc(size_t size, size_t alignment = DefaultAlignment) noexcept override;

        /// \copydoc Allocator::Realloc(void*, size_t, size_t)
        void* Realloc(void* ptr, size_t newSize, size_t alignment = DefaultAlignment) noexcept override;

        /// \copydoc Allocator::Free(void*)
        ///
        /// \note This method does not actually release any memory pages and the free'd allocation
        /// does not get reused until \ref Clear is called. Instead, this function can be useful to
        /// detect unexpected use-after-free errors from ASAN.
        void Free(void* ptr) noexcept override;

        /// Invalidates all issued pointers and resets the state for reuse. Previously committed
        /// pages are kept for reuse.
        void Clear() noexcept;

    private:
        ArenaAllocator(const ArenaAllocator&) = delete;
        ArenaAllocator& operator=(const ArenaAllocator&) = delete;
        ArenaAllocator& operator=(ArenaAllocator&& x) = delete;

    private:
        // Needed to allow us to perform Realloc operations where we need to copy the old bytes
        // into a new allocation. Also useful for poisoning free'd memory so that ASAN will track
        // user-after-free errors.
        struct AllocHeader
        {
            size_t size;
        };

        static AllocHeader* GetHeader(void* userPtr) noexcept;

    private:
        friend class ArenaAllocatorTestAttorney;

        VirtualMemory m_arena{};
        size_t m_allocatedBytes{ 0 };
        uint32_t m_committedPages{ 0 };
    };
}
