// Copyright Chad Engler

#include "he/core/result.h"

#include "he/core/string.h"

#include "fmt/format.h"

#if defined(HE_PLATFORM_API_POSIX)

#include <cerrno>
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

    String Result::ToString(Allocator& allocator) const;
    {
    #if (_POSIX_C_SOURCE < 200112L && _XOPEN_SOURCE < 600) || _GNU_SOURCE
        #error "strerror_r is provided as the GNU-specific variant."
    #endif

    // See "Return Value": https://linux.die.net/man/3/strerror_r
    #if defined(__GLIBC__) && (__GLIBC__ * 1000 + __GLIBC_MINOR__) < 2013
        #error "glibc in versions before 2.13 incorrectly returned error codes from strerror_r"
    #endif

        String ret(allocator);

        // Try to fit the message into the string's embedded buffer. In English glibc error
        // strings cap out at like 50 chracters, so they should all fit in the embedded buffer.
        // If we can't fit in the embedded buffer then we jump to 512 characters and grow from
        // there as needed. The jump in size is to make it very likely we only ever allocate once.
        ret.Resize(String::MaxEmbedCharacters);
        int e = 0;
        while ((e = strerror_r(m_code, ret.Data(), ret.Size())) == ERANGE)
        {
            ret.Resize(Max(512, ret.Size() * 2));
        }

        if (e == EINVAL)
        {
            fmt::memory_buffer buf;
            fmt::format_to(fmt::appender(buf), "Unknown error: {}", m_code);
            ret.Assign(buf.data(), static_cast<uint32_t>(buf.size()));
        }
        else
        {
            const uint32_t len = String::Length(ret.Data());
            ret.Resize(len);
        }

        return ret;
    }
}

#endif
