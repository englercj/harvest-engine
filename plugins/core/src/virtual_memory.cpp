// Copyright Chad Engler

#include "he/core/virtual_memory.h"

#include "he/core/assert.h"
#include "he/core/enum_ops.h"
#include "he/core/system.h"
#include "he/core/utils.h"

namespace he
{
    size_t VirtualMemory::PageSize()
    {
        static const size_t s_pageSize = GetSystemInfo().pageSize;
        return s_pageSize;
    }

    uint32_t VirtualMemory::BytesToPages(size_t size)
    {
        const size_t pageSize = PageSize();
        const size_t pageCount = (size + (pageSize - 1)) / pageSize;
        return static_cast<uint32_t>(pageCount);
    }

    size_t VirtualMemory::PagesToBytes(uint32_t count)
    {
        const size_t pageSize = PageSize();
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
        return *this;
    }

    void* VirtualMemory::GetPage(uint32_t index) const
    {
        HE_ASSERT(index < m_size);
        return static_cast<uint8_t*>(m_block) + PagesToBytes(index);
    }

    const char* AsString(MemoryAccess x)
    {
        switch (x)
        {
            case MemoryAccess::None: return "None";
            case MemoryAccess::Read: return "Read";
            case MemoryAccess::ReadWrite: return "ReadWrite";
            case MemoryAccess::Execute: return "Execute";
            case MemoryAccess::ReadExecute: return "ReadExecute";
            case MemoryAccess::ReadWriteExecute: return "ReadWriteExecute";
        }
        return "<unknown>";
    }
}
