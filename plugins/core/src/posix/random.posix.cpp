// Copyright Chad Engler

#include "he/core/random.h"

#if defined(HE_PLATFORM_API_POSIX)

#include <fcntl.h>
#include <unistd.h>

namespace he
{
    struct _SystemRandomState
    {
        _SystemRandomState() { handle = open("/dev/urandom", O_RDONLY, static_cast<mode_t>(0)); }
        ~_SystemRandomState() { close(handle); }
        int32_t handle;
    };

    bool GetSystemRandomBytes(uint8_t* dst, size_t count)
    {
        static _SystemRandomState state;
        if (state.handle == -1)
            return false;

        // Read chunks of no more than 32MB since that's all it will return anyway.
        // See: https://man7.org/linux/man-pages/man4/random.4.html
        // > Since Linux 3.16, a read(2) from /dev/urandom will return at most 32 MB.
        constexpr size_t ChunkSize = 32 * 1024 * 1024;

        while (count > ChunkSize)
        {
            if (static_cast<size_t>(read(state.handle, dst, ChunkSize)) != ChunkSize)
                return false;

            dst += ChunkSize;
            count -= ChunkSize;
        }

        if (count > 0)
            return static_cast<size_t>(read(state.handle, dst, count)) == count;

        return true;
    }
}

#endif
