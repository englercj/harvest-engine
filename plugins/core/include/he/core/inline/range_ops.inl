// Copyright Chad Engler

#include "he/core/allocator.h"
#include "he/core/memory_ops.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"
#include "he/core/utils.h"

namespace he
{
    struct _RangePartitionResult
    {
        uint32_t lessEnd;
        uint32_t greaterBegin;
    };

    constexpr uint32_t _RangeSortInsertionThreshold = 24;

    template <typename T> requires(IsTriviallyDestructible<T>)
    constexpr void _RangeDestruct(T*, uint32_t)
    { }

    template <typename T> requires(!IsTriviallyDestructible<T>)
    constexpr void _RangeDestruct(T* begin, uint32_t count)
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            begin[i].~T();
        }
    }

    template <typename T>
    constexpr void _RangeConstruct(T* dst, T* src, uint32_t count)
    {
        if constexpr (IsTriviallyCopyable<T>)
        {
            if (!IsConstantEvaluated())
            {
                MemMove(dst, src, count * sizeof(T));
                return;
            }
        }

        for (uint32_t i = 0; i < count; ++i)
        {
            ::new(dst + i) T(Move(src[i]));
        }
    }

    template <typename T>
    constexpr void _RangeAssign(T* dst, T* src, uint32_t count)
    {
        if constexpr (IsTriviallyCopyable<T>)
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

    template <typename T>
    constexpr void _RangeSwap(T& a, T& b)
    {
        if (&a == &b)
            return;

        T tmp(Move(a));
        a = Move(b);
        b = Move(tmp);
    }

    template <typename T, typename F>
    constexpr void _RangeInsertionSort(T* begin, uint32_t count, F& predicate)
    {
        for (uint32_t i = 1; i < count; ++i)
        {
            T value(Move(begin[i]));
            uint32_t j = i;

            while (j > 0 && predicate(value, begin[j - 1]))
            {
                begin[j] = Move(begin[j - 1]);
                --j;
            }

            begin[j] = Move(value);
        }
    }

    template <typename T, typename F>
    constexpr T* _RangeMedianOfThree(T* a, T* b, T* c, F& predicate)
    {
        if (predicate(*b, *a))
        {
            _RangeSwap(*a, *b);
        }

        if (predicate(*c, *b))
        {
            _RangeSwap(*b, *c);
            if (predicate(*b, *a))
            {
                _RangeSwap(*a, *b);
            }
        }

        return b;
    }

    template <typename T, typename F>
    constexpr void _RangeSiftDown(T* begin, uint32_t root, uint32_t count, F& predicate)
    {
        while (true)
        {
            uint32_t child = (root * 2) + 1;
            if (child >= count)
                return;

            uint32_t swapIndex = root;

            if (predicate(begin[swapIndex], begin[child]))
            {
                swapIndex = child;
            }

            if ((child + 1) < count && predicate(begin[swapIndex], begin[child + 1]))
            {
                swapIndex = child + 1;
            }

            if (swapIndex == root)
                return;

            _RangeSwap(begin[root], begin[swapIndex]);
            root = swapIndex;
        }
    }

    template <typename T, typename F>
    constexpr void _RangeHeapSort(T* begin, uint32_t count, F& predicate)
    {
        if (count < 2)
            return;

        for (uint32_t i = count / 2; i > 0; --i)
        {
            _RangeSiftDown(begin, i - 1, count, predicate);
        }

        for (uint32_t end = count; end > 1; --end)
        {
            _RangeSwap(begin[0], begin[end - 1]);
            _RangeSiftDown(begin, 0, end - 1, predicate);
        }
    }

    template <typename T, typename F>
    constexpr _RangePartitionResult _RangePartition(T* begin, uint32_t count, F& predicate)
    {
        const uint32_t last = count - 1;
        T* const pivotElement = _RangeMedianOfThree(begin, begin + (count / 2), begin + last, predicate);
        _RangeSwap(*pivotElement, begin[last]);

        T pivot(Move(begin[last]));

        uint32_t less = 0;
        uint32_t index = 0;
        uint32_t greater = last;

        while (index < greater)
        {
            if (predicate(begin[index], pivot))
            {
                _RangeSwap(begin[less], begin[index]);
                ++less;
                ++index;
            }
            else if (predicate(pivot, begin[index]))
            {
                --greater;
                _RangeSwap(begin[index], begin[greater]);
            }
            else
            {
                ++index;
            }
        }

        if (greater != last)
        {
            begin[last] = Move(begin[greater]);
        }

        begin[greater] = Move(pivot);
        return { less, greater + 1 };
    }

    constexpr uint32_t _RangeSortDepthLimit(uint32_t count)
    {
        uint32_t result = 0;
        while (count > 1)
        {
            count >>= 1;
            ++result;
        }

        return result * 2;
    }

    template <typename T, typename F>
    constexpr void _RangeIntroSort(T* begin, uint32_t count, uint32_t depthLimit, F& predicate)
    {
        while (count > _RangeSortInsertionThreshold)
        {
            if (depthLimit == 0)
            {
                _RangeHeapSort(begin, count, predicate);
                return;
            }

            --depthLimit;
            const _RangePartitionResult partition = _RangePartition(begin, count, predicate);

            const uint32_t leftCount = partition.lessEnd;
            const uint32_t rightOffset = partition.greaterBegin;
            const uint32_t rightCount = count - rightOffset;

            if (leftCount < rightCount)
            {
                _RangeIntroSort(begin, leftCount, depthLimit, predicate);
                begin += rightOffset;
                count = rightCount;
            }
            else
            {
                _RangeIntroSort(begin + rightOffset, rightCount, depthLimit, predicate);
                count = leftCount;
            }
        }

        _RangeInsertionSort(begin, count, predicate);
    }

    template <typename T, typename F>
    constexpr void _RangeBufferedMerge(T* begin, uint32_t leftCount, uint32_t rightCount, T* buffer, F& predicate)
    {
        _RangeConstruct(buffer, begin, leftCount);

        uint32_t leftIndex = 0;
        uint32_t rightIndex = 0;
        uint32_t dstIndex = 0;
        T* const right = begin + leftCount;

        while (leftIndex < leftCount && rightIndex < rightCount)
        {
            if (predicate(right[rightIndex], buffer[leftIndex]))
            {
                begin[dstIndex++] = Move(right[rightIndex++]);
            }
            else
            {
                begin[dstIndex++] = Move(buffer[leftIndex++]);
            }
        }

        if (leftIndex < leftCount)
        {
            _RangeAssign(begin + dstIndex, buffer + leftIndex, leftCount - leftIndex);
        }

        _RangeDestruct(buffer, leftCount);
    }

    template <typename T, typename F>
    constexpr void _RangeInplaceMerge(T* begin, uint32_t leftCount, uint32_t count, F& predicate)
    {
        uint32_t leftIndex = 0;
        uint32_t rightIndex = leftCount;

        while (leftIndex < rightIndex && rightIndex < count)
        {
            if (!predicate(begin[rightIndex], begin[leftIndex]))
            {
                ++leftIndex;
                continue;
            }

            T value(Move(begin[rightIndex]));
            for (uint32_t i = rightIndex; i > leftIndex; --i)
            {
                begin[i] = Move(begin[i - 1]);
            }

            begin[leftIndex] = Move(value);
            ++leftIndex;
            ++rightIndex;
        }
    }

    template <typename T, typename F>
    constexpr void _RangeStableSortImpl(T* begin, uint32_t count, T* buffer, uint32_t bufferCapacity, F& predicate)
    {
        if (count <= _RangeSortInsertionThreshold)
        {
            _RangeInsertionSort(begin, count, predicate);
            return;
        }

        const uint32_t leftCount = count / 2;
        const uint32_t rightCount = count - leftCount;

        _RangeStableSortImpl(begin, leftCount, buffer, bufferCapacity, predicate);
        _RangeStableSortImpl(begin + leftCount, rightCount, buffer, bufferCapacity, predicate);

        if (!predicate(begin[leftCount], begin[leftCount - 1]))
            return;

        if (buffer != nullptr && leftCount <= bufferCapacity)
        {
            _RangeBufferedMerge(begin, leftCount, rightCount, buffer, predicate);
        }
        else
        {
            _RangeInplaceMerge(begin, leftCount, count, predicate);
        }
    }

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

    template <typename T> requires(IsConstructible<T, T&&> && IsAssignable<T&, T&&>)
    constexpr void RangeSort(T* begin, uint32_t count)
    {
        RangeSort(begin, count, LessThan<T>{});
    }

    template <typename T, typename F> requires(IsConstructible<T, T&&> && IsAssignable<T&, T&&>)
    constexpr void RangeSort(T* begin, uint32_t count, F&& predicate)
    {
        if (count < 2)
            return;

        _RangeIntroSort(begin, count, _RangeSortDepthLimit(count), predicate);
    }

    template <typename T> requires(IsConstructible<T, T&&> && IsAssignable<T&, T&&>)
    constexpr void RangeStableSort(T* begin, uint32_t count)
    {
        RangeStableSort(begin, count, LessThan<T>{});
    }

    template <typename T, typename F> requires(IsConstructible<T, T&&> && IsAssignable<T&, T&&>)
    constexpr void RangeStableSort(T* begin, uint32_t count, F&& predicate)
    {
        if (count < 2)
            return;

        if (IsConstantEvaluated())
        {
            _RangeInsertionSort(begin, count, predicate);
            return;
        }

        T* const buffer = Allocator::GetDefault().Malloc<T>(count / 2);
        _RangeStableSortImpl(begin, count, buffer, count / 2, predicate);
        Allocator::GetDefault().Free(buffer);
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
