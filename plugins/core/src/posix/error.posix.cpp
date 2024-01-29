// Copyright Chad Engler

#include "he/core/error.h"
#include "he/core/types.h"

#if defined(HE_PLATFORM_API_POSIX)

namespace he
{
    bool _PlatformErrorHandler(const ErrorSource& source, const KeyValue* kvs, uint32_t count)
    {
        HE_UNUSED(source, kvs, count);
        return true;
    }
}

#endif
