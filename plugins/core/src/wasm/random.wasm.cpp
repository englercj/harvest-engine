// Copyright Chad Engler

#include "he/core/random.h"

#include "he/core/limits.h"

#if defined(HE_PLATFORM_API_WASM)

#include "he/core/wasm/lib_core.wasm.h"

namespace he
{
    bool GetSystemRandomBytes(uint8_t* dst, size_t count)
    {
        // Shouldn't ever have a case where we're requesting this meany bytes at once from this
        // API, but just in case we break it up into chunks so our JS API doesn't have to handle
        // size_t values.
        constexpr uint32_t ChunkSize = static_cast<uint32_t>(Limits<int32_t>::Max);

        while (count > ChunkSize)
        {
            heWASM_GetRandomBytes(dst, ChunkSize);
            dst += ChunkSize;
            count -= ChunkSize;
        }

        if (count > 0)
            heWASM_GetRandomBytes(dst, static_cast<uint32_t>(count));

        return true;
    }
}

#endif
