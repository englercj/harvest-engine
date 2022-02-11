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
