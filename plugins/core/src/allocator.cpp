// Copyright Chad Engler

#include "he/core/allocator.h"

#include "he/core/assert.h"
#include "he/core/compiler.h"
#include "he/core/config.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/types.h"
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
        alignas(Alignment) static uint8_t s_mem[sizeof(CrtAllocator)];
        static CrtAllocator* s_allocator = new(s_mem) CrtAllocator();
        return *s_allocator;
    }

    ArenaAllocator::ArenaAllocator(uint32_t maxSize) noexcept
    {
        const Result r = m_arena.Reserve(BytesToPages(maxSize));
        HE_VERIFY(r, HE_KV(result, r));
    }

    ArenaAllocator::ArenaAllocator(ArenaAllocator&& x) noexcept
        : m_arena(Move(x.m_arena))
        , m_committed(Exchange(x.m_committed, 0))
        , m_allocated(Exchange(x.m_allocated, 0))
    {}

    void* ArenaAllocator::Malloc(size_t size, size_t alignment = DefaultAlignment) noexcept
    {
        const uint8_t* start = AlignUp(m_arena + m_size, alignment);
        const void* result = VirtualCommit(start, size);
    }

    void* ArenaAllocator::Realloc(void* ptr, size_t newSize, size_t alignment = DefaultAlignment) noexcept
    {

    }

    void ArenaAllocator::Free(void*) noexcept
    {
    }

    void ArenaAllocator::Clear()
    {

    }

    void ArenaAllocator::Reset()
    {

    }



    LinearPageAllocator::LinearPageAllocator(size_t pageSize, Allocator& allocator) noexcept
        : m_allocator(allocator)
        , m_pageSize(pageSize)
    {}

    LinearPageAllocator::LinearPageAllocator(LinearPageAllocator&& x) noexcept
        : m_allocator(x.m_allocator)
        , m_currentPage(Exchange(x.m_currentPage, nullptr))
        , m_lastPage(Exchange(x.m_lastPage, nullptr))
        , m_pageOffset(Exchange(x.m_pageOffset, 0))
        , m_pageSize(x.m_pageSize)
    {}

    LinearPageAllocator::~LinearPageAllocator() noexcept
    {
        Reset();
    }

    void* LinearPageAllocator::Malloc(size_t size, size_t alignment) noexcept
    {
        HE_ASSERT(size > 0 && IsPowerOf2(alignment) && size < m_pageSize);

        if (!CanFit(size, alignment))
            AllocPage();

        uint8_t* bufferStart = reinterpret_cast<uint8_t*>(m_currentPage) + sizeof(PageHeader) + m_pageOffset;
        uint8_t* allocStart = AlignUp(bufferStart, alignment);
        const uint8_t* allocEnd = allocStart + size;
        const intptr_t allocSize = allocEnd - bufferStart;
        const size_t newAllocSize = m_pageOffset + allocSize;

        HE_ASSERT(allocEnd <= (reinterpret_cast<uint8_t*>(m_currentPage) + sizeof(PageHeader) + m_pageSize));

        m_pageOffset = newAllocSize;
        return static_cast<void*>(allocStart);
    }

    void* LinearPageAllocator::Realloc(void*, size_t newSize, size_t alignment) noexcept
    {
        if (newSize == 0)
            return nullptr;

        return Malloc(newSize, alignment);
    }

    void LinearPageAllocator::Clear()
    {
        m_currentPage = m_lastPage;
        while (m_currentPage)
        {
            m_currentPage = m_currentPage->previous;
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
            m_pageOffset = 0;
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
        m_pageOffset = 0;
    }
}

// Override global operators new and delete to use the default allocator.

// replaceable allocation functions

[[nodiscard]] void* operator new(size_t n) { return he::Allocator::GetDefault().Malloc(static_cast<uint32_t>(n)); }
[[nodiscard]] void* operator new[](size_t n) { return he::Allocator::GetDefault().Malloc(static_cast<uint32_t>(n)); }

[[nodiscard]] void* operator new(size_t count, std::align_val_t al) { return he::Allocator::GetDefault().Malloc(static_cast<uint32_t>(count), static_cast<uint32_t>(al)); }
[[nodiscard]] void* operator new[](size_t count, std::align_val_t al) { return he::Allocator::GetDefault().Malloc(static_cast<uint32_t>(count), static_cast<uint32_t>(al)); }

[[nodiscard]] void* operator new(size_t n, const std::nothrow_t&) noexcept { return he::Allocator::GetDefault().Malloc(static_cast<uint32_t>(n)); }
[[nodiscard]] void* operator new[](size_t n, const std::nothrow_t&) noexcept { return he::Allocator::GetDefault().Malloc(static_cast<uint32_t>(n)); }

[[nodiscard]] void* operator new(size_t count, std::align_val_t al, const std::nothrow_t&) noexcept { return he::Allocator::GetDefault().Malloc(static_cast<uint32_t>(count), static_cast<uint32_t>(al)); }
[[nodiscard]] void* operator new[](size_t count, std::align_val_t al, const std::nothrow_t&) noexcept { return he::Allocator::GetDefault().Malloc(static_cast<uint32_t>(count), static_cast<uint32_t>(al)); }

// replaceable usual deallocation functions

void operator delete(void* ptr) noexcept { he::Allocator::GetDefault().Free(ptr); }
void operator delete[](void* ptr) noexcept { he::Allocator::GetDefault().Free(ptr); }

void operator delete(void* ptr, std::align_val_t) noexcept { he::Allocator::GetDefault().Free(ptr); }
void operator delete[](void* ptr, std::align_val_t) noexcept { he::Allocator::GetDefault().Free(ptr); }

void operator delete(void* ptr, std::size_t) noexcept { he::Allocator::GetDefault().Free(ptr); }
void operator delete[](void* ptr, std::size_t) noexcept { he::Allocator::GetDefault().Free(ptr); }

void operator delete(void* ptr, std::size_t, std::align_val_t) noexcept { he::Allocator::GetDefault().Free(ptr); }
void operator delete[](void* ptr, std::size_t, std::align_val_t) noexcept { he::Allocator::GetDefault().Free(ptr); }
