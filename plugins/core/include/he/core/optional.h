// Copyright Chad Engler

#pragma once

#include "he/core/concepts.h"
#include "he/core/hash.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"
#include "he/core/utils.h"

namespace he
{
    template <typename T>
    class Optional
    {
        static_assert(IsObject<T> && IsDestructible<T> && !IsArray<T>, "Optional can only contain non-array, destructible, objects.");

        /// \internal
        template <typename U>
        static constexpr bool _AllowUnwrappingAssignment =
            !IsSame<T, U>
            && !IsAssignable<T&, Optional<U>&>
            && !IsAssignable<T&, const Optional<U>&>
            && !IsAssignable<T&, const Optional<U>>
            && !IsAssignable<T&, Optional<U>>;

        /// \internal
        template <typename U>
        static constexpr bool _AllowDirectConversion =
            !IsSame<RemoveCVRef<U>, Optional>
            && !(IsSame<RemoveCV<T>, bool>&& IsSpecialization<RemoveCV<U>, Optional>)
            && IsConstructible<T, U>;

        /// \internal
        template <typename U>
        static constexpr bool _AllowUnwrapping =
            IsSame<RemoveCV<T>, bool>
            || (!IsSame<T, U>
                && !IsConstructible<T, Optional<U>&>
                && !IsConstructible<T, const Optional<U>&>
                && !IsConstructible<T, const Optional<U>>
                && !IsConstructible<T, Optional<U>>
                && !IsConvertible<Optional<U>&, T>
                && !IsConvertible<const Optional<U>&, T>
                && !IsConvertible<const Optional<U>, T>
                && !IsConvertible<Optional<U>, T>);

    public:
        constexpr Optional() noexcept
            : m_dummy()
            , m_hasValue(false)
        {}

        template <typename... Args> requires(IsConstructible<T, Args...>)
        constexpr explicit Optional(Args&&... args) noexcept
            : m_value(Forward<Args>(args)...)
            , m_hasValue(true)
        {}

        template <typename U = T> requires(_AllowDirectConversion<U>)
        constexpr explicit(!IsConvertible<U, T>) Optional(U&& value) noexcept
            : m_value(Forward<U>(value))
            , m_hasValue(true)
        {}

        constexpr Optional(const Optional& x) noexcept requires(IsTriviallyCopyConstructible<T>) = default;
        constexpr Optional(Optional&& x) noexcept requires(IsTriviallyMoveConstructible<T>) = default;

        constexpr Optional(const Optional& x) noexcept requires(!IsTriviallyCopyConstructible<T> && IsCopyConstructible<T>)
            : m_dummy()
            , m_hasValue(false)
        {
            if (x)
            {
                ConstructValue(*x);
            }
        }

        constexpr Optional(Optional&& x) noexcept requires(!IsTriviallyMoveConstructible<T> && IsMoveConstructible<T>)
            : m_dummy()
            , m_hasValue(false)
        {
            if (x)
            {
                ConstructValue(Move(*x));
            }
        }

        template <typename U> requires(_AllowUnwrapping<U> && IsConstructible<T, const U&>)
        constexpr explicit(!IsConvertible<const U&, T>) Optional(const Optional<U>& x) noexcept
            : m_dummy()
            , m_hasValue(false)
        {
            if (x)
            {
                ConstructValue(*x);
            }
        }

        template <typename U> requires(_AllowUnwrapping<U> && IsConstructible<T, U>)
        constexpr explicit(!IsConvertible<U, T>) Optional(Optional<U>&& x) noexcept
            : m_dummy()
            , m_hasValue(false)
        {
            if (x)
            {
                ConstructValue(Move(*x));
            }
        }

        constexpr ~Optional() noexcept
        {
            if constexpr (IsTriviallyDestructible<T>)
            {
                if (m_hasValue)
                {
                    m_value.~T();
                }
            }
        }

        constexpr void Reset() noexcept
        {
            if constexpr (IsTriviallyDestructible<T>)
            {
                m_hasValue = false;
            }
            else
            {
                if (m_hasValue)
                {
                    m_value.~T();
                    m_hasValue = false;
                }
            }
        }

        template <typename... Args>
        constexpr T& Emplace(Args&&... args) noexcept
        {
            Reset();
            return ConstructValue(Forward<Args>(args)...);
        }

        template <typename U>
        constexpr void Assign(U&& value) noexcept
        {
            if (m_hasValue)
            {
                m_value = Forward<U>(value);
            }
            else
            {
                ConstructValue(Forward<U>(value));
            }
        }

        [[nodiscard]] constexpr bool HasValue() const noexcept { return m_hasValue; }

        [[nodiscard]] constexpr const T& Value() const& noexcept
        {
            // HE_ASSERT(m_hasValue);
            return m_value;
        }

        [[nodiscard]] constexpr T& Value() & noexcept
        {
            // HE_ASSERT(m_hasValue);
            return m_value;
        }

        [[nodiscard]] constexpr const T&& Value() const&& noexcept
        {
            // HE_ASSERT(m_hasValue);
            return Move(m_value);
        }

        [[nodiscard]] constexpr T&& Value() && noexcept
        {
            // HE_ASSERT(m_hasValue);
            return Move(m_value);
        }

        template <typename U>
        [[nodiscard]] constexpr RemoveCV<T> ValueOr(U&& fallback) const&
        {
            static_assert(IsConvertible<const T&, RemoveCV<T>>);
            static_assert(IsConvertible<U, T>);
            return m_hasValue ? static_cast<const T&>(m_value) : static_cast<RemoveCV<T>>(Forward<U>(fallback));
        }

        template <typename U>
        [[nodiscard]] constexpr RemoveCV<T> ValueOr(U&& fallback) &&
        {
            static_assert(IsConvertible<T, RemoveCV<T>>);
            static_assert(IsConvertible<U, T>);
            return m_hasValue ? static_cast<T&&>(m_value) : static_cast<RemoveCV<T>>(Forward<U>(fallback));
        }

        [[nodiscard]] uint64_t HashCode() const noexcept
        {
            return m_hasValue ? GetHashCode(m_value) : 0;
        }

    public:
        template <typename U> requires(
            !IsSame<RemoveCVRef<U>, Optional>
            && IsConstructible<T, U>
            && IsAssignable<T&, U>)
        constexpr Optional& operator=(U&& x) noexcept
        {
            Assign(Forward<U>(x));
            return *this;
        }

        template <typename U> requires(
            _AllowUnwrappingAssignment<U>
            && IsConstructible<T, const U&>
            && IsAssignable<T&, const U&>)
        constexpr Optional& operator=(const Optional<U>& x) noexcept
        {
            if (x)
            {
                Assign(*x);
            }
            else
            {
                Reset();
            }
            return *this;
        }

        template <typename U> requires(
            _AllowUnwrappingAssignment<U>
            && IsConstructible<T, U>
            && IsAssignable<T&, U>)
        constexpr Optional& operator=(Optional<U>&& x) noexcept
        {
            if (x)
            {
                Assign(Move(*x));
            }
            else
            {
                Reset();
            }
            return *this;
        }

        [[nodiscard]] constexpr const T& operator*() const& noexcept { return m_value; }
        [[nodiscard]] constexpr T& operator*() & noexcept { return m_value; }

        [[nodiscard]] constexpr const T&& operator*() const&& noexcept { return Move(m_value); }
        [[nodiscard]] constexpr T&& operator*() && noexcept { return Move(m_value); }

        [[nodiscard]] constexpr const T* operator->() const noexcept { return &m_value; }
        [[nodiscard]] constexpr T* operator->() noexcept { return &m_value; }

        template <EqualityComparableWith<T> U>
        [[nodiscard]] constexpr bool operator==(const Optional<U>& x) const noexcept
        {
            if (m_hasValue && x.HasValue())
            {
                return m_value == x.Value();
            }
            return m_hasValue == x.HasValue();
        }

        [[nodiscard]] constexpr explicit operator bool() const noexcept { return m_hasValue; }

    private:
        template <typename... Args>
        constexpr T& ConstructValue(Args&&... args) noexcept
        {
            // HE_ASSERT(!m_hasValue);
            ::new(&m_value) T(Forward<Args>(args)...);
            m_hasValue = true;
            return m_value;
        }

    private:
        struct _Dummy { constexpr _Dummy() noexcept {} };
        static_assert(!IsTriviallyDefaultConstructible<_Dummy>);

        union
        {
            _Dummy m_dummy;
            RemoveCV<T> m_value;
        };
        bool m_hasValue{ false };
    };
}
