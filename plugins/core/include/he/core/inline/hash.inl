// Copyright Chad Engler

namespace he
{
    // --------------------------------------------------------------------------------------------
    // FNV32

    inline uint32_t FNV32::Mem(const void* data, uint32_t len, uint32_t h)
    {
        const uint8_t* s = static_cast<const uint8_t*>(data);
        while (len-- > 0)
            h = Prime * (h ^ *s++);
        return h;
    }

    constexpr uint32_t FNV32::String(const char* str, uint32_t h)
    {
        while (const char ch = *str++)
            h = Prime * (h ^ static_cast<uint8_t>(ch));
        return h;
    }

    constexpr uint32_t FNV32::String(StringView str, uint32_t h)
    {
        for (const char ch : str)
            h = Prime * (h ^ static_cast<uint8_t>(ch));
        return h;
    }

    // --------------------------------------------------------------------------------------------
    // FNV64

    inline uint64_t FNV64::Mem(const void* data, uint32_t len, uint64_t h)
    {
        const uint8_t* s = static_cast<const uint8_t*>(data);
        while (len-- > 0)
            h = Prime * (h ^ *s++);
        return h;
    }

    constexpr uint64_t FNV64::String(const char* s, uint64_t h)
    {
        while (const char ch = *s++)
            h = Prime * (h ^ static_cast<uint8_t>(ch));
        return h;
    }

    constexpr uint64_t FNV64::String(StringView str, uint64_t h)
    {
        for (const char ch : str)
            h = Prime * (h ^ static_cast<uint8_t>(ch));
        return h;
    }
}
