// Copyright Chad Engler

#include "he/core/hash.h"

#include "wyhash.h"

#include "he/core/compiler.h"
#include "he/core/cpu.h"
#include "he/core/memory_ops.h"
#include "he/core/string.h"

namespace he
{
    WyHash::ValueType WyHash::HashString(const char* str, ValueType seed)
    {
        const uint32_t len = String::Length(str);
        return HashData(str, len, seed);
    }

    WyHash::ValueType WyHash::HashData(const void* data, uint32_t len, ValueType seed)
    {
        return wyhash::Hash(data, len, seed);
    }
}
