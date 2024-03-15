// Copyright Chad Engler

#include "he/core/result.h"

#include "he/core/fmt.h"
#include "he/core/string.h"
#include "he/core/string_ops.h"
#include "he/core/utils.h"

#if defined(HE_PLATFORM_API_WASM)

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
        // Try to fit the message into the string's embedded buffer. The English error strings
        // are almost all less than 30 characters which fits in the embedded buffer.
        // If we can't fit in the embedded buffer then we jump to 128 characters and grow from
        // there as needed. The jump in size is to make it very likely we only ever allocate once.
        const uint32_t offset = out.Size();
        uint32_t size = String::MaxEmbedCharacters;
        out.Resize(offset + size, DefaultInit);

        int e = 0;

        do
        {
            e = strerror_r(m_code, out.Data() + offset, size);
            if (e == ERANGE)
            {
                // The English error strings cap out at like 50 characters, so we should never
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
