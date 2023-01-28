// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/delegate.h"
#include "he/core/span.h"
#include "he/core/sync.h"
#include "he/core/types.h"

/// \def HE_ALLOC_TRACKING_FRAME_COUNT
/// This define controls the number of stack frames that are captured to track each allocation
/// made by the \ref TrackingAllocator. Each frame tracked adds a cost of sizeof(uintptr_t)
/// to each allocation made. The default value is 8.
/// 
#if !defined(HE_ALLOC_TRACKING_FRAME_COUNT)
    #define HE_ALLOC_TRACKING_FRAME_COUNT 8
#endif

namespace he
{
    // The tracking allocator is a composable allocator that tracks all the allocations it issues
    // and can report live allocations and where they come from.
    class TrackingAllocator : public Allocator
    {
    public:
        /// Constructs a tracking allocator that will proxy allocation operations to the given
        /// allocator for tracking.
        explicit TrackingAllocator(Allocator& allocator) noexcept : m_allocator(allocator) {}

        /// \copydoc Allocator::Malloc(size_t, size_t)
        [[nodiscard]] void* Malloc(size_t size, size_t alignment = DefaultAlignment) noexcept override;

        /// \copydoc Allocator::Realloc(void*, size_t, size_t)
        void* Realloc(void* ptr, size_t newSize, size_t alignment = DefaultAlignment) noexcept override;

        /// \copydoc Allocator::Free(void*)
        void Free(void* ptr) noexcept override;

        /// Gets the number of active allocations that have not yet been freed.
        uint32_t ActiveAllocationCount() const { return m_count; }

        /// Calls `iterator` for each active allocation that this allocator is tracking.
        ///
        /// The iterator function is invoked with three parameters: the size, alignment, and
        /// a span of stack frames.
        ///
        /// \param[in] iterator The function to call for each active alocation
        template <typename F>
        void ForEachActiveAllocation(F&& iterator);

    private:
        struct AllocHeader
        {
            size_t size{ 0 };
            void* mem{ nullptr };
            uintptr_t frames[HE_ALLOC_TRACKING_FRAME_COUNT]{};
            AllocHeader* next{ nullptr };
            AllocHeader* prev{ nullptr };
        };

        static AllocHeader* GetHeader(void* userPtr) noexcept;

        void Append(AllocHeader* header);
        void Remove(AllocHeader* header);

    private:
        Allocator& m_allocator;
        RWLock m_lock{};

        uint32_t m_count{ 0 };
        AllocHeader* m_head{ nullptr };
        AllocHeader* m_tail{ nullptr };
    };

    template <typename F>
    inline void TrackingAllocator::ForEachActiveAllocation(F&& iterator)
    {
        ReadLockGuard lock(m_lock);
        const AllocHeader* header = m_head;
        while (header)
        {
            // count valid frames in the array so we can make a span
            uint32_t count = 0;
            while (count < HE_LENGTH_OF(header->frames) && header->frames[count])
                ++count;

            const Span<const uintptr_t> frames(header->frames, count);
            iterator(header->size, frames);
            header = header->next;
        }
    }
}
