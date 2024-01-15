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
    config->page_size = 64*MI_KiB; // WebAssembly has a fixed page size: 64KiB
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
    // wasm heap cannot be shrunk
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
