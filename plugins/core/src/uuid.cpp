// Copyright Chad Engler

#include "he/core/uuid.h"

#include "he/core/ascii.h"
#include "he/core/hash.h"
#include "he/core/memory_ops.h"
#include "he/core/random.h"

namespace he
{
    Uuid::Uuid(const uint8_t bytes[Size]) noexcept
    {
        MemCopy(m_bytes, bytes, Size);
    }

    Uuid Uuid::FromString(StringView src)
    {
        Uuid id;

        uint32_t byteIndex = 0;
        char first = 0;

        for (const char c : src)
        {
            if (c == '-')
                continue;

            if (!IsHex(c) || byteIndex == Size)
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

        if (byteIndex != Size)
            MemZero(id.m_bytes, Size);

        return id;
    }

    template <typename Algo, uint8_t Version>
    static Uuid CreateUuidV35(StringView name, const Uuid& nsid)
    {
        static_assert(sizeof(Algo::ValueType::bytes) >= Uuid::Size);

        Hash<Algo> hash;
        hash.Update(nsid.m_bytes, HE_LENGTH_OF(nsid.m_bytes));
        hash.Update(name);

        typename Algo::ValueType value = hash.Final();

        // Per section 4.4, set bits for the version in `time_hi_and_version`
        value.bytes[6] = (value.bytes[6] & 0x0f) | Version;

        // Per section 4.4, set bits for the variant in `clock_seq_hi_and_reserved`
        value.bytes[8] = (value.bytes[8] & 0x3f) | 0x80;

        return Uuid(value.bytes);
    }

    Uuid Uuid::CreateV3(StringView name, const Uuid& nsid)
    {
        return CreateUuidV35<MD5, 0x30>(name, nsid);
    }

    Uuid Uuid::CreateV4()
    {
        Uuid id;

        // Try to use a secure random number generator for the UUID bytes. If that fails, we fall
        // back to using a pseudo random number generator. RFC 4122 doesn't require the randomness
        // to be cryptographically secure, but it is preferred.
        if (!GetSecureRandomBytes(id.m_bytes))
        {
            Random64 random;
            random.Bytes(id.m_bytes);
        }

        // Per section 4.4, set bits for the version in `time_hi_and_version`
        id.m_bytes[6] = (id.m_bytes[6] & 0x0f) | 0x40;

        // Per section 4.4, set bits for the variant in `clock_seq_hi_and_reserved`
        id.m_bytes[8] = (id.m_bytes[8] & 0x3f) | 0x80;

        return id;
    }

    Uuid Uuid::CreateV5(StringView name, const Uuid& nsid)
    {
        return CreateUuidV35<SHA1, 0x50>(name, nsid);
    }

    uint64_t Uuid::HashCode() const noexcept
    {
        static_assert(Size > sizeof(uint64_t));
        uint64_t h;
        he::MemCopy(&h, m_bytes, sizeof(uint64_t));
        return h;
    }

    bool Uuid::operator==(const Uuid& x) const
    {
        return MemEqual(m_bytes, x.m_bytes, Size);
    }

    bool Uuid::operator!=(const Uuid& x) const
    {
        return !MemEqual(m_bytes, x.m_bytes, Size);
    }

    bool Uuid::operator<(const Uuid& x) const
    {
        return MemLess(m_bytes, x.m_bytes, Size);
    }
}
