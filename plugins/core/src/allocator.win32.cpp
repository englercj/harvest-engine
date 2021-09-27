// Copyright Chad Engler

#include "he/core/allocator.h"

#include "he/core/utils.h"

#if defined(HE_PLATFORM_API_WIN32)

#include <malloc.h>

namespace he
{
    void* CrtAllocator::Malloc(size_t size, size_t alignment)
    {
        alignment = AlignUp(alignment, sizeof(void*));
        return _aligned_malloc(size, alignment);
    }

    void* CrtAllocator::Realloc(void* ptr, size_t newSize, size_t alignment)
    {
        if (ptr == nullptr)
            return _aligned_malloc(newSize, alignment);

        if (newSize == 0)
        {
            _aligned_free(ptr);
            return nullptr;
        }

        alignment = AlignUp(alignment, sizeof(void*));
        return _aligned_realloc(ptr, newSize, alignment);
    }

    void CrtAllocator::Free(void* ptr)
    {
        _aligned_free(ptr);
    }
}

#endif
