// Copyright Chad Engler

#include "he/core/allocator.h"

#include "he/core/config.h"
#include "he/core/types.h"
#include "he/core/utils.h"

#include "mimalloc.h"

#include <new>

namespace he
{
#if HE_ENABLE_DEFAULT_ALLOCATOR
    Allocator& Allocator::GetDefault()
    {
        return MiMallocAllocator::Get();
    }
#endif

    // Since we don't know static destruction order this ensures the allocator never actually
    // destructs and therefore will always outlive any static allocations.
    alignas(CrtAllocator) static uint8_t s_crtAllocatorMem[sizeof(CrtAllocator)];
    CrtAllocator& CrtAllocator::Get()
    {
        static CrtAllocator* s_allocator = ::new(s_crtAllocatorMem) CrtAllocator();
        return *s_allocator;
    }

    // Since we don't know static destruction order this ensures the allocator never actually
    // destructs and therefore will always outlive any static allocations.
    alignas(MiMallocAllocator) static uint8_t s_miMallocAllocatorMem[sizeof(MiMallocAllocator)];
    MiMallocAllocator& MiMallocAllocator::Get()
    {
        static MiMallocAllocator* s_allocator = ::new(s_miMallocAllocatorMem) MiMallocAllocator();
        return *s_allocator;
    }

    void* MiMallocAllocator::Malloc(size_t size, size_t alignment) noexcept
    {
        return mi_malloc_aligned(size, alignment);
    }

    void* MiMallocAllocator::Realloc(void* ptr, size_t newSize, size_t alignment) noexcept
    {
        return mi_realloc_aligned(ptr, newSize, alignment);
    }

    void MiMallocAllocator::Free(void* ptr) noexcept
    {
        mi_free(ptr);
    }
}

// Override global operators new and delete to use the default allocator.

#if defined(_MSC_VER) && defined(_Ret_notnull_) && defined(_Post_writable_byte_size_)
    // stay consistent with VCRT definitions
    #define HE_DECL_NEW(n)          __declspec(restrict) _Ret_notnull_ _Post_writable_byte_size_(n)
    #define HE_DECL_NEW_NOTHROW(n)  __declspec(restrict) _Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(n)
#else
    #define HE_DECL_NEW(n)
    #define HE_DECL_NEW_NOTHROW(n)
#endif

[[nodiscard]] HE_DECL_NEW(n) void* operator new(std::size_t n) noexcept(false) { return he::Allocator::GetDefault().Malloc(n); }
[[nodiscard]] HE_DECL_NEW(n) void* operator new[](std::size_t n) noexcept(false) { return he::Allocator::GetDefault().Malloc(n); }

[[nodiscard]] HE_DECL_NEW_NOTHROW(n) void* operator new(std::size_t n, const std::nothrow_t&) noexcept { return he::Allocator::GetDefault().Malloc(n); }
[[nodiscard]] HE_DECL_NEW_NOTHROW(n) void* operator new[](std::size_t n, const std::nothrow_t&) noexcept { return he::Allocator::GetDefault().Malloc(n); }

[[nodiscard]] void* operator new(std::size_t n, std::align_val_t al) noexcept(false) { return he::Allocator::GetDefault().Malloc(n, static_cast<size_t>(al)); }
[[nodiscard]] void* operator new[](std::size_t n, std::align_val_t al) noexcept(false) { return he::Allocator::GetDefault().Malloc(n, static_cast<size_t>(al)); }

[[nodiscard]] void* operator new(std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept { return he::Allocator::GetDefault().Malloc(n, static_cast<size_t>(al)); }
[[nodiscard]] void* operator new[](std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept { return he::Allocator::GetDefault().Malloc(n, static_cast<size_t>(al)); }

void operator delete(void* ptr) noexcept { he::Allocator::GetDefault().Free(ptr); }
void operator delete[](void* ptr) noexcept { he::Allocator::GetDefault().Free(ptr); }

void operator delete(void* ptr, const std::nothrow_t&) noexcept { he::Allocator::GetDefault().Free(ptr); }
void operator delete[](void* ptr, const std::nothrow_t&) noexcept { he::Allocator::GetDefault().Free(ptr); }

void operator delete(void* ptr, std::size_t) noexcept { he::Allocator::GetDefault().Free(ptr); }
void operator delete[](void* ptr, std::size_t) noexcept { he::Allocator::GetDefault().Free(ptr); }

void operator delete(void* ptr, std::align_val_t) noexcept { he::Allocator::GetDefault().Free(ptr); }
void operator delete[](void* ptr, std::align_val_t) noexcept { he::Allocator::GetDefault().Free(ptr); }

void operator delete(void* ptr, std::size_t, std::align_val_t) noexcept { he::Allocator::GetDefault().Free(ptr); }
void operator delete[](void* ptr, std::size_t, std::align_val_t) noexcept { he::Allocator::GetDefault().Free(ptr); }

void operator delete(void* ptr, std::align_val_t, const std::nothrow_t&) noexcept { he::Allocator::GetDefault().Free(ptr); }
void operator delete[](void* ptr, std::align_val_t, const std::nothrow_t&) noexcept { he::Allocator::GetDefault().Free(ptr); }
