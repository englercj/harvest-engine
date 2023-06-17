// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/concepts.h"

#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/test.h"
#include "he/core/vector.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, concepts, AllSame)
{
    static_assert(AllSame<int, int, int>);
    static_assert(AllSame<Trivial, Trivial, Trivial, Trivial, Trivial>);
    static_assert(!AllSame<int, NonTrivial, int>);
    static_assert(!AllSame<int, int, char>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, concepts, AnyOf)
{
    static_assert(AnyOf<int, char, bool, double, int, float>);
    static_assert(AnyOf<NonTrivial, char, bool, NonTrivial, int, Trivial>);
    static_assert(!AnyOf<int, char, bool, double, float>);
    static_assert(!AnyOf<NonTrivial, char, bool, int, Trivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, concepts, Arithmetic)
{
    static_assert(Arithmetic<bool>);
    static_assert(Arithmetic<char>);
    static_assert(Arithmetic<signed char>);
    static_assert(Arithmetic<unsigned char>);
    static_assert(Arithmetic<wchar_t>);
    static_assert(Arithmetic<char16_t>);
    static_assert(Arithmetic<char32_t>);
    static_assert(Arithmetic<short>);
    static_assert(Arithmetic<unsigned short>);
    static_assert(Arithmetic<int>);
    static_assert(Arithmetic<unsigned int>);
    static_assert(Arithmetic<long>);
    static_assert(Arithmetic<unsigned long>);
    static_assert(Arithmetic<long long>);
    static_assert(Arithmetic<unsigned long long>);
    static_assert(Arithmetic<float>);
    static_assert(Arithmetic<double>);
    static_assert(Arithmetic<long double>);
    static_assert(!Arithmetic<String>);
    static_assert(!Arithmetic<StringView>);
    static_assert(!Arithmetic<Trivial>);
    static_assert(!Arithmetic<NonTrivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, concepts, ConvertibleTo)
{
    static_assert(ConvertibleTo<void, void>);
    static_assert(ConvertibleTo<int, int>);
    static_assert(ConvertibleTo<int, const int&>);
    static_assert(ConvertibleTo<int, unsigned int>);
    static_assert(ConvertibleTo<int, bool>);
    static_assert(!ConvertibleTo<int, void>);
    static_assert(!ConvertibleTo<int, int&>);
    static_assert(!ConvertibleTo<int, const int*>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, concepts, DerivedFrom)
{
    struct A {};
    struct B : A {};
    struct C : B {};
    struct _1 {};
    struct D : C, _1 {};

    static_assert(DerivedFrom<A, A>);
    static_assert(DerivedFrom<B, A>);
    static_assert(DerivedFrom<B, B>);
    static_assert(DerivedFrom<C, A>);
    static_assert(DerivedFrom<C, B>);
    static_assert(DerivedFrom<C, C>);
    static_assert(DerivedFrom<D, A>);
    static_assert(DerivedFrom<D, B>);
    static_assert(DerivedFrom<D, C>);
    static_assert(DerivedFrom<D, D>);
    static_assert(DerivedFrom<D, _1>);
    static_assert(!DerivedFrom<_1, A>);
    static_assert(!DerivedFrom<C, _1>);
    static_assert(!DerivedFrom<void, void>);
    static_assert(!DerivedFrom<int, int>);
    static_assert(!DerivedFrom<void, int>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, concepts, Enum)
{
    enum UnscopedEnum { A, B, C };
    enum UnscopedTypedEnum : uint64_t { D, E, F };
    enum class ScopedEnum { A, B, C };
    enum class ScopedTypedEnum : uint64_t { A, B, C };

    static_assert(Enum<UnscopedEnum>);
    static_assert(Enum<UnscopedTypedEnum>);
    static_assert(Enum<ScopedEnum>);
    static_assert(Enum<ScopedTypedEnum>);
    static_assert(!Enum<bool>);
    static_assert(!Enum<uint32_t>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, concepts, FloatingPoint)
{
    static_assert(FloatingPoint<float>);
    static_assert(FloatingPoint<double>);
    static_assert(!FloatingPoint<nullptr_t>);
    static_assert(!FloatingPoint<void>);
    static_assert(!FloatingPoint<int>);
    static_assert(!FloatingPoint<bool>);
    static_assert(!FloatingPoint<Trivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, concepts, Integral)
{
    static_assert(Integral<int>);
    static_assert(Integral<char>);
    static_assert(Integral<bool>);
    static_assert(Integral<unsigned int>);
    static_assert(!Integral<nullptr_t>);
    static_assert(!Integral<void>);
    static_assert(!Integral<Trivial>);
    static_assert(!Integral<NonTrivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, concepts, Pointer)
{
    static_assert(Pointer<char*>);
    static_assert(Pointer<const char*>);
    static_assert(Pointer<decltype(&TestAllocator)>);
    static_assert(!Pointer<void>);
    static_assert(!Pointer<int>);
    static_assert(!Pointer<Trivial>);
    static_assert(!Pointer<NonTrivial>);
    static_assert(!Pointer<nullptr_t>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, concepts, SameAs)
{
    static_assert(SameAs<void, void>);
    static_assert(SameAs<int, int>);
    static_assert(SameAs<Trivial, Trivial>);
    static_assert(SameAs<NonTrivial, NonTrivial>);
    static_assert(!SameAs<int, void>);
    static_assert(!SameAs<void, int>);
    static_assert(!SameAs<Trivial, NonTrivial>);
    static_assert(!SameAs<NonTrivial, Trivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, concepts, SignedIntegral)
{
    static_assert(SignedIntegral<char>);
    static_assert(SignedIntegral<signed char>);
    static_assert(SignedIntegral<short>);
    static_assert(SignedIntegral<int>);
    static_assert(SignedIntegral<long>);
    static_assert(SignedIntegral<long long>);
    static_assert(!SignedIntegral<void>);
    static_assert(!SignedIntegral<void*>);
    static_assert(!SignedIntegral<unsigned char>);
    static_assert(!SignedIntegral<unsigned short>);
    static_assert(!SignedIntegral<unsigned int>);
    static_assert(!SignedIntegral<unsigned long>);
    static_assert(!SignedIntegral<unsigned long long>);
    static_assert(!SignedIntegral<Trivial>);
    static_assert(!SignedIntegral<NonTrivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, concepts, UnsignedIntegral)
{
    static_assert(UnsignedIntegral<unsigned char>);
    static_assert(UnsignedIntegral<unsigned short>);
    static_assert(UnsignedIntegral<unsigned int>);
    static_assert(UnsignedIntegral<unsigned long>);
    static_assert(UnsignedIntegral<unsigned long long>);
    static_assert(!UnsignedIntegral<char>);
    static_assert(!UnsignedIntegral<signed char>);
    static_assert(!UnsignedIntegral<short>);
    static_assert(!UnsignedIntegral<int>);
    static_assert(!UnsignedIntegral<long>);
    static_assert(!UnsignedIntegral<long long>);
    static_assert(!UnsignedIntegral<void>);
    static_assert(!UnsignedIntegral<void*>);
    static_assert(!UnsignedIntegral<Trivial>);
    static_assert(!UnsignedIntegral<NonTrivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, concepts, EqualityComparableWith)
{
    struct EqInt { bool operator==(const int&) const; };
    struct EqOnly { bool operator==(const EqOnly&) const; };

    static_assert(EqualityComparableWith<int, int>);
    static_assert(EqualityComparableWith<int, short>);
    static_assert(EqualityComparableWith<int, unsigned>);
    static_assert(EqualityComparableWith<EqOnly, EqOnly>);
    static_assert(EqualityComparableWith<EqInt, int>);
    static_assert(!EqualityComparableWith<void, void>);
    static_assert(!EqualityComparableWith<int, Trivial>);
    static_assert(!EqualityComparableWith<int, NonTrivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, concepts, EqualityComparable)
{
    struct EqOnly { bool operator==(const EqOnly&) const; };
    struct EqAndNe { bool operator==(const EqAndNe&) const; bool operator!=(const EqAndNe&) const; };

    static_assert(EqualityComparable<int>);
    static_assert(EqualityComparable<unsigned>);
    static_assert(EqualityComparable<EqAndNe>);
    static_assert(EqualityComparable<EqOnly>);
    static_assert(!EqualityComparable<void>);
    static_assert(!EqualityComparable<Trivial>);
    static_assert(!EqualityComparable<NonTrivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, concepts, ContiguousRangeOf)
{
    static_assert(ContiguousRangeOf<String, char>);
    static_assert(ContiguousRangeOf<StringView, const char>);
    static_assert(ContiguousRangeOf<Vector<int>, int>);
    static_assert(!ContiguousRangeOf<String, int>);
    static_assert(!ContiguousRangeOf<StringView, char>);
    static_assert(!ContiguousRangeOf<Vector<int>, char>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, concepts, ArithmeticRange)
{
    static_assert(ArithmeticRange<String>);
    static_assert(ArithmeticRange<StringView>);
    static_assert(ArithmeticRange<Vector<int>>);
    static_assert(!ArithmeticRange<Vector<Trivial>>);
    static_assert(!ArithmeticRange<int>);
    static_assert(!ArithmeticRange<Vector<Trivial>>);
    static_assert(!ArithmeticRange<Vector<NonTrivial>>);
}
