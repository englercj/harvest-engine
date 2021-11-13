// Copyright Chad Engler

#include "he/core/uuid.h"

#include "he/core/ascii.h"
#include "he/core/memory_ops.h"

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

    uint64_t Uuid::GetLow() const
    {
        uint64_t v;
        MemCopy(&v, m_bytes, sizeof(v));
        return v;
    }

    uint64_t Uuid::GetHigh() const
    {
        uint64_t v;
        MemCopy(&v, m_bytes + sizeof(v), sizeof(v));
        return v;
    }

    String Uuid::ToString(Allocator& allocator) const
    {
        String out(allocator);
        out.Reserve(36);

        for (uint32_t i = 0; i < HE_LENGTH_OF(m_bytes); ++i)
        {
            if (i == 4 || i == 6 || i == 8 || i == 10)
                out.PushBack('-');

            out.PushBack(ToHex((m_bytes[i] & 0xF0) >> 4));
            out.PushBack(ToHex(m_bytes[i] & 0x0F));
        }

        return out;
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

namespace std
{
    size_t hash<he::Uuid>::operator()(const he::Uuid& value) const
    {
        size_t h;
        he::MemCopy(&h, value.m_bytes, sizeof(h));
        return h;
    }
}
