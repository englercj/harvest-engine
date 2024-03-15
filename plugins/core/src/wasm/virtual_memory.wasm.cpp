// Copyright Chad Engler

#include "he/core/virtual_memory.h"

#if defined(HE_PLATFORM_API_WASM)

// WASM doesn't have virtual memory concepts, at least not explicitly.
// A good discussion can be found here: https://github.com/WebAssembly/design/issues/1397

namespace he
{
    Result VirtualMemory::Reserve([[maybe_unused]] uint32_t count)
    {
        return Result::NotSupported;
    }

    Result VirtualMemory::Release()
    {
        return Result::NotSupported;
    }

    void* VirtualMemory::Commit([[maybe_unused]] uint32_t index, [[maybe_unused]] uint32_t count, [[maybe_unused]] MemoryAccess access)
    {
        return nullptr;
    }

    Result VirtualMemory::Decommit([[maybe_unused]] uint32_t index, [[maybe_unused]] uint32_t count)
    {
        return Result::NotSupported;
    }

    Result VirtualMemory::Protect([[maybe_unused]] uint32_t index, [[maybe_unused]] uint32_t count, [[maybe_unused]] MemoryAccess access)
    {
        return Result::NotSupported;
    }
}

#endif
