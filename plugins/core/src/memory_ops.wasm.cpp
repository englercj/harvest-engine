// Copyright Chad Engler

#include "he/core/memory_ops.h"

#include "he/core/types.h"

#if defined(HE_PLATFORM_WASM)

namespace he
{
    int32_t MemCmp(const void* a, const void* b, size_t len)
    {
        const uint8_t* a8 = static_cast<const uint8_t*>(a);
        const uint8_t* b8 = static_cast<const uint8_t*>(b);

        while (len)
        {
            if (*a8 != *b8)
                return *a8 - *b8;

            ++a8;
            ++b8;
            --len;
        }

        return 0;
    }

    const void* MemChr(const void* mem, int ch, size_t len)
    {
        const uint8_t* mem8 = static_cast<const uint8_t*>(mem);
        ch = static_cast<uint8_t>(ch);

        while (len)
        {
            if (*mem8 == ch)
                return mem8;

            ++mem8;
            --len;
        }

        return nullptr;
    }
}

#endif
