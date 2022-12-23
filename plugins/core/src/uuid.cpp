// Copyright Chad Engler

#include "he/core/uuid.h"

#include "he/core/ascii.h"
#include "he/core/memory_ops.h"
#include "he/core/random.h"

namespace he
{
    HE_PUSH_WARNINGS()
        HE_DISABLE_MSVC_WARNING(4701) // potentially uninitialized local variable 'id' used

        Uuid Uuid::FromString(StringView src)
    {
        Uuid id;

        constexpr uint32_t ByteLen = HE_LENGTH_OF(id.m_bytes);

        uint32_t byteIndex = 0;
        char first = 0;

        for (const char c : src)
        {
            if (c == '-')
                continue;

            if (!IsHex(c) || byteIndex == ByteLen)
                break;

            if (first == 0)
            {
                first = c;
            }
            else
            {
                uint8_t byte = HexPairToByte(first, c);
                id.m_bytes[byteIndex++] = byte;
                first = 0;
            }
        }

        if (byteIndex != ByteLen)
            MemZero(id.m_bytes, ByteLen);

        return id;
    }

    HE_POP_WARNINGS()

        Uuid Uuid::CreateV4()
    {
        Uuid id;
        GetSecureRandomBytes(id.m_bytes);

        // Per section 4.4, set bits for version and `clock_seq_hi_and_reserved`
        id.m_bytes[6] = (id.m_bytes[6] & 0x0f) | 0x40;
        id.m_bytes[8] = (id.m_bytes[8] & 0x3f) | 0x80;

        return id;
    }

    uint64_t Uuid::HashCode() const noexcept
    {
        static_assert(sizeof(Uuid::m_bytes) > sizeof(uint64_t));
        uint64_t h;
        he::MemCopy(&h, value.m_bytes, sizeof(uint64_t));
        return h;
    }


    bool Uuid::operator==(const Uuid& x) const
    {
        return MemEqual(this, &x, sizeof(x));
    }

    bool Uuid::operator!=(const Uuid& x) const
    {
        return !MemEqual(this, &x, sizeof(x));
    }

    bool Uuid::operator<(const Uuid& x) const
    {
        return MemLess(this, &x, sizeof(x));
    }
}
