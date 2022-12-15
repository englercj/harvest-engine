// Copyright Chad Engler

#include "he/core/allocator.h"

#include "he/core/assert.h"
#include "he/core/utils.h"

#if defined(HE_PLATFORM_API_POSIX)

#include <cstring>
#include <malloc.h>
#include <stdlib.h>

namespace he
{
    void* CrtAllocator::Malloc(size_t size, size_t alignment) noexcept
    {
        // posix_memalign requires alignment to be a power of two and a multiple of sizeof(void*)
        alignment = AlignUp(alignment, sizeof(void*));
        HE_ASSERT(IsPowerOf2(alignment));

        void* p;
        int rc = posix_memalign(&p, alignment, size);
        return rc ? nullptr : p;
    }

    void* CrtAllocator::Realloc(void* ptr, size_t newSize, size_t alignment) noexcept
    {
        if (ptr == nullptr)
            return Malloc(newSize, alignment);

        if (newSize == 0)
        {
            free(ptr);
            return nullptr;
        }

        alignment = AlignUp(alignment, sizeof(void*));

        // Passing posix_memalign pointers into realloc isn't technically supported, but reading
        // the glibc source and some testing on linux seems to indicate that it is OK if the
        // allocation isn't overaligned.
        //
        // Sources:
        // - https://github.com/bminor/glibc/blob/2d5ec6692f5746ccb11db60976a6481ef8e9d74f/malloc/malloc.c
        // - https://stackoverflow.com/a/9078627/725851
        if (alignment == sizeof(void*))
            return realloc(ptr, newSize);

        // We have an overaligned pointer so we need to manually do an aligned malloc and copy
        void* p = Malloc(newSize, alignment);
        if (p == nullptr)
        {
            free(ptr);
            return p;
        }

        const size_t oldSize = malloc_usable_size(ptr);
        memcpy(p, ptr, Min(oldSize, newSize));
        free(ptr);
        return p;
    }

    void CrtAllocator::Free(void* ptr) noexcept
    {
        free(ptr);
    }
}

#endif
