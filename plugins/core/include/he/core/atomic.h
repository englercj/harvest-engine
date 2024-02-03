// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/cpu.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"

#include <atomic>

namespace he
{
    // --------------------------------------------------------------------------------------------
    /// The memory order to use for atomic operations.
    enum class MemoryOrder : uint8_t
    {
        /// No ordering or synchronization constraints.
        Relaxed,

        /// Creates an inter-thread happens-before constraint from the release (or stronger)
        /// semantic store to this consume load.
        /// Can prevent hoisting of code to before the operation.
        Consume,

        /// Creates an inter-thread happens-before constraint from the release (or stronger)
        /// semantic store to this acquire load.
        /// Can prevent hoisting of code to before the operation.
        Acquire,

        /// Creates an inter-thread happens-before constraint to acquire (or stronger)
        /// semantic loads that read from this release store.
        /// Can prevent sinking of code to after the operation.
        Release,

        /// Combines the effects of acquire and release.
        AcqRel,

        /// Enforces total ordering with all other sequentially consistent (SeqCst) operations.
        SeqCst,
    };

    // --------------------------------------------------------------------------------------------
    /// A type that provides lock-free atomic operations on a value.
    ///
    /// \tparam T The type of the value to operate on.
    template <typename T>
    class Atomic
    {
        static_assert(!IsReference<T>, "Reference types are not supported by Atomic<T>.");
        static_assert(IsIntegral<T> || IsEnum<T>, "Only integral, enum, and pointer types are supported by Atomic<T>.");
        static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8, "Unsupported atomic type size.");

    public:
        // ----------------------------------------------------------------------------------------
        // Constants

        /// The underlying type of the atomic variable.
        using ElementType = T;

        /// The difference type of the atomic variable.
        using DiffType = Conditional<IsPointer<T>, ptrdiff_t, T>;

        /// True if this atomic type supports arithmetic operations.
        static constexpr bool SupportsArithmetic = !IsSame<T, bool>;

        /// True if this atomic type supports bitwise operations.
        static constexpr bool SupportsBitwise = IsIntegral<T> && !IsSame<T, bool>;

        /// True when the Atomic type is lock-free.
        static constexpr bool IsLockFree() noexcept;

        // ----------------------------------------------------------------------------------------
        // Construction

        /// Default constructs an atomic with a value-initialized value.
        constexpr Atomic() noexcept : m_value() {}

        /// Constructs an atomic with the given value.
        ///
        /// \param[in] value The value to initialize the atomic to.
        constexpr Atomic(T value) noexcept : m_value(value) {}

        /// Atomics are not copy constructible.
        Atomic(const Atomic&) = delete;

        // ----------------------------------------------------------------------------------------
        // Operators

        /// Atomics are not copy assignable.
        Atomic& operator=(const Atomic&) = delete;

        /// Atomically assigns `desired` to the current value. Equivalent to `Store(desired)`.
        ///
        /// \param[in] desired The value to store.
        /// \return The value after the operation.
        T operator=(T desired) noexcept { Store(desired); return desired; }

        /// Atomically pre-increments the current value by 1. Equivalent to `FetchAdd(1) + 1`.
        ///
        /// \return The value after the increment.
        T operator++() noexcept requires(SupportsArithmetic) { return FetchAdd(1) + 1; }

        /// Atomically post-increments the current value by 1. Equivalent to `FetchAdd(1)`.
        ///
        /// \return The value before the increment.
        T operator++(int) noexcept requires(SupportsArithmetic) { return FetchAdd(1); }

        /// Atomically pre-decrements the current value by 1. Equivalent to `FetchSub(1) - 1`.
        ///
        /// \return The value after the decrement.
        T operator--() noexcept requires(SupportsArithmetic) { return FetchSub(1) - 1; }

        /// Atomically post-decrements the current value by 1. Equivalent to `FetchSub(1)`.
        ///
        /// \return The value before the decrement.
        T operator--(int) noexcept requires(SupportsArithmetic) { return FetchSub(1); }

        /// Atomically adds `amount` to the current value. Equivalent to `FetchAdd(amount) + amount`.
        ///
        /// \param[in] amount The amount to add.
        /// \return The value after the addition.
        T operator+=(T amount) noexcept requires(SupportsArithmetic) { return FetchAdd(amount) + amount; }

        /// Atomically subtracts `amount` from the current value. Equivalent to `FetchSub(amount) - amount`.
        ///
        /// \param[in] amount The amount to subtract.
        /// \return The value after the subtraction.
        T operator-=(T amount) noexcept requires(SupportsArithmetic) { return FetchSub(amount) - amount; }

        /// Atomically bitwise ANDs `operand` with the current value. Equivalent to `FetchAnd(operand) & operand`.
        ///
        /// \param[in] operand The operand to AND with.
        /// \return The value after the operation.
        T operator&=(T operand) noexcept requires(SupportsBitwise) { return FetchAnd(operand) & operand; }

        /// Atomically bitwise ORs `operand` with the current value. Equivalent to `FetchOr(operand) | operand`.
        ///
        /// \param[in] operand The operand to OR with.
        /// \return The value after the operation.
        T operator|=(T operand) noexcept requires(SupportsBitwise) { return FetchOr(operand) | operand; }

        /// Atomically bitwise XORs `operand` with the current value. Equivalent to `FetchXor(operand) ^ operand`.
        ///
        /// \param[in] operand The operand to XOR with.
        /// \return The value after the operation.
        T operator^=(T operand) noexcept requires(SupportsBitwise) { return FetchXor(operand) ^ operand; }

        /// Atomically loads and returns the current value of the atomic variable. Equivalent to `Load()`.
        ///
        /// \return The current value of the atomic.
        [[nodiscard]] operator T() const noexcept { return Load(); }

        // ----------------------------------------------------------------------------------------
        // Data Access

        /// Atomically loads and returns the current value of the atomic variable. Memory is
        /// affected according to the value of `order`.
        ///
        /// \note If `order` is \ref MemoryOrder::Release or \ref MemoryOrder::AcqRel then the
        /// behavior is undefined.
        ///
        /// \param[in] order Optional. The memory order to use.
        /// \return The current value of the atomic.
        [[nodiscard]] T Load(MemoryOrder order = MemoryOrder::SeqCst) const noexcept;

        // ----------------------------------------------------------------------------------------
        // Mutators

        /// Atomically replaces the current value with `desired`. Memory is affected according to
        /// the value of `order`.
        ///
        /// \note If `order` is \ref MemoryOrder::Consume, \ref MemoryOrder::Acquire, or
        /// \ref MemoryOrder::AcqRel then the behavior is undefined.
        ///
        /// \param[in] desired The value to store.
        /// \param[in] order Optional. The memory order to use.
        void Store(T desired, MemoryOrder order = MemoryOrder::SeqCst) noexcept;

        /// Atomically replaces the current value with `desired` (a read-modify-write operation).
        /// Memory is affected according to the value of `order`.
        ///
        /// \param[in] desired The value to exchange.
        /// \param[in] order Optional. The memory order to use.
        /// \return The previous value before the exchange.
        T Exchange(T desired, MemoryOrder order = MemoryOrder::SeqCst) noexcept;

        /// Atomically compares the currency value with that of `expected`.
        /// If equal, replaces the current value with `desired` (read-modify-write operation).
        /// If not equal, loads the current value into `expected` (load operation).
        ///
        /// If `desired` replaces the current value then `true` is returned and memory is affected
        /// according to the memory order specified by `successOrder`. Otherwise, `false` is returned
        /// and memory is affected according to the memory order specified by `failureOrder`.
        ///
        /// \note If `failureOrder` is \ref MemoryOrder::Release or \ref MemoryOrder::AcqRel then the
        /// behavior is undefined.
        ///
        /// \note This operation is weak, which means it may fail spuriously. If you're unsure, use
        /// \ref CompareExchangeStrong.
        ///
        /// \param[in] expected The value to compare against.
        /// \param[in] desired The value to store.
        /// \param[in] successOrder Optional. The memory order to use if the exchange succeeds.
        /// \param[in] failureOrder Optional. The memory order to use if the exchange fails.
        /// \return The previous value before the exchange.
        bool CompareExchangeWeak(T& expected, T desired, MemoryOrder successOrder, MemoryOrder failureOrder) noexcept;

        /// \copybrief CompareExchangeWeak
        /// \copydetail CompareExchangeWeak
        ///
        /// \param[in] expected The value to compare against.
        /// \param[in] desired The value to store.
        /// \param[in] order Optional. The memory order to use.
        /// \return The previous value before the exchange.
        bool CompareExchangeWeak(T& expected, T desired, MemoryOrder order = MemoryOrder::SeqCst) noexcept { return CompareExchangeWeak(expected, desired, order, order); }

        /// Atomically compares the currency value with that of `expected`.
        /// If equal, replaces the current value with `desired` (read-modify-write operation).
        /// If not equal, loads the current value into `expected` (load operation).
        ///
        /// If `desired` replaces the current value then `true` is returned and memory is affected
        /// according to the memory order specified by `successOrder`. Otherwise, `false` is returned
        /// and memory is affected according to the memory order specified by `failureOrder`.
        ///
        /// \note If `failureOrder` is \ref MemoryOrder::Release or \ref MemoryOrder::AcqRel then the
        /// behavior is undefined.
        ///
        /// \note This operation is strong, which means it will never fail spuriously.
        ///
        /// \param[in] expected The value to compare against.
        /// \param[in] desired The value to store.
        /// \param[in] successOrder Optional. The memory order to use if the exchange succeeds.
        /// \param[in] failureOrder Optional. The memory order to use if the exchange fails.
        /// \return The previous value before the exchange.
        bool CompareExchangeStrong(T& expected, T desired, MemoryOrder successOrder, MemoryOrder failureOrder) noexcept;

        /// \copybrief CompareExchangeStrong
        /// \copydetail CompareExchangeStrong
        ///
        /// \param[in] expected The value to compare against.
        /// \param[in] desired The value to store.
        /// \param[in] order Optional. The memory order to use.
        /// \return The previous value before the exchange.
        bool CompareExchangeStrong(T& expected, T desired, MemoryOrder order = MemoryOrder::SeqCst) noexcept { return CompareExchangeStrong(expected, desired, order, order); }

        /// Atomically replaces the current value with the result of arithmetic addition of the
        /// value and `amount`. Memory is affected according to the value of `order`.
        ///
        /// \note If `order` is \ref MemoryOrder::Consume, \ref MemoryOrder::Acquire, or
        /// \ref MemoryOrder::AcqRel then the behavior is undefined.
        ///
        /// \param[in] amount The amount to add.
        /// \param[in] order Optional. The memory order to use.
        /// \return The value before the operation.
        T FetchAdd(DiffType amount, MemoryOrder order = MemoryOrder::SeqCst) noexcept requires(SupportsArithmetic);

        /// Atomically replaces the current value with the result of arithmetic subtraction of the
        /// value and `amount`. Memory is affected according to the value of `order`.
        ///
        /// \note If `order` is \ref MemoryOrder::Consume, \ref MemoryOrder::Acquire, or
        /// \ref MemoryOrder::AcqRel then the behavior is undefined.
        ///
        /// \param[in] amount The amount to subtract.
        /// \param[in] order Optional. The memory order to use.
        /// \return The value before the operation.
        T FetchSub(DiffType amount, MemoryOrder order = MemoryOrder::SeqCst) noexcept requires(SupportsArithmetic);

        /// Atomically replaces the current value with the result of bitwise AND of the
        /// value and `operand`. The operation is read-modify-write operation. Memory is affected
        /// according to the value of `order`.
        ///
        /// \param[in] operand The argument to the bitwise operation.
        /// \param[in] order Optional. The memory order to use.
        /// \return The value before the operation.
        T FetchAnd(T operand, MemoryOrder order = MemoryOrder::SeqCst) noexcept requires(SupportsBitwise);

        /// Atomically replaces the current value with the result of bitwise OR of the
        /// value and `operand`. The operation is read-modify-write operation. Memory is affected
        /// according to the value of `order`.
        ///
        /// \param[in] operand The argument to the bitwise operation.
        /// \param[in] order Optional. The memory order to use.
        /// \return The value before the operation.
        T FetchOr(T operand, MemoryOrder order = MemoryOrder::SeqCst) noexcept requires(SupportsBitwise);

        /// Atomically replaces the current value with the result of bitwise XOR of the
        /// value and `operand`. The operation is read-modify-write operation. Memory is affected
        /// according to the value of `order`.
        ///
        /// \param[in] operand The argument to the bitwise operation.
        /// \param[in] order Optional. The memory order to use.
        /// \return The value before the operation.
        T FetchXor(T operand, MemoryOrder order = MemoryOrder::SeqCst) noexcept requires(SupportsBitwise);

    private:
        T m_value;
    };
}

#if HE_COMPILER_MSVC
    #include "he/core/inline/atomic.msvc.inl"
#else
    // Clang supports GCC's intrinsics for atomics
    #include "he/core/inline/atomic.gcc.inl"
#endif
