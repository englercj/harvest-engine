// Copyright Chad Engler

#include "he/core/allocator.h"

#include "he/core/assert.h"
#include "he/core/utils.h"

#if defined(HE_PLATFORM_WASM)

#include "wasm_core.js.h"

/// \def HE_ENABLE_WASM_MEMORY_VALIDATIONS
/// When enabled will perforce memory allocation validations, at the expense of performance.
#if !defined(HE_ENABLE_WASM_MEMORY_VALIDATIONS)
    #define HE_ENABLE_WASM_MEMORY_VALIDATIONS   0
#endif

// TODO: Use a mimalloc implementation.

namespace he
{
    void* CrtAllocator::Malloc(size_t size, size_t alignment) noexcept
    {
    }

    void* CrtAllocator::Realloc(void* ptr, size_t newSize, size_t alignment) noexcept
    {
    }

    void CrtAllocator::Free(void* ptr) noexcept
    {
    }
}

#endif
