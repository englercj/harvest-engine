// Copyright Chad Engler

#include "he/core/allocator.h"

#include "he/core/assert.h"
#include "he/core/utils.h"

#include "mimalloc.h"

#if defined(HE_PLATFORM_API_WASM)

namespace he
{
    // For WASM we use mi_malloc as the CrtAllocator because the only platform-specific API we
    // have for WASM is __builtin_wasm_memory_grow which grows the heap but isn't an allocator.

    void* CrtAllocator::Malloc(size_t size, size_t alignment) noexcept
    {
        return mi_malloc_aligned(size, alignment);
    }

    void* CrtAllocator::Realloc(void* ptr, size_t newSize, size_t alignment) noexcept
    {
        if (ptr == nullptr)
            return mi_malloc_aligned(newSize, alignment);

        if (newSize == 0)
        {
            mi_free(ptr);
            return nullptr;
        }

        return mi_realloc_aligned(ptr, newSize, alignment);
    }

    void CrtAllocator::Free(void* ptr) noexcept
    {
        mi_free(ptr);
    }
}

#endif
