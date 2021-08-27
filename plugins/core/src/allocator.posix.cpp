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

        // The POSIX spec doesn't say we can pass posix_memalign allocated pointers to realloc,
        // so what follows is not portable. That said, looking at glibc it seems like this
        // should be fine.
        //
        // Since realloc will avoid copies by extending the allocated memory block, or by
        // reassigning virtual pages when it needs a new block we really want to use it to avoid
        // copies of the memory bytes if we can.
        //
        // In the degenerate case where realloc gives us a new block that is improperly aligned
        // we need to perform a copy to a new allocation. This is still only one copy because
        // realloc will actually just reassign virtual pages rather than perform a byte copy.

        void* p = realloc(ptr, newSize);

        // We can return the pointer as-is if it is properly aligned.
        if (IsAligned(p, alignment))
            return p;

        // This is verifying an assumption that we can't possibly have an unaligned pointer if
        // it hasn't been reallocated to a new block. If you hit this, it is likely the alignment
        // value passed to Realloc did not match the value you originally sent to Malloc.
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
