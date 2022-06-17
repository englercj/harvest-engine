// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/assert.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"
#include "he/core/utils.h"

#include <type_traits>

namespace he
{
    /// \internal
    template <typename... T, typename R, typename... Args>
    [[nodiscard]] constexpr auto _IndexSequenceForFunction(R(*)(Args...))
    {
        return std::index_sequence_for<T..., Args...>{};
    }

    template <typename>
    class Delegate;

    template <typename R, typename... Args>
    class Delegate<R(Args...)>
    {
    public:
        using ReturnType = R;
        using InvocableType = R(Args...);
        using FunctionType = R(const void*, Args...);

    public:
        template <auto F>
        static Delegate Make() noexcept
        {
            Delegate d;
            d.Connect<F>();
            return d;
        }

        template <auto F, typename T>
        static Delegate Make(T&& payload) noexcept
        {
            Delegate d;
            d.Connect<F>(Forward<T>(payload));
            return d;
        }

        template <typename F> requires(std::is_invocable_r_v<R, F, Args...> && std::is_nothrow_assignable_v<F>)
        static Delegate Alloc(F&& invocable, Allocator& allocator = Allocator::GetDefault()) noexcept
        {
            struct Storage
            {
                F invocable;
                Allocator* allocator;
            };
            Storage* storage = allocator.New<Storage>();
            storage->invocable = Move(invocable);
            storage->allocator = allocator;

            Delegate d;
            d.Connect([](const void* payload, Args... args) -> R
            {
                Storage* store = static_cast<Storage*>(payload);
                R r = R(std::invoke(store->invocable, Forward<Args>(args)...));
                store->allocator.Delete(store);
                return r;
            }, storage);
            return d;
        }

    public:
        Delegate() noexcept = default;

        Delegate(FunctionType* func, const void* payload = nullptr) noexcept
            : m_payload(payload)
            , m_func(func)
        {}

        constexpr Delegate(InvocableType* func) noexcept
        {
            m_payload = func;
            m_func = [](const void* payload, Args... args) -> R
            {
                InvocableType* f = static_cast<InvocableType*>(const_cast<void*>(payload));
                return f(Forward<Args>(args)...);
            };
        }

        template <auto F>
        constexpr void Connect() noexcept
        {
            m_payload = nullptr;

            if constexpr (std::is_invocable_r_v<R, decltype(F), Args...>)
            {
                m_func = [](const void*, Args... args) -> R
                {
                    return R(std::invoke(F, Forward<Args>(args)...));
                };
            }
            else if constexpr (std::is_member_pointer_v<decltype(F)>)
            {
                m_func = Wrap<F>(_IndexSequenceForFunction<TypeListElement<0, TypeList<Args...>>>(FunctionPointer<decltype(F)>{}));
            }
            else
            {
                m_func = Wrap<F>(_IndexSequenceForFunction(FunctionPointer<decltype(F)>{}));
            }
        }

        template <auto F, typename T>
        void Connect(T& payload) noexcept
        {
            m_payload = &payload;

            if constexpr (std::is_invocable_r_v<R, decltype(F), T&, Args...>)
            {
                m_func = [](const void* payload, Args... args) -> R
                {
                    T* t = static_cast<T*>(const_cast<ConstnessAs<void, T>*>(payload));
                    return R(std::invoke(F, *t, Forward<Args>(args)...));
                };
            }
            else
            {
                m_func = Wrap<F>(payload, _IndexSequenceForFunction(FunctionPointer<decltype(F), T>{}));
            }
        }

        template <auto F, typename T>
        void Connect(T* payload) noexcept
        {
            m_payload = payload;

            if constexpr (std::is_invocable_r_v<R, decltype(F), T*, Args...>)
            {
                m_func = [](const void* payload, Args... args) -> R
                {
                    T* t = static_cast<T*>(const_cast<ConstnessAs<void, T>*>(payload));
                    return R(std::invoke(F, t, Forward<Args>(args)...));
                };
            }
            else
            {
                m_func = Wrap<F>(payload, _IndexSequenceForFunction(FunctionPointer<decltype(F), T>{}));
            }
        }

        void Connect(FunctionType* func, const void* payload = nullptr) noexcept
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
        template<auto F, size_t... Index>
        [[nodiscard]] static constexpr auto Wrap(std::index_sequence<Index...>) noexcept
        {
            return [](const void*, Args... args) -> R
            {
                [[maybe_unused]] const auto arguments = std::forward_as_tuple(Forward<Args>(args)...);
                return static_cast<R>(std::invoke(F, Forward<TypeListElement<Index, TypeList<Args...>>>(std::get<Index>(arguments))...));
            };
        }

        template<auto F, typename T, size_t... Index>
        [[nodiscard]] static constexpr auto Wrap(T&, std::index_sequence<Index...>) noexcept
        {
            return [](const void* payload, Args... args) -> R
            {
                [[maybe_unused]] const auto arguments = std::forward_as_tuple(Forward<Args>(args)...);
                T* t = static_cast<T*>(const_cast<ConstnessAs<void, T>*>(payload));
                return static_cast<R>(std::invoke(F, *t, Forward<TypeListElement<Index, TypeList<Args...>>>(std::get<Index>(arguments))...));
            };
        }

        template<auto F, typename T, size_t... Index>
        [[nodiscard]] static constexpr auto Wrap(T*, std::index_sequence<Index...>) noexcept
        {
            return [](const void* payload, Args... args) -> R
            {
                [[maybe_unused]] const auto arguments = std::forward_as_tuple(Forward<Args>(args)...);
                T* t = static_cast<T*>(const_cast<ConstnessAs<void, T>*>(payload));
                return static_cast<R>(std::invoke(F, t, Forward<TypeListElement<Index, TypeList<Args...>>>(std::get<Index>(arguments))...));
            };
        }

    private:
        const void* m_payload{ nullptr };
        FunctionType* m_func{ nullptr };
    };

    // deduction guide
    template<typename R, typename... Args>
    Delegate(R(*)(Args...)) -> Delegate<R(Args...)>;

    // deduction guide
    template<typename R, typename... Args>
    Delegate(R(*)(const void*, Args...), const void* = nullptr) -> Delegate<R(Args...)>;
}
