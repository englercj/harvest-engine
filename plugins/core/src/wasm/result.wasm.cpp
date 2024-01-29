// Copyright Chad Engler

#include "he/core/result.h"

#include "he/core/fmt.h"
#include "he/core/string.h"
#include "he/core/string_ops.h"
#include "he/core/utils.h"

#if defined(HE_PLATFORM_WASM)

#include <errno.h>

namespace he
{
    Result Result::Success{ 0 };
    Result Result::InvalidParameter{ EINVAL };
    Result Result::NotSupported{ ENOTSUP };

    struct ErrorMsgStrTable
    {
        #define E(n, s) char str##n[sizeof(s)];
        #include "errno_str.inl"
        #undef E
    };

    static const ErrorMsgStrTable s_errorStrings =
    {
        #define E(n, s) s,
        #include "errno_str.inl"
        #undef E
    };

    static const uint16_t s_errorOffsets[] =
    {
        #define E(n, s) [n] = offsetof(ErrorMsgStrTable, str##n),
        #include "errno_str.inl"
        #undef E
    };

    Result Result::FromLastError()
    {
        return PosixResult(errno);
    }

    void Result::ToString(String& out) const
    {
        if (m_code < 0 || m_code >= HE_LENGTH_OF(s_errorOffsets))
        {
            FormatTo(out, "Unknown error: {}", m_code);
            return;
        }

        const char* errorStrings = reinterpret_cast<const char*>(&s_errorStrings);
        const char* message = errorStrings + s_errorOffsets[m_code];
        out = message;
    }
}

#endif
