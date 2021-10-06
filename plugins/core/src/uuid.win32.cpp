// Copyright Chad Engler

#include "he/core/uuid.h"

#if defined(HE_PLATFORM_API_WIN32)

// X is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
#pragma warning(push)
#pragma warning(disable : 4668)

#include <objbase.h>

#pragma warning(pop)

namespace he
{
    Uuid Uuid::CreateV4()
    {
        GUID guid;
        ::CoCreateGuid(&guid);

        return Uuid{ {
            static_cast<uint8_t>((guid.Data1 >> 24) & 0xFF),
            static_cast<uint8_t>((guid.Data1 >> 16) & 0xFF),
            static_cast<uint8_t>((guid.Data1 >> 8) & 0xFF),
            static_cast<uint8_t>((guid.Data1) & 0xFF),
            static_cast<uint8_t>((guid.Data2 >> 8) & 0xFF),
            static_cast<uint8_t>((guid.Data2) & 0xFF),
            static_cast<uint8_t>((guid.Data3 >> 8) & 0xFF),
            static_cast<uint8_t>((guid.Data3) & 0xFF),
            guid.Data4[0],
            guid.Data4[1],
            guid.Data4[2],
            guid.Data4[3],
            guid.Data4[4],
            guid.Data4[5],
            guid.Data4[6],
            guid.Data4[7],
        } };
    }
}

#endif
