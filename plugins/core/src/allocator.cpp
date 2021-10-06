// Copyright Chad Engler

#include "he/core/allocator.h"

#include "he/core/compiler.h"
#include "he/core/config.h"
#include "he/core/utils.h"

#include <new>

namespace he
{
#if !HE_ENABLE_CUSTOM_ALLOCATORS
    Allocator& Allocator::GetDefault()
    {
        return CrtAllocator::Get();
    }

    Allocator& Allocator::GetTemp()
    {
        return CrtAllocator::Get();
    }
#endif

    CrtAllocator& CrtAllocator::Get()
    {
        // Hack to ensure the allocator is never actually destructed so it lives the entire
        // lifetime of the application.
        constexpr size_t Alignment = Max<size_t>(alignof(CrtAllocator), 8);
        static HE_ALIGNED(Alignment) char s_mem[sizeof(CrtAllocator)];
        static CrtAllocator* allocator = new(s_mem) CrtAllocator();
        return *allocator;
    }
}
