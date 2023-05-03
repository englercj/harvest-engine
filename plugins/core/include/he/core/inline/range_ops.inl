// Copyright Chad Engler

namespace he
{
    template <typename T, CopyAssignableTo<T> U>
    constexpr void RangeCopy(T* dst, const U* src, uint32_t count)
    {
        if constexpr (!IsVolatile<T>
            && !IsVolatile<U>
            && sizeof(T) == sizeof(U)
            && IsTriviallyCopyable<T>
            && IsTriviallyCopyable<U>)
        {
            if (!IsConstantEvaluated())
            {
                MemMove(dst, src, count * sizeof(T));
                return;
            }
        }

        for (uint32_t i = 0; i < count; ++i)
        {
            dst[i] = src[i];
        }
    }

    template <typename T, MoveAssignableTo<T> U>
    constexpr void RangeMove(T* dst, U* src, uint32_t count)
    {
        if constexpr (!IsVolatile<T>
            && !IsVolatile<U>
            && sizeof(T) == sizeof(U)
            && IsTriviallyCopyable<T>
            && IsTriviallyCopyable<U>)
        {
            if (!IsConstantEvaluated())
            {
                MemMove(dst, src, count * sizeof(T));
                return;
            }
        }

        for (uint32_t i = 0; i < count; ++i)
        {
            dst[i] = Move(src[i]);
        }
    }

    template <typename T, ConvertibleTo<T> V>
    constexpr void RangeFill(T* dst, uint32_t count, const V& value)
    {
        // Range of char-sized types can be memset
        if constexpr (!IsVolatile<T>
            && !IsVolatile<V>
            && sizeof(T) == sizeof(V)
            && IsAnyOf<UnwrapEnum<T>, bool, char, signed char, unsigned char>
            && IsAnyOf<UnwrapEnum<V>, bool, char, signed char, unsigned char>)
        {
            if (!IsConstantEvaluated())
            {
                MemSet(dst, value, count * sizeof(T));
                return;
            }
        }

        // Range of scalar types can be memset to zero if `value` is zero
        if constexpr (!IsVolatile<T>
            && !IsVolatile<V>
            && IsScalar<T>
            && IsScalar<V>)
        {
            if (!IsConstantEvaluated())
            {
                constexpr V ZeroValue{};
                if (MemEqual(&value, &ZeroValue, sizeof(V)))
                {
                    MemZero(dst, count * sizeof(T));
                    return;
                }
            }
        }

        for (uint32_t i = 0; i < count; ++i)
        {
            dst[i] = value;
        }
    }

    template <typename T, EqualityComparableWith<T> V>
    constexpr T* RangeFind(T* begin, uint32_t count, const V& value)
    {
        if constexpr (!IsVolatile<T>
            && !IsVolatile<V>
            && sizeof(T) == sizeof(V)
            && IsAnyOf<UnwrapEnum<T>, bool, char, signed char, unsigned char>
            && IsAnyOf<UnwrapEnum<V>, bool, char, signed char, unsigned char>)
        {
            if (!IsConstantEvaluated())
            {
                return static_cast<T*>(const_cast<void*>(MemChr(begin, value, count * sizeof(T))));
            }
        }

        for (uint32_t i = 0; i < count; ++i)
        {
            if (begin[i] == value)
            {
                return begin + i;
            }
        }

        return nullptr;
    }

    template <typename T, typename F>
    constexpr T* RangeFindIf(T* begin, uint32_t count, F&& predicate)
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            if (predicate(begin[i]))
            {
                return begin + i;
            }
        }

        return nullptr;
    }

    template <typename T, EqualityComparableWith<T> U>
    constexpr bool RangeEqual(const T* a, const U* b, uint32_t count)
    {
        if constexpr (!IsVolatile<T>
            && !IsVolatile<U>
            && sizeof(T) == sizeof(U)
            && IsScalar<T>
            && IsScalar<U>)
        {
            if (!IsConstantEvaluated())
            {
                return MemEqual(a, b, count * sizeof(T));
            }
        }

        for (uint32_t i = 0; i < count; ++i)
        {
            if (a[i] != b[i])
            {
                return false;
            }
        }

        return true;
    }

    template <typename T, typename U, typename F>
    constexpr bool RangeEqual(const T* a, const U* b, uint32_t count, F&& predicate)
    {
        if constexpr (!IsVolatile<T>
            && !IsVolatile<U>
            && sizeof(T) == sizeof(U)
            && IsScalar<T>
            && IsScalar<U>
            && IsSame<Decay<F>, EqualTo<T>>)
        {
            if (!IsConstantEvaluated())
            {
                return MemEqual(a, b, count * sizeof(T));
            }
        }

        for (uint32_t i = 0; i < count; ++i)
        {
            if (!predicate(a[i], b[i]))
            {
                return false;
            }
        }

        return true;
    }
}
