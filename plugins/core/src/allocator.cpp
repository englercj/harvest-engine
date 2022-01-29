// Copyright Chad Engler

#include "he/core/allocator.h"

#include "he/core/assert.h"
#include "he/core/compiler.h"
#include "he/core/config.h"
#include "he/core/utils.h"

#include <new>

namespace he
{
#if !HE_ENABLE_CUSTOM_ALLOCATORS
    Allocator& Allocator::GetDefault()
    {
        return CrtAllocator::Get();
    }

    Allocator& Allocator::GetTemp()
    {
        return CrtAllocator::Get();
    }
#endif

    CrtAllocator& CrtAllocator::Get()
    {
        // Since we don't know static destruciton order this ensures the allocator never actually
        // destructs and therefore will always outlive any static allocations.
        constexpr size_t Alignment = Max<size_t>(alignof(CrtAllocator), 8);
        static HE_ALIGNED(Alignment) char s_mem[sizeof(CrtAllocator)];
        static CrtAllocator* s_allocator = new(s_mem) CrtAllocator();
        return *s_allocator;
    }

    LinearPageAllocator::LinearPageAllocator(size_t pageSize, Allocator& allocator)
        : m_allocator(allocator)
        , m_pageSize(pageSize)
    {}

    LinearPageAllocator::~LinearPageAllocator()
    {
        Reset();
    }

    void* LinearPageAllocator::Malloc(size_t size, size_t alignment)
    {
        HE_ASSERT(size > 0 && IsPowerOf2(alignment) && size < m_pageSize);

        if (!CanFit(size, alignment))
            AllocPage();

        uint8_t* bufferStart = reinterpret_cast<uint8_t*>(m_lastPage) + sizeof(PageHeader) + m_pageOffset;
        uint8_t* allocStart = AlignUp(bufferStart, alignment);
        const uint8_t* allocEnd = allocStart + size;
        const intptr_t allocSize = allocEnd - bufferStart;
        const size_t newAllocSize = m_pageOffset + allocSize;

        m_pageOffset = newAllocSize;
        return static_cast<void*>(allocStart);
    }

    void* LinearPageAllocator::Realloc(void* ptr, size_t newSize, size_t alignment)
    {
        if (ptr == nullptr)
            return _aligned_malloc(newSize, alignment);

        if (newSize == 0)
        {
            _aligned_free(ptr);
            return nullptr;
        }

        alignment = AlignUp(alignment, sizeof(void*));
        return _aligned_realloc(ptr, newSize, alignment);
    }

    void LinearPageAllocator::Clear()
    {
        PageHeader* p = m_lastPage;
        while (p)
        {
            m_currentPage = p;
            p = p->previous;
        }

        m_pageOffset = 0;
    }

    void LinearPageAllocator::Reset()
    {
        while (m_lastPage)
        {
            void* mem = m_lastPage;
            m_lastPage = m_lastPage->previous;

            m_allocator.Free(mem);
        }

        m_currentPage = nullptr;
        m_lastPage = nullptr;
        m_pageOffset = 0;
    }

    bool LinearPageAllocator::CanFit(size_t size, size_t alignment) const
    {
        if (!m_currentPage)
            return false;

        const uint8_t* bufferStart = reinterpret_cast<uint8_t*>(m_currentPage) + sizeof(PageHeader) + m_pageOffset;
        const uint8_t* allocStart = AlignUp(bufferStart, alignment);

        // Check for alignment overflow
        if (allocStart < bufferStart)
            return false;

        const uint8_t* allocEnd = allocStart + size;

        // Check for size overflow
        if (allocEnd <= allocStart)
            return false;

        const intptr_t allocSize = allocEnd - bufferStart;
        const size_t newAllocSize = m_pageOffset + allocSize;

        // Check for if the size is too large
        if (newAllocSize > m_pageSize)
            return false;

        return true;
    }

    void LinearPageAllocator::AllocPage()
    {
        if (m_currentPage < m_lastPage)
        {
            m_currentPage = m_currentPage->next;
            HE_ASSERT(m_currentPage && m_currentPage <= m_lastPage);
            return;
        }

        void* mem = m_allocator.Malloc(sizeof(PageHeader) + m_pageSize, alignof(PageHeader));
        PageHeader* header = ::new(mem) PageHeader;
        header->next = nullptr;
        header->previous = m_lastPage;

        if (m_lastPage)
        {
            m_lastPage->next = header;
        }

        m_currentPage = header;
        m_lastPage = header;
    }
}
