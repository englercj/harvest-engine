// Copyright Chad Engler

// This file is included in `src/prim/prim.c`

#include "mimalloc.h"
#include "mimalloc/internal.h"
#include "mimalloc/atomic.h"
#include "mimalloc/prim.h"

//---------------------------------------------
// Initialize
//---------------------------------------------

void _mi_prim_mem_init(mi_os_mem_config_t* config)
{
    config->page_size = 64 * MI_KiB; // WebAssembly has a fixed page size: 64KiB
    config->alloc_granularity = 16;
    config->has_overcommit = false;
    config->must_free_whole = true;
    config->has_virtual_reserve = false;
}

//---------------------------------------------
// Free
//---------------------------------------------

int _mi_prim_free(void* addr, size_t size)
{
    MI_UNUSED(addr);
    MI_UNUSED(size);
    mi_assert_internal(size > 0 && (size % _mi_os_page_size()) == 0);
    // size_t pageCount = size / _mi_os_page_size();
    // __builtin_wasm_memory_discard(addr, pageCount);
    return 0;
}

//---------------------------------------------
// Allocation
//---------------------------------------------

static void* mi_memory_grow(size_t size)
{
    size_t base = (size > 0 ? __builtin_wasm_memory_grow(0, _mi_divide_up(size, _mi_os_page_size()))
                            : __builtin_wasm_memory_size(0));

    if (base == SIZE_MAX)
        return NULL;

    return (void*)(base * _mi_os_page_size());
}

static _Atomic(uint32_t) mi_heap_grow_mutex = 0;

static void* mi_prim_mem_grow(size_t size, size_t try_alignment)
{
    void* p = NULL;
    if (try_alignment <= 1)
    {
        p = mi_memory_grow(size);
    }
    else
    {
        void* base = NULL;
        size_t alloc_size = 0;

        while (true)
        {
            // Spin until we can get the lock to avoid thread interaction between getting the
            // current size and actual allocation.
            uint32_t expected = 0;
            if (!__c11_atomic_compare_exchange_weak(&mi_heap_grow_mutex, &expected, 1, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
                continue;

            void* current = mi_memory_grow(0);  // get current size
            if (current != NULL)
            {
                void* aligned_current = mi_align_up_ptr(current, try_alignment);  // and align from there to minimize wasted space
                alloc_size = _mi_align_up(((uint8_t*)aligned_current - (uint8_t*)current) + size, _mi_os_page_size());
                base = mi_memory_grow(alloc_size);
            }

            __c11_atomic_store(&mi_heap_grow_mutex, 0, __ATOMIC_SEQ_CST);
            break;
        }

        if (base != NULL)
        {
            p = mi_align_up_ptr(base, try_alignment);
            if ((uint8_t*)p + size > (uint8_t*)base + alloc_size)
            {
                // another thread used wasm_memory_grow in-between and we do not have enough
                // space after alignment. Give up (and waste the space as we cannot shrink :-( )
                // (in `mi_os_mem_alloc_aligned` this will fall back to overallocation to align)
                p = NULL;
            }
        }
    }
    /*
    if (p == NULL) {
        _mi_warning_message("unable to allocate sbrk/wasm_memory_grow OS memory (%zu bytes, %zu alignment)\n", size, try_alignment);
        errno = ENOMEM;
        return NULL;
    }
    */
    mi_assert_internal(p == NULL || try_alignment == 0 || (uintptr_t)p % try_alignment == 0);
    return p;
}

// Note: the `try_alignment` is just a hint and the returned pointer is not guaranteed to be aligned.
int _mi_prim_alloc(size_t size, size_t try_alignment, bool commit, bool allow_large, bool* is_large, bool* is_zero, void** addr)
{
    MI_UNUSED(allow_large);
    MI_UNUSED(commit);
    *is_large = false;
    *is_zero = false;
    *addr = mi_prim_mem_grow(size, try_alignment);
    return (*addr != NULL ? 0 : ENOMEM);
}
