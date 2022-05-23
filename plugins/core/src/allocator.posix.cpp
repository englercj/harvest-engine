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
    void* CrtAllocator::Malloc(size_t size, size_t alignment)
    {
        alignment = AlignUp(alignment, sizeof(void*));

        void* p;
        int rc = posix_memalign(&p, alignment, size);
        return rc ? nullptr : p;
    }

    void* CrtAllocator::Realloc(void* ptr, size_t newSize, size_t alignment)
    {
        if (ptr == nullptr)
            return Malloc(newSize, alignment);

        if (newSize == 0)
        {
            free(ptr);
            return nullptr;
        }

        alignment = AlignUp(alignment, sizeof(void*));

        // The POSIX spec doesn't say we can pass posix_memalign() allocated pointers to realloc(),
        // so what follows isn't portable. That said, glibc seems to implement realloc in such a
        // way that it should actually be safe to do so. Posix does require free() to work with
        // posix_memalign() results, and realloc() has the same smarts free() does in glibc.
        //
        // Since realloc may avoid copies by extending the allocated memory block, or by
        // reassigning virtual pages when it needs a new block we want to use it to avoid copies
        // of the memory bytes if we can.
        //
        // In the degenerate case where realloc() gives us a new block that is improperly aligned
        // we need to perform a copy to a new allocation. It is likely that this will be the only
        // copy operation because realloc() may just reassign virtual pages rather than perform a
        // copy.
        //
        // Sources:
        // - https://github.com/bminor/glibc/blob/2d5ec6692f5746ccb11db60976a6481ef8e9d74f/malloc/malloc.c
        // - https://stackoverflow.com/a/9078627/725851
        void* p = realloc(ptr, newSize);

        // We can return the pointer as-is if it is properly aligned.
        if (IsAligned(p, alignment)) [[likely]]
            return p;

        // This is documenting the assumption that we can't possibly have an unaligned pointer if
        // the pointer hasn't changed. This may get tripped if `alignment` is different than what
        // `ptr` was originally allocated with. This is not allowed.
        HE_ASSERT(p != ptr);

        // We got an unaligned pointer from realloc, so we need to manually do an aligned malloc
        p = Malloc(newSize, alignment);
        if (p == nullptr)
        {
            free(ptr);
            return p;
        }

        // Finally, copy over the old data
        const size_t oldSize = malloc_usable_size(ptr);
        memcpy(p, ptr, Min(oldSize, newSize));
        free(ptr);
        return p;
    }

    void CrtAllocator::Free(void* ptr)
    {
        free(ptr);
    }
}

#endif
