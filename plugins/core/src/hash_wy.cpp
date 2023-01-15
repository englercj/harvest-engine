// Copyright Chad Engler

#include "he/core/hash.h"

#include "wyhash.h"

namespace he
{
    uint64_t WyHash::Mem(const void* data, uint32_t len, uint64_t seed)
    {
        return wyhash::Hash(data, len, seed);
    }
}
