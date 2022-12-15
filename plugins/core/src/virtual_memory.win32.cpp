// Copyright Chad Engler

#include "he/core/virtual_memory.h"

#include "he/core/assert.h"
#include "he/core/enum_fmt.h"
#include "he/core/log.h"
#include "he/core/system.h"
#include "he/core/utils.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

namespace he
{
    static DWORD Win32MemProtection(MemoryAccess access)
    {
        switch (access)
        {
            case MemoryAccess::None: return PAGE_NOACCESS;
            case MemoryAccess::Read: return PAGE_READONLY;
            case MemoryAccess::ReadWrite: return PAGE_READWRITE;
            case MemoryAccess::Execute: return PAGE_EXECUTE;
            case MemoryAccess::ReadExecute: return PAGE_EXECUTE_READ;
            case MemoryAccess::ReadWriteExecute: return PAGE_EXECUTE_READWRITE;
        }

        HE_VERIFY(false, HE_MSG("Unknown memory access enumerator."), HE_KV(access, access));
        return PAGE_NOACCESS;
    }

    Result VirtualMemory::Reserve(uint32_t count)
    {
        const size_t size = PagesToBytes(count);
        m_block = ::VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
        if (m_block == nullptr)
            return Result::FromLastError();

        m_size = count;
        return Result::Success;
    }

    Result VirtualMemory::Release()
    {
        HE_ASSERT(m_block && m_size > 0);
        const BOOL r = ::VirtualFree(m_block, 0, MEM_RELEASE);
        m_block = nullptr;
        m_size = 0;
        return r ? Result::Success : Result::FromLastError();
    }

    void* VirtualMemory::Commit(uint32_t index, uint32_t count, MemoryAccess access)
    {
        HE_ASSERT(index < m_size);
        HE_ASSERT((index + count) <= m_size);

        void* start = GetPage(index);
        const size_t size = PagesToBytes(count);
        const DWORD flags = Win32MemProtection(access);
        return ::VirtualAlloc(start, size, MEM_COMMIT, flags);
    }

    Result VirtualMemory::Decommit(uint32_t index, uint32_t count)
    {
        HE_ASSERT(index < m_size);
        HE_ASSERT((index + count) <= m_size);

        void* start = GetPage(index);
        const size_t size = PagesToBytes(count);
        const BOOL r = ::VirtualFree(start, size, MEM_DECOMMIT);
        return r ? Result::Success : Result::FromLastError();
    }

    Result VirtualMemory::Protect(uint32_t index, uint32_t count, MemoryAccess access)
    {
        HE_ASSERT(index < m_size);
        HE_ASSERT((index + count) <= m_size);

        void* start = GetPage(index);
        const size_t size = PagesToBytes(count);
        const DWORD flags = Win32MemProtection(access);

        DWORD oldProt = 0;
        const BOOL r = ::VirtualProtect(start, size, flags, &oldProt);
        return r ? Result::Success : Result::FromLastError();
    }
}

#endif
