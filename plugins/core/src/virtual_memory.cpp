// Copyright Chad Engler

#include "he/core/virtual_memory.h"

#include "he/core/assert.h"
#include "he/core/system.h"
#include "he/core/utils.h"

namespace he
{
    size_t VirtualMemory::GetPageSize()
    {
        static const size_t s_pageSize = GetSystemInfo().pageSize;
        return s_pageSize;
    }

    size_t VirtualMemory::BytesToPages(size_t size)
    {
        const size_t pageSize = GetPageSize();
        const size_t pageCount = (size + (pageSize - 1)) / pageSize;
        return pageCount;
    }

    size_t VirtualMemory::PagesToBytes(size_t count)
    {
        const size_t pageSize = GetPageSize();
        const size_t byteSize = count * pageSize;
        return byteSize;
    }

    VirtualMemory::VirtualMemory(VirtualMemory&& x) noexcept
        : m_block(Exchange(x.m_block, nullptr))
        , m_size(Exchange(x.m_size, 0))
    {}

    VirtualMemory& VirtualMemory::operator=(VirtualMemory&& x) noexcept
    {
        m_block = Exchange(x.m_block, nullptr);
        m_size = Exchange(x.m_size, 0);
    }

    const void* VirtualMemory::GetPage(size_t index) const
    {
        HE_ASSERT(index < m_size);
        return static_cast<uint8_t*>(m_block) + PagesToBytes(index);
    }
}

