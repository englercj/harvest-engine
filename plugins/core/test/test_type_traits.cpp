// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/type_traits.h"

#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/test.h"
#include "he/core/vector.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
// Utility types
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, AlwaysFalse)
{
    static_assert(AlwaysFalse<int> == false);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, Void)
{
    static_assert(IsSame<Void<int>, void>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IntegralConstant)
{
    static_assert(IntegralConstant<int, 1>::Value == 1);
    static_assert(IntegralConstant<bool, true>::Value == true);
    static_assert(IntegralConstant<ptrdiff_t, 123456789012356>::Value == 123456789012356);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, BoolConstant)
{
    static_assert(BoolConstant<true>::Value == true);
    static_assert(BoolConstant<false>::Value == false);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, TrueType)
{
    static_assert(TrueType::Value == true);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, FalseType)
{
    static_assert(FalseType::Value == false);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, Conditional)
{
    static_assert(Conditional<true, FalseType, TrueType>::Value == false);
    static_assert(Conditional<false, FalseType, TrueType>::Value == true);
    static_assert(Conditional<true, IntegralConstant<int, 5>, IntegralConstant<unsigned long long, 123456789012345ull>>::Value == 5);
    static_assert(Conditional<false, IntegralConstant<int, 5>, IntegralConstant<unsigned long long, 123456789012345ull>>::Value == 123456789012345ull);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, EnableIf)
{
    static_assert(IsSame<EnableIf<true>, void>);
    static_assert(IsSame<EnableIf<true, int>, int>);
    static_assert(IsSame<EnableIf<true, double>, double>);

    // Does not compile when false
    // static_assert(IsSame<EnableIf<false, int>, int>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, Conjunction)
{
    static_assert(Conjunction<BoolConstant<true>, TrueType>);
    static_assert(Conjunction<BoolConstant<IsVoid<void>>, BoolConstant<!IsVoid<int>>>);
    static_assert(!Conjunction<BoolConstant<true>, FalseType>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, Disjunction)
{
    static_assert(Disjunction<BoolConstant<true>, TrueType>);
    static_assert(Disjunction<BoolConstant<IsVoid<void>>, BoolConstant<!IsVoid<int>>>);
    static_assert(Disjunction<BoolConstant<true>, FalseType>);
    static_assert(!Disjunction<BoolConstant<false>, FalseType>);
}

// ------------------------------------------------------------------------------------------------
// Type modifiers
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, RemoveCV)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, RemoveConst)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, RemoveVolatile)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, RemoveReference)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, RemoveRValueReference)
{
    static_assert(IsSame<RemoveRValueReference<int>, int>);
    static_assert(IsSame<RemoveRValueReference<int&>, int&>);
    static_assert(IsSame<RemoveRValueReference<int&&>, int>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, AddLValueReference)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, AddRValueReference)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, RemovePointer)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, AddPointer)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, RemoveExtent)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, RemoveAllExtents)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IdentityType)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, RemoveCVRef)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, Decay)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, UnderlyingType)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, ConstnessAs)
{
    static_assert(IsConst<ConstnessAs<char, const int>>);
    static_assert(!IsConst<ConstnessAs<char, int>>);

    static_assert(IsConst<ConstnessAs<NonTrivial, const Trivial>>);
    static_assert(!IsConst<ConstnessAs<NonTrivial, Trivial>>);
}

// ------------------------------------------------------------------------------------------------
// Type relationships
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsSame)
{
    static_assert(IsSame<void, void>);
    static_assert(IsSame<int, int>);
    static_assert(IsSame<Trivial, Trivial>);
    static_assert(IsSame<NonTrivial, NonTrivial>);
    static_assert(!IsSame<int, void>);
    static_assert(!IsSame<void, int>);
    static_assert(!IsSame<Trivial, NonTrivial>);
    static_assert(!IsSame<NonTrivial, Trivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsAllSame)
{
    static_assert(IsAllSame<int, int, int>);
    static_assert(IsAllSame<Trivial, Trivial, Trivial, Trivial, Trivial>);
    static_assert(!IsAllSame<int, NonTrivial, int>);
    static_assert(!IsAllSame<int, int, char>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsAnyOf)
{
    static_assert(IsAnyOf<int, char, bool, double, int, float>);
    static_assert(IsAnyOf<NonTrivial, char, bool, NonTrivial, int, Trivial>);
    static_assert(!IsAnyOf<int, char, bool, double, float>);
    static_assert(!IsAnyOf<NonTrivial, char, bool, int, Trivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsBaseOf)
{
    struct A {};
    struct B : A {};
    struct C : B {};
    struct _1 {};
    struct D : C, _1 {};

    static_assert(IsBaseOf<A, A>);
    static_assert(IsBaseOf<A, B>);
    static_assert(IsBaseOf<B, B>);
    static_assert(IsBaseOf<A, C>);
    static_assert(IsBaseOf<B, C>);
    static_assert(IsBaseOf<C, C>);
    static_assert(IsBaseOf<A, D>);
    static_assert(IsBaseOf<B, D>);
    static_assert(IsBaseOf<C, D>);
    static_assert(IsBaseOf<D, D>);
    static_assert(IsBaseOf<_1, D>);
    static_assert(!IsBaseOf<A, _1>);
    static_assert(!IsBaseOf<_1, C>);
    static_assert(!IsBaseOf<void, void>);
    static_assert(!IsBaseOf<int, int>);
    static_assert(!IsBaseOf<int, void>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsConvertible)
{
    static_assert(IsConvertible<void, void>);
    static_assert(IsConvertible<int, int>);
    static_assert(IsConvertible<int, const int&>);
    static_assert(IsConvertible<int, unsigned int>);
    static_assert(IsConvertible<int, bool>);
    static_assert(IsConvertible<int*, void*>);
    static_assert(IsConvertible<int*, const void*>);
    static_assert(!IsConvertible<int, void>);
    static_assert(!IsConvertible<int, int&>);
    static_assert(!IsConvertible<int, const int*>);
    static_assert(!IsConvertible<const int*, void*>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsSpecialization)
{
    static_assert(IsSpecialization<Vector<int>, Vector>);
    static_assert(!IsSpecialization<Vector<int>, Span>);
}

// ------------------------------------------------------------------------------------------------
// Type categories
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsVoid)
{
    static_assert(IsVoid<void>);
    static_assert(!IsVoid<nullptr_t>);
    static_assert(!IsVoid<int>);
    static_assert(!IsVoid<Trivial>);
    static_assert(!IsVoid<NonTrivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsNullptr)
{
    static_assert(IsNullptr<nullptr_t>);
    static_assert(!IsNullptr<void>);
    static_assert(!IsNullptr<int>);
    static_assert(!IsNullptr<Trivial>);
    static_assert(!IsNullptr<NonTrivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsIntegral)
{
    static_assert(IsIntegral<int>);
    static_assert(IsIntegral<char>);
    static_assert(IsIntegral<bool>);
    static_assert(IsIntegral<unsigned int>);
    static_assert(!IsIntegral<nullptr_t>);
    static_assert(!IsIntegral<void>);
    static_assert(!IsIntegral<Trivial>);
    static_assert(!IsIntegral<NonTrivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsFloatingPoint)
{
    static_assert(IsFloatingPoint<float>);
    static_assert(IsFloatingPoint<double>);
    static_assert(!IsFloatingPoint<nullptr_t>);
    static_assert(!IsFloatingPoint<void>);
    static_assert(!IsFloatingPoint<int>);
    static_assert(!IsFloatingPoint<bool>);
    static_assert(!IsFloatingPoint<Trivial>);
    static_assert(!IsFloatingPoint<NonTrivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsArray)
{
    static_assert(!IsArray<void>);
    static_assert(!IsArray<char>);
    static_assert(IsArray<char[]>);
    static_assert(IsArray<char[5]>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsEnum)
{
    enum UnscopedEnum { A, B, C };
    enum UnscopedTypedEnum : uint64_t { D, E, F };
    enum class ScopedEnum { A, B, C };
    enum class ScopedTypedEnum : uint64_t { A, B, C };

    static_assert(IsEnum<UnscopedEnum>);
    static_assert(IsEnum<UnscopedTypedEnum>);
    static_assert(IsEnum<ScopedEnum>);
    static_assert(IsEnum<ScopedTypedEnum>);
    static_assert(!IsEnum<void>);
    static_assert(!IsEnum<bool>);
    static_assert(!IsEnum<uint32_t>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsClass)
{
    enum class ScopedEnum { A, B, C };

    static_assert(IsClass<Trivial>);
    static_assert(IsClass<NonTrivial>);
    static_assert(!IsClass<void>);
    static_assert(!IsClass<int>);
    static_assert(!IsClass<ScopedEnum>);
    static_assert(!IsClass<nullptr_t>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsPointer)
{
    static_assert(IsPointer<char*>);
    static_assert(IsPointer<const char*>);
    static_assert(IsPointer<decltype(&TestAllocator)>);
    static_assert(!IsPointer<void>);
    static_assert(!IsPointer<int>);
    static_assert(!IsPointer<Trivial>);
    static_assert(!IsPointer<NonTrivial>);
    static_assert(!IsPointer<nullptr_t>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsLValueReference)
{
    static_assert(IsLValueReference<char&>);
    static_assert(IsLValueReference<NonTrivial&>);
    static_assert(!IsLValueReference<NonTrivial&&>);
    static_assert(!IsLValueReference<void>);
    static_assert(!IsLValueReference<int>);
    static_assert(!IsLValueReference<Trivial>);
    static_assert(!IsLValueReference<NonTrivial>);
    static_assert(!IsLValueReference<nullptr_t>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsRValueReference)
{
    static_assert(IsRValueReference<NonTrivial&&>);
    static_assert(!IsRValueReference<char&>);
    static_assert(!IsRValueReference<NonTrivial&>);
    static_assert(!IsRValueReference<void>);
    static_assert(!IsRValueReference<int>);
    static_assert(!IsRValueReference<Trivial>);
    static_assert(!IsRValueReference<NonTrivial>);
    static_assert(!IsRValueReference<nullptr_t>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsReference)
{
    static_assert(IsReference<char&>);
    static_assert(IsReference<NonTrivial&>);
    static_assert(IsReference<NonTrivial&&>);
    static_assert(!IsReference<void>);
    static_assert(!IsReference<int>);
    static_assert(!IsReference<Trivial>);
    static_assert(!IsReference<NonTrivial>);
    static_assert(!IsReference<nullptr_t>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsFunction)
{
    struct TestFunctor { void operator()(); };

    static_assert(IsFunction<decltype(TestAllocator)>);
    static_assert(!IsFunction<TestFunctor>);
    static_assert(!IsFunction<void>);
    static_assert(!IsFunction<int>);
    static_assert(!IsFunction<Trivial>);
    static_assert(!IsFunction<NonTrivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsMemberFunctionPointer)
{
    struct TestFunctor { void operator()(); void Tester(); int value; };
    static_assert(IsMemberFunctionPointer<decltype(&TestFunctor::operator())>);
    static_assert(IsMemberFunctionPointer<decltype(&TestFunctor::Tester)>);
    static_assert(!IsMemberFunctionPointer<decltype(&TestFunctor::value)>);
    static_assert(!IsMemberFunctionPointer<decltype(TestAllocator)>);
    static_assert(!IsMemberFunctionPointer<TestFunctor>);
    static_assert(!IsMemberFunctionPointer<void>);
    static_assert(!IsMemberFunctionPointer<int>);
    static_assert(!IsMemberFunctionPointer<Trivial>);
    static_assert(!IsMemberFunctionPointer<NonTrivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsMemberObjectPointer)
{
    struct TestFunctor { void operator()(); void Tester(); int value; };
    static_assert(IsMemberObjectPointer<decltype(&TestFunctor::value)>);
    static_assert(!IsMemberObjectPointer<decltype(&TestFunctor::operator())>);
    static_assert(!IsMemberObjectPointer<decltype(&TestFunctor::Tester)>);
    static_assert(!IsMemberObjectPointer<decltype(TestAllocator)>);
    static_assert(!IsMemberObjectPointer<TestFunctor>);
    static_assert(!IsMemberObjectPointer<void>);
    static_assert(!IsMemberObjectPointer<int>);
    static_assert(!IsMemberObjectPointer<Trivial>);
    static_assert(!IsMemberObjectPointer<NonTrivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsMemberPointer)
{
    struct TestFunctor { void operator()(); void Tester(); int value; };
    static_assert(IsMemberPointer<decltype(&TestFunctor::value)>);
    static_assert(IsMemberPointer<decltype(&TestFunctor::operator())>);
    static_assert(IsMemberPointer<decltype(&TestFunctor::Tester)>);
    static_assert(!IsMemberPointer<decltype(TestAllocator)>);
    static_assert(!IsMemberPointer<TestFunctor>);
    static_assert(!IsMemberPointer<void>);
    static_assert(!IsMemberPointer<int>);
    static_assert(!IsMemberPointer<Trivial>);
    static_assert(!IsMemberPointer<NonTrivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsArithmetic)
{
    static_assert(IsArithmetic<bool>);
    static_assert(IsArithmetic<char>);
    static_assert(IsArithmetic<int>);
    static_assert(IsArithmetic<float>);
    static_assert(IsArithmetic<ptrdiff_t>);
    static_assert(!IsArithmetic<void>);
    static_assert(!IsArithmetic<Trivial>);
    static_assert(!IsArithmetic<NonTrivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsScalar)
{
    static_assert(IsScalar<bool>);
    static_assert(IsScalar<char>);
    static_assert(IsScalar<int>);
    static_assert(IsScalar<float>);
    static_assert(IsScalar<ptrdiff_t>);
    static_assert(!IsScalar<void>);
    static_assert(!IsScalar<Trivial>);
    static_assert(!IsScalar<NonTrivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsObject)
{
    static_assert(IsObject<int>);
    static_assert(IsObject<float>);
    static_assert(IsObject<nullptr_t>);
    static_assert(IsObject<Trivial>);
    static_assert(IsObject<NonTrivial>);
    static_assert(!IsObject<void>);
    static_assert(!IsObject<int&>);
    static_assert(!IsObject<decltype(TestAllocator)>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsFundamental)
{
    static_assert(IsFundamental<int>);
    static_assert(IsFundamental<float>);
    static_assert(IsFundamental<void>);
    static_assert(IsFundamental<nullptr_t>);
    static_assert(!IsFundamental<int&>);
    static_assert(!IsFundamental<decltype(TestAllocator)>);
    static_assert(!IsFundamental<Trivial>);
    static_assert(!IsFundamental<NonTrivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsCompound)
{
    struct TestFunctor { void operator()(); void Tester(); int value; };
    union TestUnion { int a; Trivial b; };
    enum UnscopedEnum { A, B, C };
    enum class ScopedEnum { A, B, C };

    static_assert(IsCompound<const int&>);
    static_assert(IsCompound<int&>);
    static_assert(IsCompound<int&&>);
    static_assert(IsCompound<char[]>);
    static_assert(IsCompound<char[5]>);
    static_assert(IsCompound<UnscopedEnum>);
    static_assert(IsCompound<ScopedEnum>);
    static_assert(IsCompound<TestUnion>);
    static_assert(IsCompound<TestFunctor>);
    static_assert(IsCompound<decltype(TestFunctor::operator())>);
    static_assert(IsCompound<decltype(&TestFunctor::operator())>);
    static_assert(IsCompound<decltype(TestFunctor::Tester)>);
    static_assert(IsCompound<decltype(&TestFunctor::Tester)>);
    static_assert(IsCompound<decltype(&TestFunctor::value)>);
    static_assert(IsCompound<Trivial>);
    static_assert(IsCompound<NonTrivial>);
    static_assert(!IsCompound<decltype(TestFunctor::value)>);
    static_assert(!IsCompound<void>);
    static_assert(!IsCompound<int>);
}

// ------------------------------------------------------------------------------------------------
// Type properties
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsConst)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsVolatile)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsTrivial)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsTriviallyCopyable)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsStandardLayout)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsEmpty)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsPolymorphic)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsAbstract)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsFinal)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsAggregate)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsSigned)
{
    static_assert(IsSigned<char>);
    static_assert(IsSigned<signed char>);
    static_assert(IsSigned<short>);
    static_assert(IsSigned<int>);
    static_assert(IsSigned<long>);
    static_assert(IsSigned<long long>);
    static_assert(!IsSigned<void>);
    static_assert(!IsSigned<void*>);
    static_assert(!IsSigned<unsigned char>);
    static_assert(!IsSigned<unsigned short>);
    static_assert(!IsSigned<unsigned int>);
    static_assert(!IsSigned<unsigned long>);
    static_assert(!IsSigned<unsigned long long>);
    static_assert(!IsSigned<Trivial>);
    static_assert(!IsSigned<NonTrivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsUnsigned)
{
    static_assert(IsUnsigned<unsigned char>);
    static_assert(IsUnsigned<unsigned short>);
    static_assert(IsUnsigned<unsigned int>);
    static_assert(IsUnsigned<unsigned long>);
    static_assert(IsUnsigned<unsigned long long>);
    static_assert(!IsUnsigned<char>);
    static_assert(!IsUnsigned<signed char>);
    static_assert(!IsUnsigned<short>);
    static_assert(!IsUnsigned<int>);
    static_assert(!IsUnsigned<long>);
    static_assert(!IsUnsigned<long long>);
    static_assert(!IsUnsigned<void>);
    static_assert(!IsUnsigned<void*>);
    static_assert(!IsUnsigned<Trivial>);
    static_assert(!IsUnsigned<NonTrivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, Rank)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, Extent)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, MemberPointerObjectType)
{
}

// ------------------------------------------------------------------------------------------------
// Supported operations
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsConstructible)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsDefaultConstructible)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsCopyConstructible)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsMoveConstructible)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsAssignable)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsCopyAssignable)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsMoveAssignable)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsDestructible)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsTriviallyConstructible)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsTriviallyDefaultConstructible)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsTriviallyCopyConstructible)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsTriviallyMoveConstructible)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsTriviallyAssignable)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsTriviallyCopyAssignable)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsTriviallyMoveAssignable)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsTriviallyDestructible)
{
}

// ------------------------------------------------------------------------------------------------
// Integer sequences
// ------------------------------------------------------------------------------------------------

template <typename T, EnableIf<IsSame<MakeIntegerSequence<T, 0>, IntegerSequence<long long>>>* = nullptr>
constexpr bool TestMakeIntegerSequence(int) { return true; }

template <typename>
constexpr bool TestMakeIntegerSequence(long) { return false; }

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IntegerSequence)
{
    static_assert(IsSame<int, typename IntegerSequence<int, 0>::Type>);
    static_assert(IntegerSequence<int, 1, 2, 3, 4, 5>::Size == 5);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, MakeIntegerSequence)
{
    static_assert(TestMakeIntegerSequence<long long>(0));
    static_assert(!TestMakeIntegerSequence<char>(0));

    static_assert(MakeIntegerSequence<char, 10>::Size == 10);
    static_assert(IsSame<char, typename MakeIntegerSequence<char, 10>::Type>);

    static_assert(MakeIntegerSequence<int, 100>::Size == 100);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IndexSequence)
{
    static_assert(IsSame<uint32_t, typename IndexSequence<0>::Type>);
    static_assert(IndexSequence<1, 2, 3, 4, 5>::Size == 5);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, MakeIndexSequence)
{
    static_assert(MakeIndexSequence<10>::Size == 10);
    static_assert(IsSame<uint32_t, typename MakeIndexSequence<10>::Type>);
    static_assert(MakeIndexSequence<100>::Size == 100);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IndexSequenceFor)
{
    static_assert(IsSame<uint32_t, typename IndexSequenceFor<int, float>::Type>);
    static_assert(IndexSequenceFor<int, float, NonTrivial>::Size == 3);
}

// ------------------------------------------------------------------------------------------------
// Metaprogramming functions
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsConstantEvaluated)
{
    static_assert(IsConstantEvaluated());
    HE_EXPECT(!IsConstantEvaluated());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, DeclVal)
{
    static_assert(IsSame<void, decltype(DeclVal<void>())>);
    static_assert(IsSame<int&&, decltype(DeclVal<int>())>);
    static_assert(IsSame<int&, decltype(DeclVal<int&>())>);
    static_assert(IsSame<int&&, decltype(DeclVal<int&&>())>);
    static_assert(IsSame<const NonTrivial*&&, decltype(DeclVal<const NonTrivial*>())>);
}

// ------------------------------------------------------------------------------------------------
// Aligned Storage
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, AlignedStorage)
{
    using T0 = AlignedStorage<64, 16>;
    static_assert(sizeof(T0) == 64);
    static_assert(alignof(T0) == 16);

    using T1 = AlignedStorage<233, 256>;
    static_assert(sizeof(T1) == 256); // multiple of alignment
    static_assert(alignof(T1) == 256);

    using T2 = AlignedStorage<2049, 512>;
    static_assert(sizeof(T2) == 2560); // multiple of alignment
    static_assert(alignof(T2) == 512);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, AlignedStorageFor)
{
    using T0 = AlignedStorageFor<Trivial>;
    static_assert(sizeof(T0) == sizeof(Trivial));
    static_assert(alignof(T0) == alignof(Trivial));

    using T1 = AlignedStorageFor<NonTrivial>;
    static_assert(sizeof(T1) == sizeof(NonTrivial));
    static_assert(alignof(T1) == alignof(NonTrivial));

    struct alignas(512) OverAligned { char bigBuffer[1024]; };
    using T2 = AlignedStorageFor<OverAligned>;
    static_assert(sizeof(T2) == sizeof(OverAligned));
    static_assert(alignof(T2) == alignof(OverAligned));
}

// ------------------------------------------------------------------------------------------------
// Type List
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, TypeList)
{
    static_assert(TypeList<int, char, double>::Size == 3);
    static_assert(IsSame<TypeListElement<0, TypeList<int, char, double>>, int>);
    static_assert(IsSame<TypeListElement<1, TypeList<int, char, double>>, char>);
    static_assert(IsSame<TypeListElement<2, TypeList<int, char, double>>, double>);
}

// ------------------------------------------------------------------------------------------------
// Misc
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
template <typename T>
void TestFunctionType()
{
    static_assert(!IsVoid<T>);
    static_assert(!IsNullptr<T>);
    static_assert(!IsIntegral<T>);
    static_assert(!IsFloatingPoint<T>);
    static_assert(!IsArray<T>);
    static_assert(!IsPointer<T>);
    static_assert(!IsLValueReference<T>);
    static_assert(!IsRValueReference<T>);
    static_assert(!IsMemberObjectPointer<T>);
    static_assert(!IsMemberFunctionPointer<T>);
    static_assert(!IsEnum<T>);
    static_assert(!IsUnion<T>);
    static_assert(!IsClass<T>);
    static_assert(IsFunction<T>);

    // Composite type traits. Function types are compound, but not anything else.
    static_assert(!IsReference<T>);
    static_assert(!IsArithmetic<T>);
    static_assert(!IsFundamental<T>);
    static_assert(!IsObject<T>);
    static_assert(!IsScalar<T>);
    static_assert(IsCompound<T>);
    static_assert(!IsMemberPointer<T>);

    // Type properties, convenience forms. They're false for plain function types,
    // and they're required to be false for non-referenceable types.
    static_assert(!IsCopyConstructible<T>);
    static_assert(!IsMoveConstructible<T>);
    static_assert(!IsCopyAssignable<T>);
    static_assert(!IsMoveAssignable<T>);
    //static_assert(!IsSwappable<T>);
    static_assert(!IsTriviallyCopyConstructible<T>);
    static_assert(!IsTriviallyMoveConstructible<T>);
    static_assert(!IsTriviallyCopyAssignable<T>);
    static_assert(!IsTriviallyMoveAssignable<T>);
    //static_assert(!IsNothrowCopyConstructible<T>);
    //static_assert(!IsNothrowMoveConstructible<T>);
    //static_assert(!IsNothrowCopyAssignable<T>);
    //static_assert(!IsNothrowMoveAssignable<T>);
    //static_assert(!IsNothrowSwappable<T>);

    //static_assert(!HasUniqueObjectRepresentations<T>);

    static_assert(IsSame<RemoveCVRef<T>, T>);

    //static_assert(!IsBoundedArray<T>);
    //static_assert(!IsUnboundedArray<T>);
}

template <typename T>
void TestPlainFunctionType()
{
    TestFunctionType<T>();

    // These transformations work normally on plain function types.
    static_assert(IsSame<AddLValueReference<T>, T&>);
    static_assert(IsSame<AddRValueReference<T>, T&&>);
    static_assert(IsSame<AddPointer<T>, T*>);
    static_assert(IsSame<RemoveCVRef<T&>, T>);
    static_assert(IsSame<RemoveCVRef<T&&>, T>);
}

template <typename T>
void TestWeirdFunctionType()
{
    TestFunctionType<T>();

    // These transformations are required to leave non-referenceable function types unchanged.
    static_assert(IsSame<AddLValueReference<T>, T>);
    static_assert(IsSame<AddRValueReference<T>, T>);
    static_assert(IsSame<AddPointer<T>, T>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, function_types)
{
    TestPlainFunctionType<int(int)>();
    TestWeirdFunctionType<int(int) const>();
    TestWeirdFunctionType<int(int) volatile>();
    TestWeirdFunctionType<int(int) const volatile>();
    TestWeirdFunctionType<int(int)&>();
    TestWeirdFunctionType<int(int) const&>();
    TestWeirdFunctionType<int(int) volatile&>();
    TestWeirdFunctionType<int(int) const volatile&>();
    TestWeirdFunctionType<int(int)&&>();
    TestWeirdFunctionType<int(int) const&&>();
    TestWeirdFunctionType<int(int) volatile&&>();
    TestWeirdFunctionType<int(int) const volatile&&>();

    TestPlainFunctionType<int(int, ...)>();
    TestWeirdFunctionType<int(int, ...) const>();
    TestWeirdFunctionType<int(int, ...) volatile>();
    TestWeirdFunctionType<int(int, ...) const volatile>();
    TestWeirdFunctionType<int(int, ...)&>();
    TestWeirdFunctionType<int(int, ...) const&>();
    TestWeirdFunctionType<int(int, ...) volatile&>();
    TestWeirdFunctionType<int(int, ...) const volatile&>();
    TestWeirdFunctionType<int(int, ...)&&>();
    TestWeirdFunctionType<int(int, ...) const&&>();
    TestWeirdFunctionType<int(int, ...) volatile&&>();
    TestWeirdFunctionType<int(int, ...) const volatile&&>();
}
