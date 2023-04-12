// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/type_traits.h"
#include "he/core/types.h"

namespace he
{
    /// \internal
    struct _Invoke_Functor
    {
        template <class T, class... Args>
        static HE_FORCE_INLINE constexpr auto Call(T&& obj, Args&&... args)
            noexcept(noexcept(static_cast<T&&>(obj)(static_cast<Args&&>(args)...)))
            -> decltype(static_cast<T&&>(obj)(static_cast<Args&&>(args)...))
        {
            return static_cast<T&&>(obj)(static_cast<Args&&>(args)...);
        }
    };

    struct _Invoke_PMF_Object
    {
        template <class T, class Arg1, class... Args>
        static HE_FORCE_INLINE constexpr auto Call(T obj, Arg1&& arg1, Args&&... args)
            noexcept(noexcept((static_cast<Arg1&&>(arg1).*obj)(static_cast<Args&&>(args)...)))
            -> decltype((static_cast<Arg1&&>(arg1).*obj)(static_cast<Args&&>(args)...))
        {
            return (static_cast<Arg1&&>(arg1).*obj)(static_cast<Args&&>(args)...);
        }
    };

    struct _Invoke_PMF_Pointer
    {
        template <class T, class Arg1, class... Args>
        static HE_FORCE_INLINE constexpr auto Call(T obj, Arg1&& arg1, Args&&... args)
            noexcept(noexcept(((*static_cast<Arg1&&>(arg1)).*obj)(static_cast<Args&&>(args)...)))
            -> decltype(((*static_cast<Arg1&&>(arg1)).*obj)(static_cast<Args&&>(args)...))
        {
            return ((*static_cast<Arg1&&>(arg1)).*obj)(static_cast<Args&&>(args)...);
        }
    };

    struct _Invoke_PMD_Object
    {
        template <class T, class Arg1>
        static HE_FORCE_INLINE constexpr auto Call(T obj, Arg1&& arg1) noexcept
            -> decltype(static_cast<Arg1&&>(arg1).*obj)
        {
            return static_cast<Arg1&&>(arg1).*obj;
        }
    };

    struct _Invoke_PMD_Pointer
    {
        template <class T, class Arg1>
        static HE_FORCE_INLINE constexpr auto Call(T obj, Arg1&& arg1)
            noexcept(noexcept((*static_cast<Arg1&&>(arg1)).*obj))
            -> decltype((*static_cast<Arg1&&>(arg1)).*obj)
        {
            return (*static_cast<Arg1&&>(arg1)).*obj;
        }
    };

    template <typename T, typename Arg1,
        bool IsPmf = IsMemberFunctionPointer<RemoveCVRef<T>>,
        bool IsPmd = IsMemberObjectPointer<RemoveCVRef<T>>>
    struct _Invoker;

    template <typename T, typename Arg1>
    struct _Invoker<T, Arg1, false, false> : _Invoke_Functor {};

    template <typename T, typename Arg1>
    struct _Invoker<T, Arg1, true, false>
        : Conditional<IsBaseOf<MemberPointerObjectType<RemoveCVRef<T>>, RemoveReference<Arg1>>, _Invoke_PMF_Object, _Invoke_PMF_Pointer>
    {};

    template <typename T, typename Arg1>
    struct _Invoker<T, Arg1, false, true>
        : Conditional<IsBaseOf<MemberPointerObjectType<RemoveCVRef<T>>, RemoveReference<Arg1>>, _Invoke_PMD_Object, _Invoke_PMD_Pointer>
    {};
    /// \endinternal

    template <typename T>
    HE_FORCE_INLINE constexpr auto Invoke(T&& obj)
        noexcept(noexcept(static_cast<T&&>(obj)()))
        -> decltype(static_cast<T&&>(obj)())
    {
        return static_cast<T&&>(obj)();
    }

    template <typename T, typename Arg1, typename... Args>
    HE_FORCE_INLINE constexpr auto Invoke(T&& obj, Arg1&& arg1, Args&&... args)
        noexcept(noexcept(_Invoker<T, Arg1>::Call(static_cast<T&&>(obj), static_cast<Arg1&&>(arg1), static_cast<Args&&>(args)...)))
        -> decltype(_Invoker<T, Arg1>::Call(static_cast<T&&>(obj), static_cast<Arg1&&>(arg1), static_cast<Args&&>(args)...))
    {
        if constexpr (IsBaseOf<_Invoke_Functor, _Invoker<T, Arg1>>)
        {
            return static_cast<T&&>(obj)(static_cast<Arg1&&>(arg1), static_cast<Args&&>(args)...);
        }
        else if constexpr (IsBaseOf<_Invoke_PMF_Object, _Invoker<T, Arg1>>)
        {
            return (static_cast<Arg1&&>(arg1).*obj)(static_cast<Args&&>(args)...);
        }
        else if constexpr (IsBaseOf<_Invoke_PMF_Pointer, _Invoker<T, Arg1>>)
        {
            return ((*static_cast<Arg1&&>(arg1)).*obj)(static_cast<Args&&>(args)...);
        }
        else if constexpr (IsBaseOf<_Invoke_PMD_Object, _Invoker<T, Arg1>>)
        {
            return static_cast<Arg1&&>(arg1).*obj;
        }
        else if constexpr (IsBaseOf<_Invoke_PMD_Pointer, _Invoker<T, Arg1>>)
        {
            return (*static_cast<Arg1&&>(arg1)).*obj;
        }
    }

    /// \internal
    template <typename T> [[nodiscard]] T _InvokeTraits_FakeCopyInit(T) noexcept;
    template <typename T> T _InvokeTraits_ReturnsExactly() noexcept;

    template <typename From, typename To, typename = void>
    struct _InvokeTraits_Convertible : FalseType {};

    template <typename From, typename To>
    struct _InvokeTraits_Convertible<From, To, Void<decltype(_InvokeTraits_FakeCopyInit<To>(_InvokeTraits_ReturnsExactly<From>()))>> : TrueType {};

    template <typename R>
    struct _InvokeTraits_Valid
    {
        using ResultType = R;
        static constexpr bool IsInvocable = true;
        template <typename R2> static constexpr bool IsInvocableR = Disjunction<BoolConstant<IsVoid<R2>>, _InvokeTraits_Convertible<R, R2>>;
    };

    template <typename, typename>
    struct _InvokeTraits_NoArgs
    {
        static constexpr bool IsInvocable = false;
        template <typename R> static constexpr bool IsInvocableR = false;
    };

    template <typename T>
    struct _InvokeTraits_NoArgs<Void<decltype(DeclVal<T>()())>, T>
        : _InvokeTraits_Valid<decltype(DeclVal<T>()())>
    {};

    template <typename, typename...>
    struct _InvokeTraits_WithArgs
    {
        static constexpr bool IsInvocable = false;
        template <typename R> static constexpr bool IsInvocableR = false;
    };

    template <typename T, typename Arg1, typename... Args>
    using _InvokeTraits_WithArgs_Decltype = decltype(_Invoker<T, Arg1>::Call(DeclVal<T>(), DeclVal<Arg1>(), DeclVal<Args>()...));

    template <typename T, typename Arg1, typename... Args>
    struct _InvokeTraits_WithArgs<Void<_InvokeTraits_WithArgs_Decltype<T, Arg1, Args...>>, T, Arg1, Args...>
        : _InvokeTraits_Valid<_InvokeTraits_WithArgs_Decltype<T, Arg1, Args...>>
    {};

    template <typename T, typename... Args>
    using _InvokeTraits = Conditional<sizeof...(Args) == 0, _InvokeTraits_NoArgs<void, T>, _InvokeTraits_WithArgs<void, T, Args...>>;
    /// \endinternal

    template <typename T, typename... Args>
    inline constexpr bool IsInvocable = _InvokeTraits<T, Args...>::IsInvocable;

    template <typename R, typename T, typename... Args>
    inline constexpr bool IsInvocableR = _InvokeTraits<T, Args...>::template IsInvocableR<R>;

    template <typename T, typename... Args>
    using InvokeResult = typename _InvokeTraits<T, Args...>::ResultType;
}
