// Copyright Chad Engler

#include "he/core/arena_allocator.h"

#include "he/core/asan.h"
#include "he/core/assert.h"
#include "he/core/memory_ops.h"
#include "he/core/result_fmt.h"
#include "he/core/utils.h"

namespace he
{
    ArenaAllocator::ArenaAllocator(size_t size) noexcept
    {
        const Result r = m_arena.Reserve(VirtualMemory::BytesToPages(size));
        HE_VERIFY(r, HE_KV(result, r));
        HE_ASAN_POISON_MEMORY(m_arena.Block(), m_arena.ByteSize());
    }

    ArenaAllocator::ArenaAllocator(ArenaAllocator&& x) noexcept
        : m_arena(Move(x.m_arena))
        , m_allocatedBytes(Exchange(x.m_allocatedBytes, 0))
        , m_committedPages(Exchange(x.m_committedPages, 0))
    {}

    ArenaAllocator::~ArenaAllocator() noexcept
    {
        Clear();
    }

    void* ArenaAllocator::Malloc(size_t size, size_t alignment) noexcept
    {
        alignment = AlignUp(alignment, sizeof(void*));

        HE_ASSERT(m_arena.Block());
        uint8_t* blockStart = static_cast<uint8_t*>(m_arena.Block());
        uint8_t* freeStart = blockStart + m_allocatedBytes;

        // Calculate aligned pointers to our header and the user's pointer
        uint8_t* headPtr = AlignUp(freeStart, alignof(AllocHeader));
        uint8_t* userPtr = AlignUp(headPtr + sizeof(AllocHeader), alignment);

        // Now shift the header pointer as close to the user pointer as possible
        headPtr = AlignDown(userPtr - sizeof(AllocHeader), alignof(AllocHeader));
        HE_ASSERT(GetHeader(userPtr) == reinterpret_cast<AllocHeader*>(headPtr));

        // Use the total number of bytes this arena has allocated after this allocation completes
        // to ensure we have comitted the pages necessary to service the allocations we have.
        const size_t totalBytes = (userPtr + size) - blockStart;
        const uint32_t totalPages = VirtualMemory::BytesToPages(totalBytes);
        if (totalPages > m_committedPages)
        {
            m_arena.Commit(m_committedPages, totalPages - m_committedPages);
            m_committedPages = totalPages;
        }
        m_allocatedBytes = totalBytes;

        // allocation completed, store the allocation size after unpoisoning the header and user memory
        HE_ASAN_UNPOISON_MEMORY(headPtr, sizeof(AllocHeader));
        HE_ASAN_UNPOISON_MEMORY(userPtr, size);
        AllocHeader* header = reinterpret_cast<AllocHeader*>(headPtr);
        header->size = size;
        return userPtr;
    }

    void* ArenaAllocator::Realloc(void* ptr, size_t newSize, size_t alignment) noexcept
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

    void ArenaAllocator::Free([[maybe_unused]] void* ptr) noexcept
    {
    #if HE_HAS_ASAN
        const AllocHeader* header = GetHeader(ptr);
        HE_ASAN_POISON_MEMORY(ptr, header->size);
        HE_ASAN_POISON_MEMORY(header, sizeof(AllocHeader));
    #endif
    }

    void ArenaAllocator::Clear() noexcept
    {
        m_arena.Decommit(0, m_arena.Size());
        HE_ASAN_POISON_MEMORY(m_arena.Block(), m_arena.ByteSize());
    }

    ArenaAllocator::AllocHeader* ArenaAllocator::GetHeader(void* userPtr) noexcept
    {
        uint8_t* p = static_cast<uint8_t*>(userPtr);
        uint8_t* h = AlignDown(p - sizeof(AllocHeader), alignof(AllocHeader));
        return reinterpret_cast<AllocHeader*>(h);
    }
}
