// Copyright Chad Engler

#include "he/core/string_pool.h"

#include "he/core/allocator.h"

namespace he
{
    alignas(StringPool) static uint8_t s_defaultStringPoolMem[sizeof(StringPool)];
    StringPool& StringPool::GetDefault()
    {
        static StringPool* s_pool = ::new(s_defaultStringPoolMem) StringPool();
        return *s_pool;
    }

    StringPoolRef StringPool::Add(StringView str)
    {
        HE_UNUSED(str);
        return {};
    }

    StringView StringPool::Get(StringPoolRef ref) const
    {
        HE_UNUSED(ref);
        return {};
    }

    StringPoolRef StringPool::Add(StringView str, uint64_t hash)
    {
        HE_UNUSED(str, hash);
        return {};
    }
}
