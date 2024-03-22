// Copyright Chad Engler

#include "he/core/error.h"
#include "he/core/types.h"

#if defined(HE_PLATFORM_API_POSIX)

namespace he
{
    bool _PlatformErrorHandler([[maybe_unused]] const ErrorSource& source, [[maybe_unused]] const KeyValue* kvs, [[maybe_unused]] uint32_t count)
    {
        // TODO: print the error if possible
        return true;
    }
}

#endif
