// Copyright Chad Engler

namespace he
{
    // --------------------------------------------------------------------------------------------
    // FNV32

    constexpr uint32_t _FNV32Prime = 0x1000193ul;

    constexpr FNV32::ValueType FNV32::HashString(const char* str, ValueType h)
    {
        while (char ch = *str++)
            h = _FNV32Prime * (h ^ static_cast<uint8_t>(ch));
        return h;
    }

    constexpr FNV32::ValueType FNV32::HashStringN(const char* str, uint32_t len, ValueType h)
    {
        while (len-- > 0)
            h = _FNV32Prime * (h ^ static_cast<uint8_t>(*str++));
        return h;
    }

    inline FNV32::ValueType FNV32::HashData(const void* data, uint32_t len, ValueType h)
    {
        const uint8_t* s = static_cast<const uint8_t*>(data);
        while (len-- > 0)
            h = _FNV32Prime * (h ^ *s++);
        return h;
    }

    template <>
    inline FNV32::ValueType FNV32::HashScalar(const bool& b, ValueType h)
    {
        h = _FNV32Prime * (h ^ static_cast<uint8_t>(b));
        return h;
    }

    template <Arithmetic T>
    inline FNV32::ValueType FNV32::HashScalar(const T& obj, ValueType h)
    {
        const uint8_t* s = reinterpret_cast<const uint8_t*>(&obj);
        for (uint32_t i = 0; i < sizeof(T); ++i)
            h = _FNV32Prime * (h ^ *s++);
        return h;
    }

    inline FNV32::FNV32(ValueType seed)
    {
        Reset(seed);
    }

    template <Arithmetic T>
    inline FNV32& FNV32::Scalar(const T& obj)
    {
        m_state = HashScalar(obj, m_state);
        return *this;
    }

    inline FNV32& FNV32::String(const char* str)
    {
        m_state = HashString(str, m_state);
        return *this;
    }

    inline FNV32& FNV32::Data(const void* data, uint32_t len)
    {
        m_state = HashData(data, len, m_state);
        return *this;
    }

    inline FNV32& FNV32::Reset(ValueType seed)
    {
        m_state = seed;
        return *this;
    }

    inline FNV32::ValueType FNV32::Done()
    {
        return m_state;
    }

    // --------------------------------------------------------------------------------------------
    // FNV64

    constexpr uint64_t _FNV64Prime = 0x100000001b3ull;

    constexpr FNV64::ValueType FNV64::HashString(const char* s, ValueType h)
    {
        while (char ch = *s++)
            h = _FNV64Prime * (h ^ static_cast<uint8_t>(ch));
        return h;
    }

    constexpr FNV64::ValueType FNV64::HashStringN(const char* str, uint32_t len, ValueType h)
    {
        while (len-- > 0)
            h = _FNV64Prime * (h ^ static_cast<uint8_t>(*str++));
        return h;
    }

    inline FNV64::ValueType FNV64::HashData(const void* data, uint32_t len, ValueType h)
    {
        const uint8_t* s = static_cast<const uint8_t*>(data);
        while (len-- > 0)
            h = _FNV64Prime * (h ^ *s++);
        return h;
    }

    template <>
    inline FNV64::ValueType FNV64::HashScalar(const bool& b, ValueType h)
    {
        h = _FNV64Prime * (h ^ static_cast<uint8_t>(b));
        return h;
    }

    template <Arithmetic T>
    inline FNV64::ValueType FNV64::HashScalar(const T& obj, ValueType h)
    {
        const uint8_t* s = reinterpret_cast<const uint8_t*>(&obj);
        for (uint32_t i = 0; i < sizeof(T); ++i)
            h = _FNV64Prime * (h ^ *s++);
        return h;
    }

    inline FNV64::FNV64(ValueType seed)
    {
        Reset(seed);
    }

    template <Arithmetic T>
    inline FNV64& FNV64::Scalar(const T& obj)
    {
        m_state = HashScalar(obj, m_state);
        return *this;
    }

    inline FNV64& FNV64::String(const char* str)
    {
        m_state = HashString(str, m_state);
        return *this;
    }

    inline FNV64& FNV64::Data(const void* data, uint32_t len)
    {
        m_state = HashData(data, len, m_state);
        return *this;
    }

    inline FNV64& FNV64::Reset(ValueType seed)
    {
        m_state = seed;
        return *this;
    }

    inline FNV64::ValueType FNV64::Done()
    {
        return m_state;
    }
}
