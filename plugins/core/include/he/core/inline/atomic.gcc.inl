// Copyright Chad Engler

namespace he
{
    // Discover if any of the atomic types are not lock-free for our targets.

    HE_FORCE_INLINE constexpr int _ToGccOrder(MemoryOrder order)
    {
        switch (order)
        {
            case MemoryOrder::Relaxed: return __ATOMIC_RELAXED;
            case MemoryOrder::Consume: return __ATOMIC_CONSUME;
            case MemoryOrder::Acquire: return __ATOMIC_ACQUIRE;
            case MemoryOrder::Release: return __ATOMIC_RELEASE;
            case MemoryOrder::AcqRel: return __ATOMIC_ACQ_REL;
            case MemoryOrder::SeqCst: return __ATOMIC_SEQ_CST;
            default: return __ATOMIC_CONSUME;
        }
    }

    template <typename T>
    constexpr bool Atomic<T>::IsLockFree() noexcept
    {
        return __atomic_always_lock_free(sizeof(T), 0);
    }

    template <typename T>
    inline T Atomic<T>::Load(MemoryOrder order) const noexcept
    {
        HE_ASSERT(order != MemoryOrder::Release && order != MemoryOrder::AcqRel);
        return __atomic_load_n(const_cast<T*>(&m_value), _ToGccOrder(order));
    }

    template <typename T>
    inline void Atomic<T>::Store(T desired, MemoryOrder order) noexcept
    {
        HE_ASSERT(order != MemoryOrder::Consume && order != MemoryOrder::Acquire && order != MemoryOrder::AcqRel);
        __atomic_store_n(&m_value, desired, _ToGccOrder(order));
    }

    template <typename T>
    inline T Atomic<T>::Exchange(T desired, MemoryOrder order) noexcept
    {
        return __atomic_exchange_n(&m_value, desired, _ToGccOrder(order));
    }

    template <typename T>
    inline bool Atomic<T>::CompareExchangeWeak(T& expected, T desired, MemoryOrder successOrder, MemoryOrder failureOrder) noexcept
    {
        HE_ASSERT(failureOrder != MemoryOrder::Release && failureOrder != MemoryOrder::AcqRel);
        HE_ASSERT(failureOrder <= successOrder);
        return __atomic_compare_exchange_n(&m_value, &expected, desired, true, _ToGccOrder(successOrder), _ToGccOrder(failureOrder));
    }

    template <typename T>
    inline bool Atomic<T>::CompareExchangeStrong(T& expected, T desired, MemoryOrder successOrder, MemoryOrder failureOrder) noexcept
    {
        HE_ASSERT(failureOrder != MemoryOrder::Release && failureOrder != MemoryOrder::AcqRel);
        HE_ASSERT(failureOrder <= successOrder);
        return __atomic_compare_exchange_n(&m_value, &expected, desired, false, _ToGccOrder(successOrder), _ToGccOrder(failureOrder));
    }

    template <typename T>
    inline T Atomic<T>::FetchAdd(DiffType amount, MemoryOrder order) noexcept requires(SupportsArithmetic)
    {
        HE_ASSERT(order != MemoryOrder::Consume && order != MemoryOrder::Acquire && order != MemoryOrder::AcqRel);
        return __atomic_fetch_add(&m_value, amount, _ToGccOrder(order));
    }

    template <typename T>
    inline T Atomic<T>::FetchSub(DiffType amount, MemoryOrder order) noexcept requires(SupportsArithmetic)
    {
        HE_ASSERT(order != MemoryOrder::Consume && order != MemoryOrder::Acquire && order != MemoryOrder::AcqRel);
        return __atomic_fetch_sub(&m_value, amount, _ToGccOrder(order));
    }

    template <typename T>
    inline T Atomic<T>::FetchAnd(T operand, MemoryOrder order) noexcept requires(SupportsBitwise)
    {
        return __atomic_fetch_and(&m_value, operand, _ToGccOrder(order));
    }

    template <typename T>
    inline T Atomic<T>::FetchOr(T operand, MemoryOrder order) noexcept requires(SupportsBitwise)
    {
        return __atomic_fetch_or(&m_value, operand, _ToGccOrder(order));
    }

    template <typename T>
    inline T Atomic<T>::FetchXor(T operand, MemoryOrder order) noexcept requires(SupportsBitwise)
    {
        return __atomic_fetch_xor(&m_value, operand, _ToGccOrder(order));
    }
}
