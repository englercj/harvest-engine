// Copyright Chad Engler

#include "he/core/assert.h"
#include "he/core/memory_ops.h"
#include "he/core/types.h"
#include "he/core/type_traits.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Bitset

    template <uint32_t N>
    template <uint32_t X>
    constexpr BitSet<N>::BitSet(const BitSet<X>& x)
    {
        constexpr uint32_t Count = Min(DataCount, BitSet<X>::DataCount);

        if (IsConstantEvaluated())
        {
            for (uint32_t i = 0; i < Count; ++i)
            {
                m_data[i] = x.m_data[i];
            }
        }
        else
        {
            MemCopy(m_data, x.m_data, Count * sizeof(ElementType));
        }
    }

    template <uint32_t N>
    template <uint32_t X>
    constexpr BitSet<N>& BitSet<N>::operator=(const BitSet<X>& x)
    {
        constexpr uint32_t Count = Min(DataCount, BitSet<X>::DataCount);

        if (this == &x)
            return *this;

        if (IsConstantEvaluated())
        {
            for (uint32_t i = 0; i < Count; ++i)
            {
                m_data[i] = x.m_data[i];
            }
            for (uint32_t i = Count; i < DataCount; ++i)
            {
                m_data[i] = 0;
            }
        }
        else
        {
            MemCopy(m_data, x.m_data, Count * sizeof(ElementType));
            if constexpr (Count < DataCount)
            {
                MemZero(m_data + Count, (DataCount - Count) * sizeof(ElementType));
            }
        }

        return *this;
    }

    template <uint32_t N>
    constexpr BitReference<BitSet<N>> BitSet<N>::operator[](uint32_t index)
    {
        HE_ASSERT(index < BitCount);
        return BitReference<BitSet>(*this, index);
    }

    template <uint32_t N>
    constexpr bool BitSet<N>::operator==(const BitSet<N>& x) const
    {
        if (IsConstantEvaluated())
        {
            for (uint32_t i = 0; i < DataCount; ++i)
            {
                if (m_data[i] != x.m_data[i])
                    return false;
            }
            return true;
        }
        else
        {
            return MemEqual(m_data, x.m_data, DataCount * sizeof(ElementType));
        }
    }

    template <uint32_t N>
    constexpr bool BitSet<N>::operator!=(const BitSet<N>& x) const
    {
        if (IsConstantEvaluated())
        {
            for (uint32_t i = 0; i < DataCount; ++i)
            {
                if (m_data[i] != x.m_data[i])
                    return true;
            }
            return false;
        }
        else
        {
            return !MemEqual(m_data, x.m_data, DataCount * sizeof(ElementType));
        }
    }

    template <uint32_t N>
    constexpr bool BitSet<N>::IsSet(uint32_t index) const
    {
        HE_ASSERT(index < BitCount);
        const uint32_t dataIndex = index / BitsPerElement;
        const uint32_t shift = index % BitsPerElement;
        return (m_data[dataIndex] & (1 << shift)) != 0;
    }

    template <uint32_t N>
    constexpr void BitSet<N>::Set(uint32_t index, bool value)
    {
        HE_ASSERT(index < BitCount);
        const uint32_t dataIndex = index / BitsPerElement;
        const uint32_t shift = index % BitsPerElement;
        m_data[dataIndex] |= (m_data[dataIndex] & ~(1 << shift)) | (static_cast<uint8_t>(value) << shift);
    }

    template <uint32_t N>
    constexpr void BitSet<N>::Set(uint32_t index)
    {
        HE_ASSERT(index < BitCount);
        const uint32_t dataIndex = index / BitsPerElement;
        const uint32_t shift = index % BitsPerElement;
        m_data[dataIndex] |= (1 << shift);
    }

    template <uint32_t N>
    constexpr void BitSet<N>::Unset(uint32_t index)
    {
        HE_ASSERT(index < BitCount);
        const uint32_t dataIndex = index / BitsPerElement;
        const uint32_t shift = index % BitsPerElement;
        m_data[dataIndex] &= ~(1 << shift);
    }

    template <uint32_t N>
    constexpr void BitSet<N>::Flip(uint32_t index)
    {
        HE_ASSERT(index < BitCount);
        const uint32_t dataIndex = index / BitsPerElement;
        const uint32_t shift = index % BitsPerElement;
        m_data[dataIndex] ^= 1 << shift;
    }

    template <uint32_t N>
    constexpr void BitSet<N>::Clear()
    {
        if (IsConstantEvaluated())
        {
            for (uint32_t i = 0; i < DataCount; ++i)
            {
                m_data[i] = 0;
            }
        }
        else
        {
            MemSet(m_data, 0, DataCount * sizeof(ElementType));
        }
    }

    // --------------------------------------------------------------------------------------------
    // Bitspan

    constexpr BitReference<BitSpan> BitSpan::operator[](uint32_t index)
    {
        HE_ASSERT(m_data != nullptr);
        HE_ASSERT(index < Size());
        return BitReference<BitSpan>(*this, index);
    }

    constexpr bool BitSpan::operator==(const BitSpan& x) const
    {
        if (m_size != x.m_size)
            return false;

        if (IsConstantEvaluated())
        {
            for (uint32_t i = 0; i < m_size; ++i)
            {
                if (m_data[i] != x.m_data[i])
                    return false;
            }
            return true;
        }
        else
        {
            return MemEqual(m_data, x.m_data, m_size * sizeof(ElementType));
        }
    }

    constexpr bool BitSpan::operator!=(const BitSpan& x) const
    {
        if (m_size != x.m_size)
            return true;

        if (IsConstantEvaluated())
        {
            for (uint32_t i = 0; i < m_size; ++i)
            {
                if (m_data[i] != x.m_data[i])
                    return true;
            }
            return false;
        }
        else
        {
            return !MemEqual(m_data, x.m_data, m_size * sizeof(ElementType));
        }
    }

    constexpr bool BitSpan::IsSet(uint32_t index) const
    {
        HE_ASSERT(m_data != nullptr);
        HE_ASSERT(index < Size());
        const uint32_t dataIndex = index / BitsPerElement;
        const uint32_t shift = index % BitsPerElement;
        return (m_data[dataIndex] & (1 << shift)) != 0;
    }

    constexpr void BitSpan::Set(uint32_t index, bool value)
    {
        HE_ASSERT(m_data != nullptr);
        HE_ASSERT(index < Size());
        const uint32_t dataIndex = index / BitsPerElement;
        const uint32_t shift = index % BitsPerElement;
        m_data[dataIndex] |= (m_data[dataIndex] & ~(1 << shift)) | (static_cast<uint8_t>(value) << shift);
    }

    constexpr void BitSpan::Set(uint32_t index)
    {
        HE_ASSERT(m_data != nullptr);
        HE_ASSERT(index < Size());
        const uint32_t dataIndex = index / BitsPerElement;
        const uint32_t shift = index % BitsPerElement;
        m_data[dataIndex] |= (1 << shift);
    }

    constexpr void BitSpan::Unset(uint32_t index)
    {
        HE_ASSERT(m_data != nullptr);
        HE_ASSERT(index < Size());
        const uint32_t dataIndex = index / BitsPerElement;
        const uint32_t shift = index % BitsPerElement;
        m_data[dataIndex] &= ~(1 << shift);
    }

    constexpr void BitSpan::Flip(uint32_t index)
    {
        HE_ASSERT(m_data != nullptr);
        HE_ASSERT(index < Size());
        const uint32_t dataIndex = index / BitsPerElement;
        const uint32_t shift = index % BitsPerElement;
        m_data[dataIndex] ^= 1 << shift;
    }

    constexpr void BitSpan::Clear()
    {
        if (!m_data)
            return;

        if (IsConstantEvaluated())
        {
            for (uint32_t i = 0; i < m_size; ++i)
            {
                m_data[i] = 0;
            }
        }
        else
        {
            MemSet(m_data, 0, m_size * sizeof(ElementType));
        }
    }
}
