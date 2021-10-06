// Copyright Chad Engler

#include "he/core/uuid.h"

#if defined(HE_PLATFORM_LINUX)

#include <uuid/uuid.h>

namespace he
{
    Uuid Uuid::CreateV4()
    {
        uuid_t id;
        uuid_generate_random(id);

        return Uuid{ {
            id[0],
            id[1],
            id[2],
            id[3],
            id[4],
            id[5],
            id[6],
            id[7],
            id[8],
            id[9],
            id[10],
            id[11],
            id[12],
            id[13],
            id[14],
            id[15],
        } };
    }
}

#endif
