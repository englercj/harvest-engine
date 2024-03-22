// Copyright Chad Engler

#include "he/core/assert.h"
#include "he/core/cpu.h"
#include "he/core/macros.h"
#include "he/core/memory_ops.h"
#include "he/core/types.h"

#include <intrin.h>

template <size_t> struct _heAtomicTypeHelper;
template <> struct _heAtomicTypeHelper<1> { using Type = char; };
template <> struct _heAtomicTypeHelper<2> { using Type = short; };
template <> struct _heAtomicTypeHelper<4> { using Type = long; };
template <> struct _heAtomicTypeHelper<8> { using Type = long long; };

template <typename T>
using _heAtomicType = typename _heAtomicTypeHelper<sizeof(T)>::Type;

template <typename T, typename Int = _heAtomicType<T>>
[[nodiscard]] volatile Int* _heAtomicAddr(T& src) noexcept
{
    static_assert(he::IsIntegral<Int>);
    return &reinterpret_cast<volatile Int&>(src);
}

template <typename T, typename Int = _heAtomicType<T>>
[[nodiscard]] Int _heAtomicVal(const T& src) noexcept
{
    static_assert(he::IsIntegral<Int>);
    if constexpr (he::IsIntegral<T> && sizeof(Int) == sizeof(T))
    {
        return static_cast<Int>(src);
    }
    else if constexpr (he::IsPointer<T> && sizeof(Int) == sizeof(T))
    {
        return reinterpret_cast<Int>(src);
    }
    else
    {
        Int result{};
        he::MemCopy(&result, &src, sizeof(src));
        return result;
    }
}

#if HE_CPU_ARM
    #pragma intrinsic(__dmb)
    #define HE_COMPILER_OR_MEMORY_BARRIER() __dmb(0xB) // inner shared data memory barrier

    #define HE_INTERLOCK_APPLY(r, order, intrin, ...) \
        switch (order) { \
            case MemoryOrder::Relaxed: \
                r = HE_PP_JOIN(intrin, _nf)(__VA_ARGS__); \
                break; \
            case MemoryOrder::Consume: \
            case MemoryOrder::Acquire: \
                r = HE_PP_JOIN(intrin, _acq)(__VA_ARGS__); \
                break; \
            case MemoryOrder::Release: \
                r = HE_PP_JOIN(intrin, _rel)(__VA_ARGS__); \
                break; \
            case MemoryOrder::AcqRel: \
            case MemoryOrder::SeqCst: \
                r = intrin(__VA_ARGS__); \
                break; \
        }

    #define HE_ATOMIC_STORE_SEQCST(sz, ptr, desired) \
        HE_COMPILER_OR_MEMORY_BARRIER(); \
        HE_PP_JOIN(__iso_volatile_store, sz)((ptr), (desired)); \
        HE_COMPILER_OR_MEMORY_BARRIER()

#else
    #define HE_COMPILER_OR_MEMORY_BARRIER() _ReadWriteBarrier()

    #define HE_INTERLOCK_APPLY(r, order, intrin, ...) \
        r = intrin(__VA_ARGS__)

    #define HE_ATOMIC_STORE_SEQCST(sz, ptr, desired) \
        if constexpr (sz == 8) { (void)_InterlockedExchange8((ptr), (desired)); } \
        if constexpr (sz == 16) { (void)_InterlockedExchange16((ptr), (desired)); } \
        if constexpr (sz == 32) { (void)_InterlockedExchange(reinterpret_cast<volatile long*>(ptr), static_cast<long>(desired)); } \
        if constexpr (sz == 64) { (void)_InterlockedExchange64((ptr), (desired)); }

#endif

#define HE_INTERLOCK_CHOOSE(r, order, intrin, ...) \
    if constexpr (sizeof(T) == 1) { HE_INTERLOCK_APPLY(r, order, intrin ## 8, __VA_ARGS__); } \
    else if constexpr (sizeof(T) == 2) { HE_INTERLOCK_APPLY(r, order, intrin ## 16, __VA_ARGS__); } \
    else if constexpr (sizeof(T) == 4) { HE_INTERLOCK_APPLY(r, order, intrin, __VA_ARGS__); } \
    else if constexpr (sizeof(T) == 8) { HE_INTERLOCK_APPLY(r, order, intrin ## 64, __VA_ARGS__); }

#define HE_ATOMIC_STORE(sz, ptr, desired) \
    switch (order) { \
        case MemoryOrder::Relaxed: \
            HE_PP_JOIN(__iso_volatile_store, sz)((ptr), (desired)); \
            break; \
        case MemoryOrder::Release: \
            HE_COMPILER_OR_MEMORY_BARRIER(); \
            HE_PP_JOIN(__iso_volatile_store, sz)((ptr), (desired)); \
            break; \
        case MemoryOrder::Consume: \
        case MemoryOrder::Acquire: \
        case MemoryOrder::AcqRel: \
            /* Invalid memory orders */ \
            [[fallthrough]]; \
        case MemoryOrder::SeqCst: \
            HE_ATOMIC_STORE_SEQCST(sz, ptr, desired); \
            break; \
    }

namespace he
{
    // --------------------------------------------------------------------------------------------
    template <typename T>
    constexpr bool Atomic<T>::IsLockFree() noexcept
    {
        return sizeof(T) <= sizeof(void*);
    }

    template <typename T>
    inline T Atomic<T>::Load([[maybe_unused]] MemoryOrder order) const noexcept
    {
        HE_ASSERT(order != MemoryOrder::Release && order != MemoryOrder::AcqRel);

        _heAtomicType<T> result{ 0 };

        if constexpr (sizeof(T) == 1)
        {
            result = __iso_volatile_load8(_heAtomicAddr<const T, const __int8>(m_value));
        }
        else if constexpr (sizeof(T) == 2)
        {
            result = __iso_volatile_load16(_heAtomicAddr<const T, const __int16>(m_value));
        }
        else if constexpr (sizeof(T) == 4)
        {
            result = __iso_volatile_load32(_heAtomicAddr<const T, const __int32>(m_value));
        }
        else if constexpr (sizeof(T) == 8)
        {
            result = __iso_volatile_load64(_heAtomicAddr<const T, const __int64>(m_value));
        }

        switch (order)
        {
            case MemoryOrder::Consume:
            case MemoryOrder::Acquire:
            case MemoryOrder::SeqCst:
                HE_COMPILER_OR_MEMORY_BARRIER();
                break;
            case MemoryOrder::Relaxed:
                break;
            case MemoryOrder::Release:
            case MemoryOrder::AcqRel:
                // Invalid memory orders
                break;
        }
        return reinterpret_cast<T&>(result);
    }

    template <typename T>
    inline void Atomic<T>::Store(T desired, [[maybe_unused]] MemoryOrder order) noexcept
    {
        HE_ASSERT(order != MemoryOrder::Consume && order != MemoryOrder::Acquire && order != MemoryOrder::AcqRel);

        if constexpr (sizeof(T) == 1)
        {
            HE_ATOMIC_STORE(8, (_heAtomicAddr<T, __int8>(m_value)), (_heAtomicVal<T, __int8>(desired)));
        }
        else if constexpr (sizeof(T) == 2)
        {
            HE_ATOMIC_STORE(16, (_heAtomicAddr<T, __int16>(m_value)), (_heAtomicVal<T, __int16>(desired)));
        }
        else if constexpr (sizeof(T) == 4)
        {
            HE_ATOMIC_STORE(32, (_heAtomicAddr<T, __int32>(m_value)), (_heAtomicVal<T, __int32>(desired)));
        }
        else if constexpr (sizeof(T) == 8)
        {
            HE_ATOMIC_STORE(64, (_heAtomicAddr<T, __int64>(m_value)), (_heAtomicVal<T, __int64>(desired)));
        }
    }

    template <typename T>
    inline T Atomic<T>::Exchange(T desired, [[maybe_unused]] MemoryOrder order) noexcept
    {
        // MSVC is always SeqCst
        _heAtomicType<T> result{ _heAtomicVal(desired) };
        HE_INTERLOCK_CHOOSE(result, order, _InterlockedExchange, _heAtomicAddr(m_value), _heAtomicVal(desired));
        return reinterpret_cast<T&>(result);
    }

    template <typename T>
    inline bool Atomic<T>::CompareExchangeWeak(T& expected, T desired, [[maybe_unused]] MemoryOrder successOrder, [[maybe_unused]] MemoryOrder failureOrder) noexcept
    {
        HE_ASSERT(failureOrder != MemoryOrder::Release && failureOrder != MemoryOrder::AcqRel);
        HE_ASSERT(failureOrder <= successOrder);
        return CompareExchangeStrong(expected, desired, successOrder, failureOrder);
    }

    template <typename T>
    inline bool Atomic<T>::CompareExchangeStrong(T& expected, T desired, [[maybe_unused]] MemoryOrder successOrder, [[maybe_unused]] MemoryOrder failureOrder) noexcept
    {
        HE_ASSERT(failureOrder != MemoryOrder::Release && failureOrder != MemoryOrder::AcqRel);
        HE_ASSERT(failureOrder <= successOrder);
        _heAtomicType<T> result{ 0 };
        HE_INTERLOCK_CHOOSE(result, successOrder, _InterlockedCompareExchange, _heAtomicAddr(m_value), _heAtomicVal(desired), _heAtomicVal(expected));

        const T& newValue = reinterpret_cast<T&>(result);
        if (newValue == expected)
            return true;

        expected = newValue;
        return false;
    }

    template <typename T>
    inline T Atomic<T>::FetchAdd(DiffType amount, [[maybe_unused]] MemoryOrder order) noexcept requires(SupportsArithmetic)
    {
        HE_ASSERT(order != MemoryOrder::Consume && order != MemoryOrder::Acquire && order != MemoryOrder::AcqRel);

        if constexpr (IsPointer<T>)
        {
            const ptrdiff_t shift = static_cast<ptrdiff_t>(static_cast<size_t>(amount) * sizeof(RemovePointer<T>));
            ptrdiff_t result{ 0 };
            HE_INTERLOCK_CHOOSE(result, order, _InterlockedExchangeAdd, _heAtomicAddr(m_value), shift);
            return reinterpret_cast<T>(result);
        }
        else
        {
            _heAtomicType<T> result{ 0 };
            HE_INTERLOCK_CHOOSE(result, order, _InterlockedExchangeAdd, _heAtomicAddr(m_value), _heAtomicVal(amount));
            return result;
        }
    }

    template <typename T>
    inline T Atomic<T>::FetchSub(DiffType amount, [[maybe_unused]] MemoryOrder order) noexcept requires(SupportsArithmetic)
    {
        HE_ASSERT(order != MemoryOrder::Consume && order != MemoryOrder::Acquire && order != MemoryOrder::AcqRel);

        if constexpr (IsPointer<T>)
        {
            const ptrdiff_t diff = static_cast<ptrdiff_t>(0 - static_cast<size_t>(amount));
            return FetchAdd(diff, order);
        }
        else
        {
            const T diff = static_cast<T>(0u - static_cast<MakeUnsigned<T>>(amount)); // two's compliment negation
            return FetchAdd(diff, order);
        }
    }

    template <typename T>
    inline T Atomic<T>::FetchAnd(T operand, [[maybe_unused]] MemoryOrder order) noexcept requires(SupportsBitwise)
    {
        _heAtomicType<T> result{ 0 };
        HE_INTERLOCK_CHOOSE(result, order, _InterlockedAnd, _heAtomicAddr(m_value), _heAtomicVal(operand));
        return static_cast<T>(result);
    }

    template <typename T>
    inline T Atomic<T>::FetchOr(T operand, [[maybe_unused]] MemoryOrder order) noexcept requires(SupportsBitwise)
    {
        _heAtomicType<T> result{ 0 };
        HE_INTERLOCK_CHOOSE(result, order, _InterlockedOr, _heAtomicAddr(m_value), _heAtomicVal(operand));
        return static_cast<T>(result);
    }

    template <typename T>
    inline T Atomic<T>::FetchXor(T operand, [[maybe_unused]] MemoryOrder order) noexcept requires(SupportsBitwise)
    {
        _heAtomicType<T> result{ 0 };
        HE_INTERLOCK_CHOOSE(result, order, _InterlockedXor, _heAtomicAddr(m_value), _heAtomicVal(operand));
        return static_cast<T>(result);
    }
}

#undef HE_COMPILER_OR_MEMORY_BARRIER
#undef HE_INTERLOCK_APPLY
#undef HE_INTERLOCK_CHOOSE
#undef HE_ATOMIC_STORE_SEQCST
#undef HE_ATOMIC_STORE
