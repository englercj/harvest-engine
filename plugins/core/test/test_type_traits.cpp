// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/type_traits.h"

#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/test.h"
#include "he/core/vector.h"

#include <string>
#include <string_view>
#include <vector>

using namespace he;

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
HE_TEST(core, type_traits, IsSpecialization)
{
    static_assert(IsSpecialization<std::vector<int>, std::vector>);
    static_assert(IsSpecialization<Vector<int>, Vector>);
    static_assert(!IsSpecialization<Vector<int>, std::vector>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, ArrayElementType)
{
    static_assert(std::is_same_v<ArrayElementType<int[]>, int>);
    static_assert(std::is_same_v<ArrayElementType<int[5]>, int>);

    static_assert(std::is_same_v<ArrayElementType<NonTrivial[]>, NonTrivial>);
    static_assert(std::is_same_v<ArrayElementType<NonTrivial[32]>, NonTrivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, RemoveRValueReference)
{
    static_assert(std::is_same_v<RemoveRValueReference<int>, int>);
    static_assert(std::is_same_v<RemoveRValueReference<int&>, int&>);
    static_assert(std::is_same_v<RemoveRValueReference<int&&>, int>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, ConstnessAs)
{
    static_assert(std::is_const_v<ConstnessAs<char, const int>>);
    static_assert(!std::is_const_v<ConstnessAs<char, int>>);

    static_assert(std::is_const_v<ConstnessAs<NonTrivial, const Trivial>>);
    static_assert(!std::is_const_v<ConstnessAs<NonTrivial, Trivial>>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, TypeList)
{
    static_assert(TypeList<int, char, double>::Size == 3);
    static_assert(std::is_same_v<TypeListElement<0, TypeList<int, char, double>>, int>);
    static_assert(std::is_same_v<TypeListElement<1, TypeList<int, char, double>>, char>);
    static_assert(std::is_same_v<TypeListElement<2, TypeList<int, char, double>>, double>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, Arithmetic)
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

    static_assert(!Arithmetic<std::string>);
    static_assert(!Arithmetic<std::vector<int>>);
    static_assert(!Arithmetic<String>);
    static_assert(!Arithmetic<StringView>);
    static_assert(!Arithmetic<Trivial>);
    static_assert(!Arithmetic<NonTrivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, Enum)
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
HE_TEST(core, type_traits, AllSame)
{
    static_assert(AllSame<int, int, int>);
    static_assert(AllSame<Trivial, Trivial, Trivial, Trivial, Trivial>);
    static_assert(!AllSame<int, NonTrivial, int>);
    static_assert(!AllSame<int, int, char>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, AnyOf)
{
    static_assert(AnyOf<int, char, bool, double, int, float>);
    static_assert(!AnyOf<int, char, bool, double, float>);

    static_assert(AnyOf<NonTrivial, char, bool, NonTrivial, int, Trivial>);
    static_assert(!AnyOf<NonTrivial, char, bool, int, Trivial>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, ContiguousRange)
{
    static_assert(ContiguousRange<String, char>);
    static_assert(ContiguousRange<StringView, const char>);
    static_assert(ContiguousRange<Vector<int>, int>);

    static_assert(!ContiguousRange<String, int>);
    static_assert(!ContiguousRange<StringView, char>);
    static_assert(!ContiguousRange<Vector<int>, char>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, StdContiguousRange)
{
    static_assert(StdContiguousRange<std::string, const char>);
    static_assert(StdContiguousRange<std::string_view, const char>);
    static_assert(StdContiguousRange<std::vector<int>, int>);

    static_assert(!StdContiguousRange<std::string, int>);
    static_assert(!StdContiguousRange<std::string_view, char>);
    static_assert(!StdContiguousRange<std::vector<int>, char>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, ArithmeticRange)
{
    static_assert(ArithmeticRange<String>);
    static_assert(ArithmeticRange<StringView>);
    static_assert(ArithmeticRange<Vector<int>>);
    static_assert(!ArithmeticRange<Vector<Trivial>>);

    static_assert(!StdArithmeticRange<String>);
    static_assert(!StdArithmeticRange<StringView>);
    static_assert(!StdArithmeticRange<Vector<int>>);
    static_assert(!StdArithmeticRange<Vector<Trivial>>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, StdArithmeticRange)
{
    static_assert(StdArithmeticRange<std::string>);
    static_assert(StdArithmeticRange<std::string_view>);
    static_assert(StdArithmeticRange<std::vector<int>>);
    static_assert(!StdArithmeticRange<std::vector<Trivial>>);

    static_assert(!ArithmeticRange<std::string>);
    static_assert(!ArithmeticRange<std::string_view>);
    static_assert(!ArithmeticRange<std::vector<int>>);
    static_assert(!ArithmeticRange<std::vector<Trivial>>);
}
