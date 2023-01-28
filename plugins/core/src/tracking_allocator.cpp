// Copyright Chad Engler

#include "he/core/tracking_allocator.h"

#include "he/core/asan.h"
#include "he/core/assert.h"
#include "he/core/stack_trace.h"

namespace he
{
    void* TrackingAllocator::Malloc(size_t size, size_t alignment) noexcept
    {
        alignment = AlignUp(alignment, sizeof(void*));

        // Allocate our worst-case size so we can properly fit and align all our pointers
        const size_t allocSize = sizeof(AllocHeader) + size + alignment;
        void* mem = m_allocator.Malloc(allocSize, alignof(AllocHeader));

        // Calculate aligned pointers to our header and the user's pointer
        uint8_t* headPtr = static_cast<uint8_t*>(mem);
        uint8_t* userPtr = AlignUp(headPtr + sizeof(AllocHeader), alignment);
        HE_ASSERT(userPtr + size <= headPtr + allocSize);

        // Now shift the header pointer as close to the user pointer as possible.
        // This makes it easy for us to find the header pointer from the user pointer.
        headPtr = AlignDown(userPtr - sizeof(AllocHeader), alignof(AllocHeader));
        HE_ASSERT(GetHeader(userPtr) == reinterpret_cast<AllocHeader*>(headPtr));

        // Setup the header
        AllocHeader* header = ::new(headPtr) AllocHeader();
        header->size = size;
        header->mem = mem;

        uint32_t count = HE_LENGTH_OF(header->frames);
        CaptureStackTrace(header->frames, count, 1);

        Append(header);
        return userPtr;
    }

    void* TrackingAllocator::Realloc(void* ptr, size_t newSize, size_t alignment) noexcept
    {
        if (ptr == nullptr)
            return Malloc(newSize, alignment);

        if (newSize == 0)
        {
            Free(ptr);
            return nullptr;
        }

        void* p = Malloc(newSize, alignment);
        const AllocHeader* oldHeader = GetHeader(ptr);
        const size_t oldSize = oldHeader->size;
        MemCopy(p, ptr, Min(oldSize, newSize));
        Free(ptr);
        return p;
    }

    void TrackingAllocator::Free(void* ptr) noexcept
    {
        AllocHeader* header = GetHeader(ptr);
        Remove(header);
        m_allocator.Free(header->mem);
    }

    TrackingAllocator::AllocHeader* TrackingAllocator::GetHeader(void* userPtr) noexcept
    {
        uint8_t* p = static_cast<uint8_t*>(userPtr);
        uint8_t* h = AlignDown(p - sizeof(AllocHeader), alignof(AllocHeader));
        return reinterpret_cast<AllocHeader*>(h);
    }

    void TrackingAllocator::Append(AllocHeader* header)
    {
        LockGuard lock(m_lock);
        if (m_count == 0)
        {
            m_head = header;
            m_tail = header;
        }
        else
        {
            header->prev = m_tail;
            m_tail->next = header;
            m_tail = header;
        }
        ++m_count;
    }

    void TrackingAllocator::Remove(AllocHeader* header)
    {
        LockGuard lock(m_lock);

        AllocHeader* prev = header->prev;
        AllocHeader* next = header->next;

        if (prev)
            prev->next = next;
        else
            m_head = next;

        if (next)
            next->prev = prev;
        else
            m_tail = prev;

        --m_count;
    }
}
