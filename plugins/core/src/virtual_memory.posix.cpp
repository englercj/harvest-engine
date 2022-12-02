// Copyright Chad Engler

#include "he/core/virtual_memory.h"


#if defined(HE_PLATFORM_API_POSIX)

#include <sys/mman.h>

namespace he
{
    static int PosixMemProtection(MemoryAccess access)
    {
        switch (access)
        {
            case MemoryAccess::None: return PROT_NONE;
            case MemoryAccess::Read: return PROT_READ;
            case MemoryAccess::ReadWrite: return PROT_READ | PROT_WRITE;
            case MemoryAccess::Execute: return PROT_EXEC;
            case MemoryAccess::ReadExecute: return PROT_READ | PROT_EXEC;
            case MemoryAccess::ReadWriteExecute: return PROT_READ | PROT_WRITE | PROT_EXEC;
        }

        HE_VERIFY(false, HE_MSG("Unknown memory access enumerator."), HE_KV(access, access));
        return PROT_NONE;
    }

    Result VirtualMemory::Reserve(size_t count)
    {
        const size_t size = PagesToBytes(count);
        m_block = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (m_block == MAP_FAILED)
        {
            m_block = nullptr;
            return Result::FromLastError();
        }

        m_size = count;
        return Result::Success;
    }

    Result VirtualMemory::Release(void* block)
    {
        HE_ASSERT(m_block && m_size > 0);
        const size_t size = PagesToBytes(m_size);
        const int rc = munmap(block, size);
        m_block = nullptr;
        m_size = 0;
        return rc == 0 ? Result::Success : Result::FromLastError();
    }

    void* VirtualMemory::Commit(size_t index, size_t count, MemoryAccess access)
    {
        HE_ASSERT(index < m_size);
        HE_ASSERT((index + count) <= m_size);

        void* start = GetPage(index);
        const size_t size = PagesToBytes(count);
        const int flags = PosixMemProtection(access);
        const int rc = mprotect(start, size, flags);
        return rc == 0 ? start : nullptr;
    }

    Result VirtualMemory::Decommit(void* block, size_t index, size_t count)
    {
        HE_ASSERT(index < m_size);
        HE_ASSERT((index + count) <= m_size);

        // Posix doesn't really have a decommit concept, but we can tell the kernel that we don't
        // need these pages anymore. The kernel may decide to free the pages if there is memory
        // presure, but there is no guarantee.
        void* start = GetPage(index);
        const size_t size = PagesToBytes(count);
        const int rc = madvise(start, size, MADV_FREE);
        return rc == 0 ? Result::Success : Result::FromLastError();
    }

    Result VirtualMemory::Protect(size_t index, size_t count, MemoryAccess access)
    {
        HE_ASSERT(index < m_size);
        HE_ASSERT((index + count) <= m_size);

        void* start = GetPage(index);
        const size_t size = PagesToBytes(count);
        const int flags = PosixMemProtection(access);
        const int rc = mprotect(start, size, flags);
        return rc == 0 ? start : nullptr;
    }
}

#endif
