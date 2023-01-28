// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/assert.h"
#include "he/core/invoke.h"
#include "he/core/tuple.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"
#include "he/core/utils.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Helpful type traits

    // Deduces the function type for a delegate to call
    template <typename R, typename... Args>
    constexpr auto _AsFunctionPtr(R(*)(Args...)) -> R(*)(Args...);

    template <typename R, typename T, typename... Args, typename Rest>
    constexpr auto _AsFunctionPtr(R(*)(T, Args...), Rest&&) -> R(*)(Args...);

    template <typename T, typename R, typename... Args, typename... Rest>
    constexpr auto _AsFunctionPtr(R(T::*)(Args...), Rest&&...) -> R(*)(Args...);

    template <typename T, typename R, typename... Args, typename... Rest>
    constexpr auto _AsFunctionPtr(R(T::*)(Args...) const, Rest&&...) -> R(*)(Args...);

    template <typename T, typename R, typename... Rest>
    constexpr auto _AsFunctionPtr(R T::*, Rest&&...) -> R(*)();

    template <typename... T>
    using AsFunctionPtr = decltype(_AsFunctionPtr(DeclVal<T>()...));

    template <typename... T, typename R, typename... Args>
    [[nodiscard]] constexpr decltype(auto) _IndexSequenceForFunction(R(*)(Args...)) { return IndexSequenceFor<T..., Args...>{}; }

    // --------------------------------------------------------------------------------------------
    // Delegate implementation

    template <typename>
    class Delegate;

    template <typename R, typename... Args>
    class Delegate<R(Args...)>
    {
    public:
        using ReturnType = R;
        using ArgumentTypes = TypeList<Args...>;
        using FunctionType = R(const void*, Args...);

    public:
        template <auto F>
        static Delegate Make() noexcept
        {
            Delegate d;
            d.Set<F>();
            return d;
        }

        template <auto F, typename T>
        static Delegate Make(T&& payload) noexcept
        {
            Delegate d;
            d.Set<F>(Forward<T>(payload));
            return d;
        }

        static Delegate Make(FunctionType* func, const void* payload = nullptr) noexcept
        {
            return Delegate(func, payload);
        }

    public:
        Delegate() = default;

        Delegate(FunctionType* func, const void* payload = nullptr) noexcept
        {
            Set(func, payload);
        }

        template <auto F>
        void Set() noexcept
        {
            m_payload = nullptr;

            if constexpr (IsInvocableR<R, decltype(F), Args...>)
            {
                m_func = [](const void*, Args... args) -> R
                {
                    return R(Invoke(F, Forward<Args>(args)...));
                };
            }
            else if constexpr (IsMemberPointer<decltype(F)>)
            {
                m_func = Wrap<F>(_IndexSequenceForFunction<TypeListElement<0, ArgumentTypes>>(AsFunctionPtr<decltype(F)>{}));
            }
            else
            {
                m_func = Wrap<F>(_IndexSequenceForFunction(AsFunctionPtr<decltype(F)>{}));
            }
        }

        template <auto F, typename T>
        void Set(T& payload) noexcept
        {
            m_payload = &payload;

            if constexpr (IsInvocableR<R, decltype(F), T&, Args...>)
            {
                m_func = [](const void* payload, Args... args) -> R
                {
                    T* t = static_cast<T*>(const_cast<ConstnessAs<void, T>*>(payload));
                    return R(Invoke(F, *t, Forward<Args>(args)...));
                };
            }
            else
            {
                m_func = Wrap<F>(payload, _IndexSequenceForFunction(AsFunctionPtr<decltype(F), T>{}));
            }
        }

        template <auto F, typename T>
        void Set(T* payload) noexcept
        {
            m_payload = payload;

            if constexpr (IsInvocableR<R, decltype(F), T*, Args...>)
            {
                m_func = [](const void* payload, Args... args) -> R
                {
                    T* t = static_cast<T*>(const_cast<ConstnessAs<void, T>*>(payload));
                    return R(Invoke(F, t, Forward<Args>(args)...));
                };
            }
            else
            {
                m_func = Wrap<F>(payload, _IndexSequenceForFunction(AsFunctionPtr<decltype(F), T>{}));
            }
        }

        void Set(FunctionType* func, const void* payload = nullptr) noexcept
        {
            m_payload = payload;
            m_func = func;
        }

        void Clear() noexcept
        {
            m_payload = nullptr;
            m_func = nullptr;
        }

        [[nodiscard]] const void* Payload() const noexcept { return m_payload; }

        R operator()(Args... args) const
        {
            HE_ASSERT(m_func != nullptr);
            return m_func(m_payload, Forward<Args>(args)...);
        }

        [[nodiscard]] explicit operator bool() const noexcept { return m_func != nullptr; }
        [[nodiscard]] bool operator==(const Delegate& x) const noexcept { return m_func == x.m_func && m_payload == x.m_payload; }
        [[nodiscard]] bool operator!=(const Delegate& x) const noexcept { return m_func != x.m_func || m_payload != x.m_payload; }

    private:
        template<auto F, uint32_t... Index>
        [[nodiscard]] static auto Wrap(IndexSequence<Index...>) noexcept
        {
            return [](const void*, Args... args) -> R
            {
                [[maybe_unused]] const auto arguments = ForwardAsTuple(Forward<Args>(args)...);
                return static_cast<R>(Invoke(F, Forward<TypeListElement<Index, ArgumentTypes>>(TupleGet<Index>(arguments))...));
            };
        }

        template<auto F, typename T, uint32_t... Index>
        [[nodiscard]] static auto Wrap(T&, IndexSequence<Index...>) noexcept
        {
            return [](const void* payload, Args... args) -> R
            {
                [[maybe_unused]] const auto arguments = ForwardAsTuple(Forward<Args>(args)...);
                T* t = static_cast<T*>(const_cast<ConstnessAs<void, T>*>(payload));
                return static_cast<R>(Invoke(F, *t, Forward<TypeListElement<Index, ArgumentTypes>>(TupleGet<Index>(arguments))...));
            };
        }

        template<auto F, typename T, uint32_t... Index>
        [[nodiscard]] static auto Wrap(T*, IndexSequence<Index...>) noexcept
        {
            return [](const void* payload, Args... args) -> R
            {
                [[maybe_unused]] const auto arguments = ForwardAsTuple(Forward<Args>(args)...);
                T* t = static_cast<T*>(const_cast<ConstnessAs<void, T>*>(payload));
                return static_cast<R>(Invoke(F, t, Forward<TypeListElement<Index, ArgumentTypes>>(TupleGet<Index>(arguments))...));
            };
        }

    private:
        const void* m_payload{ nullptr };
        FunctionType* m_func{ nullptr };
    };

    // deduction guide
    template<typename R, typename... Args>
    Delegate(R(*)(const void*, Args...), const void* = nullptr) -> Delegate<R(Args...)>;

    // deduction guide
    template<typename R, typename... Args>
    Delegate(R(*)(Args...)) -> Delegate<R(Args...)>;
}
