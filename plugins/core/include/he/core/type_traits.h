// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/types.h"

namespace he
{
    // --------------------------------------------------------------------------------------------
    // Utility types

    /// Always evaluates to false.
    /// Useful to attach dependent names for static_assert.
    ///
    /// \tparam ...T The dependent types.
    template <typename... T>
    constexpr bool AlwaysFalse = false;

    /// Utility metafunction that maps a sequence of any types to the type `void`.
    /// This metafunction is a convenient way to leverage SFINAE.
    ///
    /// \tparam ...T The type to map to void.
    template <typename... T>
    using Void = void;

    /// Wraps a static constant of specified type.
    ///
    /// \tparam T The type of the value to hold.
    /// \tparam Val The value to hold.
    template <typename T, T Val>
    struct IntegralConstant { static constexpr T Value = Val; };

    /// A helper alias template defined for the common case where you want an IntegralConstant
    /// that is a boolean.
    ///
    /// \tparam Val The value to hold.
    template <bool Val>
    using BoolConstant = IntegralConstant<bool, Val>;

    /// A helper alias template defined for the common case where you want an IntegralConstant
    /// that is a `uint32_t` index.
    ///
    /// \tparam Val The value to hold.
    template <uint32_t Val>
    using IndexConstant = IntegralConstant<uint32_t, Val>;

    /// A helper alias for a BoolConstant that holds `true`.
    using TrueType = BoolConstant<true>;

    /// A helper alias for a BoolConstant that holds `false`.
    using FalseType = BoolConstant<false>;

    /// \internal
    template <bool Test, typename T, typename F> struct _Conditional { using Type = T; };
    template <typename T, typename F> struct _Conditional<false, T, F> { using Type = F; };
    /// \endinternal

    /// Evaluates to the type `T` if `Test` is `true`, or `F` if `Test` is `false`.
    ///
    /// \tparam Test The value used to select `T` or `F`.
    /// \tparam T The type to use if `Test` is `true`.
    /// \tparam F The type to use if `Test` is `false`.
    template <bool Test, typename T, typename F>
    using Conditional = typename _Conditional<Test, T, F>::Type;

    /// \internal
    template <bool Test, typename T = void> struct _EnableIf {};
    template <typename T> struct _EnableIf<true, T> { using Type = T; };
    /// \endinternal

    /// Evaluates to the type `T` if `Test` is `true`. Fails to compile otherwise.
    ///
    /// \tparam Test The value used to determine if a type should properly resolve.
    /// \tparam T The type to resolve to if `Test` is true.
    template <bool Test, typename T = void>
    using EnableIf = typename _EnableIf<Test, T>::Type;

    /// \internal
    template <bool Pass, typename T, typename... Args> struct _ConjunctionBase { using Type = T; };
    template <typename True, typename T, typename... Args> struct _ConjunctionBase<true, True, T, Args...> { using Type = typename _ConjunctionBase<T::Value, T, Args...>::Type; };

    template <typename... Args> struct _Conjunction : TrueType {};
    template <typename T, typename... Rest> struct _Conjunction<T, Rest...> : _ConjunctionBase<T::Value, T, Rest...>::Type {};
    /// \endinternal

    /// Forms the logical conjunction of the type traits `Args...`, effectively performing a
    /// logical AND on the sequence of traits.
    ///
    /// \note `static_cast<bool>(Arg::Value)` must be a valid expression for all traits.
    ///
    /// \tparam ...Args Type traits to evaluate.
    template <typename... Args>
    inline constexpr bool Conjunction = _Conjunction<Args...>::Value;

    /// \internal
    template <bool Pass, typename T, typename... Args> struct _DisjunctionBase { using Type = T; };
    template <typename False, typename T, typename... Args> struct _DisjunctionBase<false, False, T, Args...> { using Type = typename _DisjunctionBase<T::Value, T, Args...>::Type; };

    template <typename... Args> struct _Disjunction : FalseType {};
    template <typename T, typename... Args> struct _Disjunction<T, Args...> : _DisjunctionBase<T::Value, T, Args...>::Type {};
    /// \endinternal

    /// Forms the logical disjunction of the type traits `Args...`, effectively performing a
    /// logical OR on the sequence of traits.
    ///
    /// \note `static_cast<bool>(Arg::Value)` must be a valid expression for all traits.
    ///
    /// \tparam ...Args Type traits to evaluate.
    template <typename... Args>
    inline constexpr bool Disjunction = _Disjunction<Args...>::Value;

    // --------------------------------------------------------------------------------------------
    // Type modifiers

    /// \internal
    template <typename T> struct _RemoveCV { using Type = T; };
    template <typename T> struct _RemoveCV<const T> { using Type = T; };
    template <typename T> struct _RemoveCV<volatile T> { using Type = T; };
    template <typename T> struct _RemoveCV<const volatile T> { using Type = T; };
    /// \endinternal

    /// Removes the topmost `const`, or `volatile`, or both, from `T` if present.
    ///
    /// \tparam T The type to modify.
    template <typename T>
    using RemoveCV = typename _RemoveCV<T>::Type;

    /// \internal
    template <typename T> struct _RemoveConst { using Type = T; };
    template <typename T> struct _RemoveConst<const T> { using Type = T; };
    /// \endinternal

    /// Removes the topmost `const` from `T` if present.
    ///
    /// \tparam T The type to modify.
    template <typename T>
    using RemoveConst = typename _RemoveConst<T>::Type;

    /// \internal
    template <typename T> struct _RemoveVolatile { using Type = T; };
    template <typename T> struct _RemoveVolatile<volatile T> { using Type = T; };
    /// \endinternal

    /// Removes the topmost `volatile` from `T` if present.
    ///
    /// \tparam T The type to modify.
    template <typename T>
    using RemoveVolatile = typename _RemoveVolatile<T>::Type;

    /// \internal
    template <typename T> struct _RemoveReference { using Type = T; };
    template <typename T> struct _RemoveReference<T&> { using Type = T; };
    template <typename T> struct _RemoveReference<T&&> { using Type = T; };
    /// \endinternal

    /// If `T` is a reference type, resolves to the type referred to by `T`.
    ///
    /// \tparam T The type to modify.
    template <typename T>
    using RemoveReference = typename _RemoveReference<T>::Type;

    /// \internal
    template <typename T> struct _RemoveRValueRef { using Type = T; };
    template <typename T> struct _RemoveRValueRef<T&&> { using Type = T; };
    /// \endinternal

    /// If `T` is an r-value reference type, resolves to the type referred to by `T`.
    ///
    /// \tparam T The type to modify.
    template <typename T>
    using RemoveRValueReference = typename _RemoveRValueRef<T>::Type;

    /// \internal
    template <typename T, typename = void> struct _AddReference { using LV = T; using RV = T; };
    template <typename T> struct _AddReference<T, Void<T&>> { using LV = T&; using RV = T&&; };
    /// \endinternal

    /// If `T` is a function type that has no cv- or ref- qualifier or an object type, evaluates
    /// to `T&`. If `T` is an rvalue reference to some type `U`, then evaluates to `U&`.
    /// Otherwise, evaluates to `T`.
    ///
    /// \tparam T The type to modify.
    template <typename T>
    using AddLValueReference = typename _AddReference<T>::LV;

    /// If `T` is a function type that has no cv- or ref- qualifier or an object type, evaluates
    /// to `T&&`, otherwise evaluates to `T`.
    ///
    /// \tparam T The type to modify.
    template <typename T>
    using AddRValueReference = typename _AddReference<T>::RV;

    /// \internal
    template <typename T> struct _RemovePointer { using Type = T; };
    template <typename T> struct _RemovePointer<T*> { using Type = T; };
    template <typename T> struct _RemovePointer<T* const> { using Type = T; };
    template <typename T> struct _RemovePointer<T* volatile> { using Type = T; };
    template <typename T> struct _RemovePointer<T* const volatile> { using Type = T; };
    /// \endinternal

    /// Resolves to the type pointed to by `T`, or if `T` is not a pointer then the same type `T`.
    ///
    /// \tparam T The type to modify.
    template <typename T>
    using RemovePointer = typename _RemovePointer<T>::Type;

    /// \internal
    template <typename T, typename = void> struct _AddPointer { using Type = T; };
    template <typename T> struct _AddPointer<T, Void<RemoveReference<T>*>> { using Type = RemoveReference<T>*; };
    /// \endinternal

    /// If `T` is a reference type, then evaluates to a pointer to the referred type.
    /// Otherwise, if `T` names an object type, a function type that is not cv- or ref-qualified,
    /// or a (possibly cv-qualified) void type, evaluates to the type `T*`.
    /// Otherwise, if `T` is a cv- or ref-qualified function type, evaluates to the type `T`.
    ///
    /// \tparam T The type to modify.
    template <typename T>
    using AddPointer = typename _AddPointer<T>::Type;

    /// \internal
    template <typename T> struct _RemoveExtent { using Type = T; };
    template <typename T, uint32_t N> struct _RemoveExtent<T[N]> { using Type = T; };
    template <typename T> struct _RemoveExtent<T[]> { using Type = T; };
    /// \endinternal

    /// If `T` is an array of some type `X`, evaluates to the type `X`, otherwise evaluates to `T`.
    /// Note that if `T` is a multidimensional array, only the first dimension is removed.
    ///
    /// \tparam T The type to modify.
    template <typename T>
    using RemoveExtent = typename _RemoveExtent<T>::Type;

    /// \internal
    template <typename T> struct _RemoveAllExtents { using Type = T; };
    template <typename T, uint32_t N> struct _RemoveAllExtents<T[N]> { using Type = typename _RemoveAllExtents<T>::Type; };
    template <typename T> struct _RemoveAllExtents<T[]> { using Type = typename _RemoveAllExtents<T>::Type; };
    /// \endinternal

    /// If `T` is a multidimensional array of some type `X`, evaluates to the type `X`.
    /// Otherwise, evaluates to `T`.
    ///
    /// \tparam T The type to modify.
    template <typename T>
    using RemoveAllExtents = typename _RemoveAllExtents<T>::Type;

    /// \internal
    template <typename T> struct _IdentityType { using Type = T; };
    /// \endinternal

    template <typename T>
    using IdentityType = typename _IdentityType<T>::Type;

    template <typename T>
    using RemoveCVRef = RemoveCV<RemoveReference<T>>;

    // --------------------------------------------------------------------------------------------
    // Type relationships

    /// Evaluates to `true` if `T` and `U` are the same type, taking into account const/volatile
    /// qualifications. Otherwise, evaluates to `false`.
    ///
    /// \note For any two types, `IsSame<T, U> == IsSame<U, T>`.
    ///
    /// \tparam T The first type to check.
    /// \tparam T The second type to check.
    template <typename T, typename U>
    inline constexpr bool IsSame
#if HE_HAS_BUILTIN(__is_same)
        = __is_same(T, U);
#else
        = false;

    /// \ignore
    template <typename T>
    inline constexpr bool IsSame<T, T> = true;
#endif

    /// Evaluates to `true` if all types passed as template parameters are the same.
    ///
    /// \tparam T The first type to check.
    /// \tparam ...U The rest of the types to check.
    template <typename T, typename... U>
    inline constexpr bool IsAllSame = (IsSame<T, U> && ...);

    /// Evaluates to `true` if `T` can be found in the pack `U`, otherwise evaluates
    /// to `false`.
    ///
    /// \tparam T The type to look for.
    /// \tparam ...U The list of types to search in.
    template <typename T, typename... U>
    inline constexpr bool IsAnyOf = (IsSame<T, U> || ...);

    /// Evaluates to `true` if `Derived` inherits from `Base`, or they are the same type.
    ///
    /// \tparam Base The base typename To look for.
    /// \tparam Derived The derived typename To check if it inherits from Base.
    template <typename Base, typename Derived>
    inline constexpr bool IsBaseOf = __is_base_of(Base, Derived);

    /// Evaluates to `true` if `From` can be implicitly converted to `To`.
    ///
    /// \tparam From The type to check for conversion from.
    /// \tparam To The type to check for conversion to.
    template <typename From, typename To>
    inline constexpr bool IsConvertible = __is_convertible_to(From, To);

    /// \internal
    template <typename T, template <typename...> typename Template> struct _IsSpecialization : FalseType {};
    template <template <typename...> typename Template, typename... Types> struct _IsSpecialization<Template<Types...>, Template> : TrueType {};
    /// \endinternal

    template <typename T, template <typename...> typename Template>
    inline constexpr bool IsSpecialization = _IsSpecialization<T, Template>::Value;

    // --------------------------------------------------------------------------------------------
    // Type categories

    template <typename T>
    inline constexpr bool IsVoid = IsSame<RemoveCV<T>, void>;

    template <typename T>
    inline constexpr bool IsNullptr = IsSame<RemoveCV<T>, decltype(nullptr)>;

    template <typename T>
    inline constexpr bool IsIntegral = IsAnyOf<RemoveCV<T>, bool, char, signed char, unsigned char, wchar_t, char8_t, char16_t, char32_t, short, unsigned short, int, unsigned int, long, unsigned long, long long, unsigned long long>;

    template <typename T>
    inline constexpr bool IsFloatingPoint = IsAnyOf<RemoveCV<T>, float, double, long double>;

    template <typename T>
    inline constexpr bool IsArray = false;

    template <typename T, uint32_t N>
    inline constexpr bool IsArray<T[N]> = true;

    template <typename T>
    inline constexpr bool IsArray<T[]> = true;

    template <typename T>
    inline constexpr bool IsEnum = __is_enum(T);

    template <typename T>
    inline constexpr bool IsUnion = __is_union(T);

    template <typename T>
    inline constexpr bool IsClass = __is_class(T);

    template <typename T>
    inline constexpr bool IsPointer = false;

    template <typename T>
    inline constexpr bool IsPointer<T*> = true;

    template <typename T>
    inline constexpr bool IsPointer<T* const> = true;

    template <typename T>
    inline constexpr bool IsPointer<T* volatile> = true;

    template <typename T>
    inline constexpr bool IsPointer<T* const volatile> = true;

    template <typename T>
    inline constexpr bool IsLValueReference = false;

    template <typename T>
    inline constexpr bool IsLValueReference<T&> = true;

    template <typename T>
    inline constexpr bool IsRValueReference = false;

    template <typename T>
    inline constexpr bool IsRValueReference<T&&> = true;

    template <typename T>
    inline constexpr bool IsReference = IsLValueReference<T> || IsRValueReference<T>;

    template <typename T>
    inline constexpr bool IsArithmetic = IsIntegral<T> || IsFloatingPoint<T>;

    template <typename T>
    inline constexpr bool IsFundamental = IsArithmetic<T> || IsVoid<T> || IsNullptr<T>;

    template <typename T>
    inline constexpr bool IsCompound = !IsFundamental<T>;

    // --------------------------------------------------------------------------------------------
    // Type properties

    /// Evaluates to `true` if `T` is const-qualified, otherwise evaluates to `false`.
    ///
    /// \tparam T The type to check.
    template <typename T>
    inline constexpr bool IsConst = false;

    /// \ignore
    template <typename T>
    inline constexpr bool IsConst<const T> = true;

    /// Evaluates to `true` if `T` is volatile-qualified, otherwise evaluates to `false`.
    ///
    /// \tparam T The type to check.
    template <typename T>
    inline constexpr bool IsVolatile = false;

    /// \ignore
    template <typename T>
    inline constexpr bool IsVolatile<volatile T> = true;

    /// Evaluates to `true` if `T` is a trivial type, otherwise evaluates to `false`.
    /// \see https://en.cppreference.com/w/cpp/named_req/TrivialType
    ///
    /// \tparam T The type to check.
    template <typename T>
    inline constexpr bool IsTrivial = __is_trivially_constructible(T) && __is_trivially_copyable(T);

    /// Evaluates to `true` if `T` is a trivially copyable type, otherwise evaluates to `false`.
    /// \see https://en.cppreference.com/w/cpp/named_req/TriviallyCopyable
    ///
    /// \tparam T The type to check.
    template <typename T>
    inline constexpr bool IsTriviallyCopyable = __is_trivially_copyable(T);

    /// Evaluates to `true` if `T` is a standard-layout type, otherwise evaluates to `false`.
    ///
    /// \tparam T The type to check.
    template <typename T>
    inline constexpr bool IsStandardLayout = __is_standard_layout(T);

    /// Evaluates to `true` if `T` is an empty type. Tthat is, a non-union class type with
    /// no non-static data members other than bit-fields of size 0, no virtual functions,
    /// no virtual base classes, and no non-empty base classes. Otherwise evaluates to `false`.
    ///
    /// @tparam T
    template <typename T>
    inline constexpr bool IsEmpty = __is_empty(T);

    /// Evaluates to `true` if `T` is a polymorphic class. That is, a non-union class that
    /// declares or inherits at least one virtual function. Otherwise evaluates to `false`.
    ///
    /// @tparam T The type to check.
    template <typename T>
    inline constexpr bool IsPolymorphic = __is_polymorphic(T);

    /// Evaluates to `true` if `T` is an abstract class. That is, a non-union class that declares
    /// or inherits at least one pure virtual function. Otherwise evaluates to `false`.
    ///
    /// @tparam T The type to check.
    template <typename T>
    inline constexpr bool IsAbstract = __is_abstract(T);

    /// Evaluates to `true` if `T` is a final class. That is, a class declared with the `final`
    /// specifier. Otherwise evaluates to `false`.
    ///
    /// @tparam T The type to check.
    template <typename T>
    inline constexpr bool IsFinal = __is_final(T);

    /// Evaluates to `true` if `T` is an aggregate type, otherwise evaluates to `false`.
    /// \see https://en.cppreference.com/w/cpp/language/aggregate_initialization
    ///
    /// \tparam T The type to check.
    template <typename T>
    inline constexpr bool IsAggregate = __is_aggregate(T);

    /// \internal
    template <typename T, bool = IsIntegral<T>>
    struct _Signed
    {
        static constexpr bool Signed = static_cast<RemoveCV<T>>(-1) < static_cast<RemoveCV<T>>(0);
        static constexpr bool Unsigned = !Signed;
    };

    template <typename T>
    struct _Signed<T, false>
    {
        static constexpr bool Signed = IsFloatingPoint<T>;
        static constexpr bool Unsigned = false;
    };
    /// \endinternal

    /// Evaluates to `true` if `T` is a signed arithmetic type, otherwise evaluates to `false`.
    ///
    /// \tparam T The type to check.
    template <typename T>
    inline constexpr bool IsSigned = _Signed<T>::Signed;

    /// Evaluates to `true` if `T` is a unsigned arithmetic type, otherwise evaluates to `false`.
    ///
    /// \tparam T The type to check.
    template <typename T>
    inline constexpr bool IsUnsigned = _Signed<T>::Unsigned;

    /// If `T` is an array type, evaluates to a value equal to the number of dimensions of the
    /// array. For any other type, the value is 0.
    ///
    /// \tparam T The array type to check.
    template <typename T>
    inline constexpr uint32_t Rank = 0; // determine number of dimensions of array _Ty

    /// \ignore
    template <typename T, uint32_t N>
    inline constexpr uint32_t Rank<T[N]> = Rank<T> + 1;

    /// \ignore
    template <typename T>
    inline constexpr uint32_t Rank<T[]> = Rank<T> + 1;

    /// If `T` is an array type, evaluates to a value equal to the number of elements along the
    /// `Dim`th dimension of the array, if `Dim` is in the range `[0, Rank<T>)`. For any other
    /// type, or if `T` is an array of unknown bound along its first dimension and `Dim` is 0,
    /// the value is 0.
    ///
    /// \tparam T The type to get the extent of.
    /// \tparam Dim Optional. The dimension of the array to get the extent of. Default is zero.
    template <typename T, uint32_t Dim = 0>
    inline constexpr uint32_t Extent = 0;

    /// \ignore
    template <typename T, uint32_t N>
    inline constexpr uint32_t Extent<T[N], 0> = N;

    /// \ignore
    template <typename T, uint32_t Dim, uint32_t N>
    inline constexpr uint32_t Extent<T[N], Dim> = Extent<T, Dim - 1>;

    /// \ignore
    template <typename T, uint32_t Dim>
    inline constexpr uint32_t Extent<T[], Dim> = Extent<T, Dim - 1>;

    /// \internal
    template <typename T> struct _MemberPointerObjectType;
    template <typename T, typename U> struct _MemberPointerObjectType<T U::*> { using Type = U; };
    /// \endinternal

    template <typename T>
    using MemberPointerObjectType = typename _MemberPointerObjectType<T>::Type;

    // --------------------------------------------------------------------------------------------
    // Type categories (continued)

    template <typename T>
    inline constexpr bool IsObject = IsConst<const T> && !IsVoid<T>;

    template <typename T>
    inline constexpr bool IsFunction = !IsConst<const T> && !IsReference<T>;

#if HE_COMPILER_MSVC
    /// \internal
    template <typename T>
    struct _IsMemberPointer
    {
        static constexpr bool Value = false;
        static constexpr bool IsFunc = false;
        static constexpr bool IsObj = false;
    };

    template <typename T, typename U>
    struct _IsMemberPointer<T U::*>
    {
        static constexpr bool Value = true;
        static constexpr bool IsFunc = IsFunction<T>;
        static constexpr bool IsObj = !IsFunc;
    };
    /// \endinternal

    template <typename T>
    inline constexpr bool IsMemberFunctionPointer = _IsMemberPointer<RemoveCV<T>>::IsFunc;

    template <typename T>
    inline constexpr bool IsMemberObjectPointer = _IsMemberPointer<RemoveCV<T>>::IsObj;

    template <typename T>
    inline constexpr bool IsMemberPointer = _IsMemberPointer<RemoveCV<T>>::Value;
#else
    template <typename T>
    inline constexpr bool IsMemberFunctionPointer = __is_member_function_pointer(T);

    template <typename T>
    inline constexpr bool IsMemberObjectPointer = __is_member_object_pointer(T);

    template <typename T>
    inline constexpr bool IsMemberPointer = __is_member_pointer(T);
#endif

    template <typename T>
    inline constexpr bool IsScalar = IsArithmetic<T> || IsEnum<T> || IsPointer<T> || IsMemberPointer<T> || IsNullptr<T>;

    // --------------------------------------------------------------------------------------------
    // Type modifiers (continued)

    /// \internal
    template <typename T>
    struct _Decay
    {
        using T1 = RemoveReference<T>;
        using T2 = Conditional<IsFunction<T1>, AddPointer<T1>, RemoveCV<T1>>;
        using Type = Conditional<IsArray<T1>, AddPointer<RemoveExtent<T1>>, T2>;
    };
    /// \endinternal

    /// Applies lvalue-to-rvalue, array-to-pointer, and function-to-pointer implicit conversions
    /// to the type `T`, removes cv-qualifiers, and evaluates to the resulting type.
    ///
    /// \tparam T The type to modify.
    template <typename T>
    using Decay = typename _Decay<T>::Type;

    /// If `T` is a complete enumeration type, evaluates to the underlying type of `T`.
    ///
    /// \tparam T The enum type to get the underlying type for.
    template <typename T> requires(IsEnum<T>)
    using UnderlyingType = __underlying_type(T);

    /// Modifies the type `T` to have the same constness as the type `U`.
    ///
    /// \tparam T The type to modify.
    /// \tparam U The type to copy the constness of.
    template <typename T, typename U>
    using ConstnessAs = Conditional<IsConst<U>, const T, RemoveConst<T>>;

    /// Forms lvalue reference to const type of t.
    ///
    /// \tparam T The type to modify.
    /// \param[in] t The value to return a const ref to.
    template <typename T> constexpr const T& AsConst(T& t) noexcept { return t; }

    /// \internal
    template <typename T> void AsConst(const T&&) = delete;
    /// \endinternal

    /// \internal
    template <typename T, bool Enum> struct _UnwrapEnum { using Type = T; };
    template <typename T> struct _UnwrapEnum<T, true> { using Type = UnderlyingType<T>; };
    /// \endinternal

    /// Resolves a type `T` to the underlying type if it is an enum, or to `T` if it is not.
    ///
    /// \tparam T The type to modify.
    template <typename T>
    using UnwrapEnum = _UnwrapEnum<T, IsEnum<T>>::Type;

#if HE_HAS_BUILTIN(__make_unsigned)
    template <typename T>
    using MakeUnsigned = __make_unsigned(T);
#else
    /// \internal
    template <size_t> struct _MakeUnsigned;
    template <> struct _MakeUnsigned<1> { template <typename T> using Type = EnableIf<!IsSame<T, bool>, unsigned char>; };
    template <> struct _MakeUnsigned<2> { template <typename T> using Type = unsigned short; };
    template <> struct _MakeUnsigned<4> { template <typename T> using Type = Conditional<IsSame<T, long> || IsSame<T, unsigned long>, unsigned long, unsigned int>; };
    template <> struct _MakeUnsigned<8> { template <typename T> using Type = Conditional<IsSame<T, long> || IsSame<T, unsigned long>, unsigned long, unsigned long long>; };
    /// \endinternal

    template <typename T>
    using MakeUnsigned = _MakeUnsigned<sizeof(T)>::template Type<T>;
#endif

    // --------------------------------------------------------------------------------------------
    // Supported operations

    /// Evaluates to `true` if `T` is an object or reference type and the variable definition
    /// `T obj(DeclVal<Args>()...);` is well-formed, otherwise evaluates to `false`.
    ///
    /// \tparam T The type to check.
    /// \tparam ...Args The arguments to attempt to pass to a constructor.
    template <typename T, typename... Args>
    inline constexpr bool IsConstructible = __is_constructible(T, Args...);

    /// Evaluates to `true` if `T` can be default constructed. Same as `IsConstructible<T>`.
    ///
    /// \tparam T The type to check.
    template <typename T>
    inline constexpr bool IsDefaultConstructible = __is_constructible(T);

    /// Evaluates to `true` if `T` can be copy constructed. Same as `IsConstructible<T, const T&>`.
    ///
    /// \tparam T The type to check.
    template <typename T>
    inline constexpr bool IsCopyConstructible = __is_constructible(T, AddLValueReference<const T>);

    /// Evaluates to `true` if `T` can be move constructed. Same as `IsConstructible<T, T&&>`.
    ///
    /// \tparam T The type to check.
    template <typename T>
    inline constexpr bool IsMoveConstructible = __is_constructible(T, AddRValueReference<T>);

    /// Evaluates to `true` if the expression `DeclVal<T>() = DeclVal<U>()` is
    /// well-formed in unevaluated context, otherwise evaluates to `false`.
    /// Access checks are performed as if from a context unrelated to either type.
    ///
    /// \tparam T The type to assign to.
    /// \tparam U The type to assign from.
    template <typename T, typename U>
    inline constexpr bool IsAssignable = __is_assignable(T, U);

    /// Evaluates to `true` if `T` can be copy assigned from `U`. Same as `IsAssignable<T&, const U&>`.
    ///
    /// \tparam T The type to assign to.
    /// \tparam U The type to assign from. Defaults to T.
    template <typename T, typename U = T>
    inline constexpr bool IsCopyAssignable = __is_assignable(AddLValueReference<T>, AddLValueReference<const U>);

    /// Evaluates to `true` if `T` can be move assigned from `U`. Same as `IsAssignable<T&, U&&>`.
    ///
    /// \tparam T The type to assign to.
    /// \tparam U The type to assign from. Defaults to T.
    template <typename T, typename U = T>
    inline constexpr bool IsMoveAssignable = __is_assignable(AddLValueReference<T>, AddRValueReference<U>);

    /// Evaluates to `true` if `T` can be destructed, otherwise evaluates to `false`.
    ///
    /// \tparam T The type to check.
    template <typename T>
    inline constexpr bool IsDestructible = __is_destructible(T);

    /// Same as \ref IsConstructible, but the constructor must also be trivial.
    ///
    /// \tparam T The type to check.
    /// \tparam ...Args The arguments to attempt to pass to a constructor.
    template <typename T, typename... Args>
    inline constexpr bool IsTriviallyConstructible = __is_trivially_constructible(T, Args...);

    /// Same as \ref IsDefaultConstructible, but the constructor must also be trivial.
    ///
    /// \tparam T The type to check.
    template <typename T>
    inline constexpr bool IsTriviallyDefaultConstructible = __is_trivially_constructible(T);

    /// Same as \ref IsCopyConstructible, but the constructor must also be trivial.
    ///
    /// \tparam T The type to check.
    template <typename T>
    inline constexpr bool IsTriviallyCopyConstructible = __is_trivially_constructible(T, AddLValueReference<const T>);

    /// Same as \ref IsMoveConstructible, but the constructor must also be trivial.
    ///
    /// \tparam T The type to check.
    template <typename T>
    inline constexpr bool IsTriviallyMoveConstructible = __is_trivially_constructible(T, AddRValueReference<T>);

    /// Same as \ref IsAssignable, but the operator must also be trivial.
    ///
    /// \tparam T The type to assign to.
    /// \tparam U The type to assign from.
    template <typename T, typename U>
    inline constexpr bool IsTriviallyAssignable = __is_trivially_assignable(T, U);

    /// Same as \ref IsCopyAssignable, but the operator must also be trivial.
    ///
    /// \tparam T The type to check.
    template <typename T>
    inline constexpr bool IsTriviallyCopyAssignable = __is_trivially_assignable(AddLValueReference<T>, AddLValueReference<const T>);

    /// Same as \ref IsMoveAssignable, but the operator must also be trivial.
    ///
    /// \tparam T The type to check.
    template <typename T>
    inline constexpr bool IsTriviallyMoveAssignable = __is_trivially_assignable(AddLValueReference<T>, AddRValueReference<T>);

    /// Same as \ref IsDestructible, but the destructor must also be trivial.
    ///
    /// \tparam T The type to check.
    template <typename T>
    inline constexpr bool IsTriviallyDestructible = __is_trivially_destructible(T);

    // --------------------------------------------------------------------------------------------
    // Integer sequences

    template <typename T, T... Values> requires(IsIntegral<T>)
    struct IntegerSequence
    {
        using Type = T;
        static constexpr uint32_t Size = sizeof...(Values);
    };

    template <typename T, T Size>
    using MakeIntegerSequence
#if HE_COMPILER_MSVC || HE_HAS_BUILTIN(__make_integer_seq)
        = __make_integer_seq<IntegerSequence, T, Size>;
#else
        = IntegerSequence<T, __integer_pack(Size)...>;
#endif

    template <uint32_t... Values>
    using IndexSequence = IntegerSequence<uint32_t, Values...>;

    template <uint32_t Size>
    using MakeIndexSequence = MakeIntegerSequence<uint32_t, Size>;

    template <typename... Types>
    using IndexSequenceFor = MakeIndexSequence<sizeof...(Types)>;

    // --------------------------------------------------------------------------------------------
    // Metaprogramming functions

    /// Detects whether the function call occurs within a constant-evaluated context. Returns `true`
    /// if the evaluation of the call occurs within the evaluation of an expression or conversion
    /// that is manifestly constant-evaluated; otherwise returns `false`.
    ///
    /// \return True if in a constant-evaluated context, or false otherwise.
    constexpr bool IsConstantEvaluated() noexcept { return __builtin_is_constant_evaluated(); }

    /// Converts any type `T` to a reference type, making it possible to use member functions in
    /// the operand of the `decltype` specifier without the need to go through constructors.
    ///
    /// \tparam T
    /// \return Cannot be called, and therefore does not return a value.
    template <typename T> AddRValueReference<T> DeclVal() noexcept { static_assert(AlwaysFalse<T>, "DeclVal is not allowed in an evaluated context."); }

    // --------------------------------------------------------------------------------------------
    // Aligned Storage

    /// A trivial standard-layout type suitable for use as uninitialized storage for any object
    /// whose size is at most `Len` and whose alignment requirement is a divisor of `Align`.
    ///
    /// \note Behavior is undefined if `Len == 0`
    ///
    /// \tparam Len The number of bytes to provide storage for.
    /// \tparam Align The alignment the bytes must satisfy.
    template <uint32_t Len, uint32_t Align>
    struct AlignedStorage
    {
        static_assert(Len > 0, "Size must be greater than zero.");
        static_assert(Align > 0 && (Align & (Align - 1)) == 0, "Alignment must be a power of two.");

        static constexpr uint32_t Size = Len;
        static constexpr uint32_t Alignment = Align;
        alignas(Alignment) uint8_t data[Size];
    };

    /// A trivial standard-layout type suitable for use as uninitialized storage for an object
    /// of type `T`.
    ///
    /// \tparam T The type to provide aligned storage for.
    template <typename T>
    using AlignedStorageFor = AlignedStorage<sizeof(T), alignof(T)>;

    // --------------------------------------------------------------------------------------------
    // Structure that contains a parameter pack and provides access into indexed types

    template <typename... T>
    struct TypeList
    {
        using Type = TypeList<T...>;
        static constexpr auto Size = sizeof...(T);
    };

    /// \internal
    template <uint32_t, typename>
    struct _TypeListElement;

    // Maybe one day Visual Studio will have this intrinsic too (probably not)
    // See: https://developercommunity.visualstudio.com/t/add-support-for-clangs-type-pack-element-to-improv/1439235
#if HE_HAS_BUILTIN(__type_pack_element)
    template <uint32_t I, typename... Ts>
    struct _TypeListElement<I, TypeList<Ts...>> { using Type = __type_pack_element<I, Ts...>; };
#else
    template <uint32_t Index, typename First, typename... Rest>
    struct _TypeListElement<Index, TypeList<First, Rest...>> : _TypeListElement<Index - 1u, TypeList<Rest...>> {};

    template <typename First, typename... Rest>
    struct _TypeListElement<0u, TypeList<First, Rest...>> { using Type = First; };
#endif

    // Hopefully we'll get a replacement intrinsic for this one day...
    // See: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100157
    template <typename T, typename... Ts>
    constexpr uint32_t _FindUniqueTypeInPack()
    {
        constexpr uint32_t size = sizeof...(Ts);
        constexpr bool found[size] = { IsSame<T, Ts>... };
        uint32_t n = size;
        for (uint32_t i = 0; i < size; ++i)
        {
            if (found[i])
            {
                if (n < size)
                    return size;
                n = i;
            }
        }
        return n;
    }

    template <typename T, typename List>
    struct _TypeListIndex;

    template <typename T>
    struct _TypeListIndex<T, TypeList<>>
    {
        static constexpr uint32_t Value = 0;
    };

    template <typename T, typename... Ts>
    struct _TypeListIndex<T, TypeList<Ts...>>
    {
        static constexpr uint32_t Value = _FindUniqueTypeInPack<T, Ts...>();
    };
    /// \endinternal

    template <uint32_t Index, typename List>
    using TypeListElement = typename _TypeListElement<Index, List>::Type;

    template <typename T, typename List>
    inline constexpr uint32_t TypeListIndex = _TypeListIndex<T, List>::Value;

    template <typename... Ls, typename... Rs>
    constexpr auto operator+(TypeList<Ls...>, TypeList<Rs...>) { return TypeList<Ls..., Rs...>{}; }
}
