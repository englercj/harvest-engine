// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"

/// \def HE_HAS_ASAN
/// Defined as 1 when built with address sanitizer enabled.

/// \def HE_ASAN_POISON_MEMORY
/// Marks memory region [addr, addr+size) as unaddressable.
///
/// This memory must be previously allocated by the user program. Accessing addresses in this
/// region from instrumented code is forbidden until this region is unpoisoned. This function is
/// not guaranteed to poison the whole region - it may poison only subregion of [addr, addr+size)
/// due to ASan alignment restrictions.
///
/// This is NOT thread-safe in the sense that no two threads can (un)poison memory in the same
/// memory region simultaneously.

/// \def HE_ASAN_UNPOISON_MEMORY
/// Marks memory region [addr, addr+size) as addressable.
///
/// This memory must be previously allocated by the user program. Accessing addresses in this
/// region is allowed until this region is poisoned again. This function may unpoison a superregion
/// of [addr, addr+size) due to ASan alignment restrictions.
///
/// Method is NOT thread-safe in the sense that no two threads can (un)poison memory in the same
/// memory region simultaneously.


#if HE_HAS_FEATURE(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
    #define HE_HAS_ASAN 1
#else
    #define HE_HAS_ASAN 0
#endif

#if HE_HAS_ASAN
    extern "C" void __asan_poison_memory_region(void const volatile *addr, size_t size);
    extern "C" void __asan_unpoison_memory_region(void const volatile *addr, size_t size);

    #define HE_ASAN_POISON_MEMORY(addr, size)       __asan_poison_memory_region((addr), (size))
    #define HE_ASAN_UNPOISON_MEMORY(addr, size)     __asan_unpoison_memory_region((addr), (size))
#else
    #define HE_ASAN_POISON_MEMORY(addr, size)       ((void)(addr), (void)(size))
    #define HE_ASAN_UNPOISON_MEMORY(addr, size)     ((void)(addr), (void)(size))
#endif
