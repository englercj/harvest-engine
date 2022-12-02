// Copyright Chad Engler

#include "he/core/virtual_memory.h"

#include "he/core/assert.h"
#include "he/core/system.h"
#include "he/core/utils.h"

namespace he
{
    size_t BytesToPages(size_t size)
    {
        static const size_t s_pageSize = GetSystemInfo().pageSize;
        const size_t pageCount = (size + (s_pageSize - 1)) / s_pageSize;
        return pageCount;
    }

    size_t PagesToBytes(size_t count)
    {
        static const size_t s_pageSize = GetSystemInfo().pageSize;
        const size_t byteSize = count * s_pageSize;
        return byteSize;
    }

    VirtualMemory::VirtualMemory(VirtualMemory&& x) noexcept
        : m_block(Exchange(x.m_block, nullptr))
        , m_size(Exchange(x.m_size, 0))
    {}

    VirtualMemory& VirtualMemory::operator=(VirtualMemory&& x)
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

