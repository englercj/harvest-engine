// Copyright Chad Engler

#include "he/core/result.h"

#include "he/core/fmt.h"
#include "he/core/string.h"
#include "he/core/string_ops.h"
#include "he/core/utils.h"

#if defined(HE_PLATFORM_API_POSIX)

#include <errno.h>
#include <string.h>

namespace he
{
    Result Result::Success{ 0 };
    Result Result::InvalidParameter{ EINVAL };
    Result Result::NotSupported{ ENOTSUP };

    Result Result::FromLastError()
    {
        return PosixResult(errno);
    }

    void Result::ToString(String& out) const
    {
        // Try to fit the message into the string's embedded buffer. In English glibc error
        // strings are almost all less than 30 characters which fits in the embedded buffer.
        // If we can't fit in the embedded buffer then we jump to 128 characters and grow from
        // there as needed. The jump in size is to make it very likely we only ever allocate once.
        const uint32_t offset = out.Size();
        uint32_t size = String::MaxEmbedCharacters;
        out.Resize(offset + size, DefaultInit);

        int e = 0;

        do
        {
        #if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && !defined(_GNU_SOURCE)
            // See "Return Value": https://linux.die.net/man/3/strerror_r
            #if defined(__GLIBC__) && (__GLIBC__ * 1000 + __GLIBC_MINOR__) < 2013
                #error "glibc in versions before 2.13 incorrectly returned error codes from POSIX strerror_r"
            #endif

            e = strerror_r(m_code, out.Data() + offset, size);
        #else
            errno = 0;
            char* msg = strerror_r(m_code, out.Data() + offset, size);
            e = errno;

            // GNU version is allowed to return a static string rather than copying to our buffer.
            if (msg != (out.Data() + offset))
            {
                out.Resize(offset);
                out.Append(msg);
            }
        #endif
            if (e == ERANGE)
            {
                // English glibc error strings cap out at like 50 characters, so we should never
                // have to allocate more than once.
                size = Max(128u, size * 2);
                out.Resize(offset + size, DefaultInit);
            }
        } while (e == ERANGE);

        if (e == EINVAL)
        {
            FormatTo(out, "Unknown error: {}", m_code);
        }
        else
        {
            // Resizing is necessary here because we used the string like a buffer so Size()
            // doesn't accurately represent the length of the string.
            const uint32_t len = StrLen(out.Data() + offset);
            out.Resize(offset + len);
        }
    }
}

#endif
