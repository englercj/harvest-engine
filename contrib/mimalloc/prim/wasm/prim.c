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
                void* aligned_current = (void*)_mi_align_up((uintptr_t)current, try_alignment);  // and align from there to minimize wasted space
                alloc_size = _mi_align_up(((uint8_t*)aligned_current - (uint8_t*)current) + size, _mi_os_page_size());
                base = mi_memory_grow(alloc_size);
            }

            __c11_atomic_store(&mi_heap_grow_mutex, 0, __ATOMIC_SEQ_CST);
            break;
        }

        if (base != NULL)
        {
            p = (void*)_mi_align_up((uintptr_t)base, try_alignment);
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

//---------------------------------------------
// Commit/Reset
//---------------------------------------------

int _mi_prim_commit(void* addr, size_t size, bool* is_zero)
{
    MI_UNUSED(addr); MI_UNUSED(size);
    *is_zero = false;
    return 0;
}

int _mi_prim_decommit(void* addr, size_t size, bool* needs_recommit)
{
    MI_UNUSED(addr); MI_UNUSED(size);
    *needs_recommit = false;
    return 0;
}

int _mi_prim_reset(void* addr, size_t size)
{
    MI_UNUSED(addr); MI_UNUSED(size);
    return 0;
}

int _mi_prim_protect(void* addr, size_t size, bool protect)
{
    MI_UNUSED(addr); MI_UNUSED(size); MI_UNUSED(protect);
    return 0;
}

//---------------------------------------------
// Huge pages and NUMA nodes
//---------------------------------------------

int _mi_prim_alloc_huge_os_pages(void* hint_addr, size_t size, int numa_node, bool* is_zero, void** addr)
{
    MI_UNUSED(hint_addr); MI_UNUSED(size); MI_UNUSED(numa_node);
    *is_zero = true;
    *addr = NULL;
    return ENOSYS;
}

size_t _mi_prim_numa_node(void)
{
    return 0;
}

size_t _mi_prim_numa_node_count(void)
{
    return 1;
}

//----------------------------------------------------------------
// Clock
//----------------------------------------------------------------

extern uint64_t _he_prim_clock_now();

mi_msecs_t _mi_prim_clock_now(void)
{
    return (mi_msecs_t)_he_prim_clock_now();
}


//----------------------------------------------------------------
// Process info
//----------------------------------------------------------------

void _mi_prim_process_info(mi_process_info_t* pinfo)
{
    // use defaults
    MI_UNUSED(pinfo);
}

//----------------------------------------------------------------
// Output
//----------------------------------------------------------------

extern void _he_prim_out_stderr(const char* msg);

void _mi_prim_out_stderr(const char* msg)
{
    _he_prim_out_stderr(msg);
}

//----------------------------------------------------------------
// Environment
//----------------------------------------------------------------

extern bool _he_prim_getenv(const char* name, char* result, size_t result_size);

bool _mi_prim_getenv(const char* name, char* result, size_t result_size)
{
    return _he_prim_getenv(name, result, result_size);
}

//----------------------------------------------------------------
// Random
//----------------------------------------------------------------

extern bool _he_prim_random_buf(void* buf, size_t len);

bool _mi_prim_random_buf(void* buf, size_t buf_len)
{
    return _he_prim_random_buf(buf, buf_len);
}

//----------------------------------------------------------------
// Thread init/done
//----------------------------------------------------------------

extern void _he_prim_thread_init_auto_done();
extern void _he_prim_thread_done_auto_done();
extern void _he_prim_thread_associate_default_heap(mi_heap_t* heap);

void _mi_prim_thread_init_auto_done(void)
{
    _he_prim_thread_init_auto_done();
}

void _mi_prim_thread_done_auto_done(void)
{
    _he_prim_thread_done_auto_done();
}

void _mi_prim_thread_associate_default_heap(mi_heap_t* heap)
{
    _he_prim_thread_associate_default_heap(heap);
}
