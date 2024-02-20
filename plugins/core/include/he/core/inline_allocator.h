// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/assert.h"
#include "he/core/types.h"
#include "he/core/virtual_memory.h"

namespace he
{
    /// Allocator that uses an inline buffer for allocations, and falls back to a secondary
    /// allocator when the inline buffer is exhausted.
    template <size_t InlineSize, size_t Alignment = Allocator::DefaultAlignment>
    class InlineAllocator final : public Allocator
    {
    public:
        static_assert(InlineSize > 0, "InlineSize must be greater than zero");

        static constexpr size_t Size = InlineSize;

    public:
        /// Creates a new InlineAllocator with the given inline buffer size and fallback allocator.
        ///
        /// \param[in] fallback The fallback allocator to use when the inline buffer is exhausted.
        explicit InlineAllocator(Allocator& fallback) noexcept
            : m_fallback(fallback)
            , m_cursor(m_buffer)
        {}

        /// \copydoc Allocator::Malloc(size_t, size_t)
        [[nodiscard]] void* Malloc(size_t size, size_t alignment = DefaultAlignment) noexcept override
        {
            uint8_t* result = AlignUp(m_cursor, alignment);

            const uint8_t* end = m_buffer + Size;
            if (result < end && size <= static_cast<size_t>(end - result))
            {
                m_cursor = result + size;
                return result;
            }

            return m_fallback.Malloc(size, alignment);
        }

        /// \copydoc Allocator::Realloc(void*, size_t, size_t)
        void* Realloc(void* ptr, size_t newSize, size_t alignment = DefaultAlignment) noexcept override
        {
            if (ptr == nullptr)
                return Malloc(newSize, alignment);

            if (newSize == 0)
            {
                Free(ptr);
                return nullptr;
            }

            // TODO: Need to store a header with size if I want to support realloc.
            HE_ASSERT(false, HE_MSG("InlineAllocator does not support realloc. Use Malloc, MemCopy, and Free instead."));
            return nullptr;
        }

        /// \copydoc Allocator::Free(void*)
        void Free(void* ptr) noexcept override
        {
            if (!ptr)
                return;

            if (ptr >= m_buffer && ptr < (m_buffer + Size))
            {
                // Do nothing
            }
            else
            {
                m_fallback.Free(ptr);
            }
        }

    private:
        InlineAllocator(const InlineAllocator&) = delete;
        InlineAllocator& operator=(const InlineAllocator&) = delete;
        InlineAllocator& operator=(InlineAllocator&& x) = delete;

    private:
        Allocator& m_fallback;
        uint8_t* m_cursor;
        alignas(Alignment) uint8_t m_buffer[Size];
    };
}
