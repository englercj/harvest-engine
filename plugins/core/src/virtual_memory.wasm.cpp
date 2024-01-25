// Copyright Chad Engler

#include "he/core/virtual_memory.h"

#if defined(HE_PLATFORM_WASM)

// WASM doesn't have virtual memory concepts, at least not explicitly.
// A good discussion can be found here: https://github.com/WebAssembly/design/issues/1397

namespace he
{
    Result VirtualMemory::Reserve(uint32_t count)
    {
        return Result::NotSupported;
    }

    Result VirtualMemory::Release(void* block)
    {
        return Result::NotSupported;
    }

    void* VirtualMemory::Commit(uint32_t index, uint32_t count, MemoryAccess access)
    {
        return Result::NotSupported;
    }

    Result VirtualMemory::Decommit(void* block, uint32_t index, uint32_t count)
    {
        return Result::NotSupported;
    }

    Result VirtualMemory::Protect(uint32_t index, uint32_t count, MemoryAccess access)
    {
        return Result::NotSupported;
    }
}

#endif
