// Copyright Chad Engler

#include "he/core/uuid.h"

#include "he/core/ascii.h"
#include "he/core/hash.h"
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

    template <typename Algo, uint8_t Version>
    static Uuid CreateUuidV35(StringView name, const Uuid& nsid)
    {
        static_assert(sizeof(Algo::ValueType::bytes) >= sizeof(Uuid::m_bytes));

        Hash<Algo> hash;
        hash.Update(nsid.m_bytes, HE_LENGTH_OF(nsid.m_bytes));
        hash.Update(name);

        typename Algo::ValueType value = hash.Final();

        // Per section 4.4, set bits for version and `clock_seq_hi_and_reserved`
        value.bytes[6] = (value.bytes[6] & 0x0f) | Version;
        value.bytes[8] = (value.bytes[8] & 0x3f) | 0x80;

        Uuid id;
        MemCopy(id.m_bytes, value.bytes, sizeof(id.m_bytes));
        return id;
    }

    Uuid Uuid::CreateV3(StringView name, const Uuid& nsid)
    {
        return CreateUuidV35<MD5, 0x30>(name, nsid);
    }

    Uuid Uuid::CreateV4()
    {
        Uuid id;
        GetSecureRandomBytes(id.m_bytes);

        // Per section 4.4, set bits for version and `clock_seq_hi_and_reserved`
        id.m_bytes[6] = (id.m_bytes[6] & 0x0f) | 0x40;
        id.m_bytes[8] = (id.m_bytes[8] & 0x3f) | 0x80;

        return id;
    }

    Uuid Uuid::CreateV5(StringView name, const Uuid& nsid)
    {
        return CreateUuidV35<SHA1, 0x50>(name, nsid);
    }

    uint64_t Uuid::HashCode() const noexcept
    {
        static_assert(sizeof(Uuid::m_bytes) > sizeof(uint64_t));
        uint64_t h;
        he::MemCopy(&h, m_bytes, sizeof(uint64_t));
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
