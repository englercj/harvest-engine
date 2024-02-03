// Copyright Chad Engler

#include "he/core/macros.h"

#include <intrin.h>

#define HE_INTRIN_CHOOSE_SIZE(T, r, ORDER, intrin, ...) \
    if constexpr (IsPointer<T>) \
        r = ORDER(intrin ## Pointer)(__VA_ARGS__); \
    else if constexpr (sizeof(T) == 1) \
        r = ORDER(intrin ## 8)(__VA_ARGS__); \
    else if constexpr (sizeof(T) == 2) \
        r = ORDER(intrin ## 16)(__VA_ARGS__); \
    else if constexpr (sizeof(T) == 4) \
        r = ORDER(intrin)(__VA_ARGS__); \
    else if constexpr (sizeof(T) == 8) \
        r = ORDER(intrin ## 64)(__VA_ARGS__)

#if HE_CPU_ARM
    #define HE_INTRIN_RELAXED(x) HE_PP_JOIN(x, _nf)
    #define HE_INTRIN_ACQUIRE(x) HE_PP_JOIN(x, _acq)
    #define HE_INTRIN_RELEASE(x) HE_PP_JOIN(x, _rel)
    #define HE_INTRIN_SEQCST(x) x

    #define HE_INTRIN_CHOOSE(T, r, order, intrin, ...) \
        switch (order) { \
            case MemoryOrder::Relaxed: \
                HE_INTRIN_CHOOSE_SIZE(T, r, HE_INTRIN_RELAXED, intrin, __VA_ARGS__); \
                break; \
            case MemoryOrder::Consume: \
            case MemoryOrder::Acquire: \
                HE_INTRIN_CHOOSE_SIZE(T, r, HE_INTRIN_ACQUIRE, intrin, __VA_ARGS__); \
                break; \
            case MemoryOrder::Release: \
                HE_INTRIN_CHOOSE_SIZE(T, r, HE_INTRIN_RELEASE, intrin, __VA_ARGS__); \
                break; \
            case MemoryOrder::AcqRel: \
            case MemoryOrder::SeqCst: \
                HE_INTRIN_CHOOSE_SIZE(T, r, HE_INTRIN_SEQCST, intrin, __VA_ARGS__); \
                break; \
        }

#else
    #define HE_INTRIN_RELAXED(x) x
    #define HE_INTRIN_ACQUIRE(x) x
    #define HE_INTRIN_RELEASE(x) x
    #define HE_INTRIN_SEQCST(x) x

    #define HE_INTRIN_CHOOSE(T, r, order, intrin, ...) \
        HE_INTRIN_CHOOSE_SIZE(T, r, HE_INTRIN_SEQCST, intrin, __VA_ARGS__)
#endif

namespace he
{
    // --------------------------------------------------------------------------------------------
    template <typename T>
    constexpr bool Atomic<T>::IsLockFree() noexcept
    {
        return sizeof(T) <= sizeof(void*);
    }

    template <typename T>
    inline T Atomic<T>::Load(MemoryOrder order) const noexcept
    {
        static_assert(order != MemoryOrder::Release && order != MemoryOrder::AcqRel);
        T result{ 0 };
        HE_INTRIN_CHOOSE(T, result, order, _InterlockedCompareExchange, &m_value, 0, 0);
        return result;
    }

    template <typename T>
    inline void Atomic<T>::Store(T desired, MemoryOrder order) noexcept
    {
        static_assert(order != MemoryOrder::Consume && order != MemoryOrder::Acquire && order != MemoryOrder::AcqRel);
        Exchange(desired, order);
    }

    template <typename T>
    inline T Atomic<T>::Exchange(T desired, MemoryOrder order) noexcept
    {
        T result{ desired };
        HE_INTRIN_CHOOSE(T, result, order, _InterlockedExchange, &m_value, desired);
        return result;
    }

    template <typename T>
    inline bool Atomic<T>::CompareExchangeWeak(T& expected, T desired, MemoryOrder successOrder, MemoryOrder failureOrder) noexcept
    {
        return CompareExchangeStrong(expected, desired, successOrder, failureOrder);
    }

    template <typename T>
    inline bool Atomic<T>::CompareExchangeStrong(T& expected, T desired, MemoryOrder successOrder, MemoryOrder failureOrder) noexcept
    {
        static_assert(failureOrder != MemoryOrder::Release && failureOrder != MemoryOrder::AcqRel, "Failure order must be weaker than release.");
        static_assert(failureOrder <= successOrder);
        T result = 0;
        HE_INTRIN_CHOOSE(T, result, successOrder, _InterlockedCompareExchange, &m_value, desired, expected);

        if (result == expected)
            return true;

        expected = result;
        return false;
    }

    template <typename T>
    inline T Atomic<T>::FetchAdd(DiffType amount, MemoryOrder order) noexcept requires(SupportsArithmetic)
    {
        static_assert(!IsSame<T, bool>, "Cannot perform atomic arithmetic on bool.")
        static_assert(order != MemoryOrder::Consume && order != MemoryOrder::Acquire && order != MemoryOrder::AcqRel);

        if constexpr (IsPointer<T>)
        {
            const ptrdiff_t shift = static_cast<ptrdiff_t>(static_cast<size_t>(amount) * sizeof(RemovePointer<T>));
            ptrdiff_t result{ 0 };
            HE_INTRIN_CHOOSE(intptr_t, result, order, _InterlockedExchangeAdd, &reinterpret_cast<intptr_t>(m_value), shift);
            return reinterpret_cast<T>(result);
        }
        else
        {
            T result{ 0 };
            HE_INTRIN_CHOOSE(T, result, order, _InterlockedExchangeAdd, &m_value, amount);
            return result;
        }
    }

    template <typename T>
    inline T Atomic<T>::FetchSub(DiffType amount, MemoryOrder order) noexcept requires(SupportsArithmetic)
    {
        if constexpr (IsPointer<T>)
        {
            const ptrdiff_t diff = static_cast<ptrdiff_t>(0 - static_cast<size_t>(amount));
            return FetchAdd(diff, order);
        }
        else
        {
            const T diff = static_cast<T>(0u - static_cast<unsigned T>(amount)); // two's compliment negation
            return FetchAdd(diff, order);
        }
    }

    template <typename T>
    inline T Atomic<T>::FetchAnd(T operand, MemoryOrder order) noexcept requires(SupportsBitwise)
    {
        static_assert(!IsSame<T, bool>, "Cannot perform atomic bitwise operations on bool.")
        static_assert(!IsPointer<T>, "Cannot perform atomic bitwise operations on pointers.");
        T result{ 0 };
        HE_INTRIN_CHOOSE(T, result, order, _InterlockedAnd, &m_value, operand);
        return result;
    }

    template <typename T>
    inline T Atomic<T>::FetchOr(T operand, MemoryOrder order) noexcept requires(SupportsBitwise)
    {
        static_assert(!IsSame<T, bool>, "Cannot perform atomic bitwise operations on bool.")
        static_assert(!IsPointer<T>, "Cannot perform atomic bitwise operations on pointers.");
        T result{ 0 };
        HE_INTRIN_CHOOSE(T, result, order, _InterlockedOr, &m_value, operand);
        return result;
    }

    template <typename T>
    inline T Atomic<T>::FetchXor(T operand, MemoryOrder order) noexcept requires(SupportsBitwise)
    {
        static_assert(!IsSame<T, bool>, "Cannot perform atomic bitwise operations on bool.")
        static_assert(!IsPointer<T>, "Cannot perform atomic bitwise operations on pointers.");
        T result{ 0 };
        HE_INTRIN_CHOOSE(T, result, order, _InterlockedXor, &m_value, operand);
        return result;
    }
}

#undef HE_INTRIN_RELAXED
#undef HE_INTRIN_ACQUIRE
#undef HE_INTRIN_RELEASE
#undef HE_INTRIN_SEQCST

#undef HE_INTRIN_CHOOSE
#undef HE_INTRIN_CHOOSE_SIZE
