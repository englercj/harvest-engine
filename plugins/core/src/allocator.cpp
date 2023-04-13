// Copyright Chad Engler

#include "he/core/allocator.h"

#include "he/core/config.h"
#include "he/core/types.h"
#include "he/core/utils.h"

#include <new>

namespace he
{
#if !HE_USER_DEFINED_DEFAULT_ALLOCATOR
    Allocator& Allocator::GetDefault()
    {
        return CrtAllocator::Get();
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
}

// Override global operators new and delete to use the default allocator.

// replaceable allocation functions

[[nodiscard]] void* operator new(size_t n) { return he::Allocator::GetDefault().Malloc(static_cast<uint32_t>(n)); }
[[nodiscard]] void* operator new[](size_t n) { return he::Allocator::GetDefault().Malloc(static_cast<uint32_t>(n)); }

[[nodiscard]] void* operator new(size_t count, std::align_val_t al) { return he::Allocator::GetDefault().Malloc(static_cast<uint32_t>(count), static_cast<uint32_t>(al)); }
[[nodiscard]] void* operator new[](size_t count, std::align_val_t al) { return he::Allocator::GetDefault().Malloc(static_cast<uint32_t>(count), static_cast<uint32_t>(al)); }

[[nodiscard]] void* operator new(size_t n, const std::nothrow_t&) noexcept { return he::Allocator::GetDefault().Malloc(static_cast<uint32_t>(n)); }
[[nodiscard]] void* operator new[](size_t n, const std::nothrow_t&) noexcept { return he::Allocator::GetDefault().Malloc(static_cast<uint32_t>(n)); }

[[nodiscard]] void* operator new(size_t count, std::align_val_t al, const std::nothrow_t&) noexcept { return he::Allocator::GetDefault().Malloc(static_cast<uint32_t>(count), static_cast<uint32_t>(al)); }
[[nodiscard]] void* operator new[](size_t count, std::align_val_t al, const std::nothrow_t&) noexcept { return he::Allocator::GetDefault().Malloc(static_cast<uint32_t>(count), static_cast<uint32_t>(al)); }

// replaceable usual deallocation functions

void operator delete(void* ptr) noexcept { he::Allocator::GetDefault().Free(ptr); }
void operator delete[](void* ptr) noexcept { he::Allocator::GetDefault().Free(ptr); }

void operator delete(void* ptr, std::align_val_t) noexcept { he::Allocator::GetDefault().Free(ptr); }
void operator delete[](void* ptr, std::align_val_t) noexcept { he::Allocator::GetDefault().Free(ptr); }

void operator delete(void* ptr, std::size_t) noexcept { he::Allocator::GetDefault().Free(ptr); }
void operator delete[](void* ptr, std::size_t) noexcept { he::Allocator::GetDefault().Free(ptr); }

void operator delete(void* ptr, std::size_t, std::align_val_t) noexcept { he::Allocator::GetDefault().Free(ptr); }
void operator delete[](void* ptr, std::size_t, std::align_val_t) noexcept { he::Allocator::GetDefault().Free(ptr); }
