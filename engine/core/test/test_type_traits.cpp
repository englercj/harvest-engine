// Copyright Chad Engler

#include "he/core/type_traits.h"

#include "he/core/string.h"
#include "he/core/test.h"
#include "he/core/vector.h"

#include <string>
#include <type_traits>
#include <vector>

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IntegralConst)
{
    using IC_Bool_True = IntegralConst<bool, true>;
    static_assert(std::is_same<IC_Bool_True::Type, bool>::value);
    static_assert(IC_Bool_True::Value == true);

    using IC_Int_6 = IntegralConst<int, 6>;
    static_assert(std::is_same<IC_Int_6::Type, int>::value);
    static_assert(IC_Int_6::Value == 6);

    using IC_Uint_ffffffffff = IntegralConst<uint64_t, 0xffffffffff>;
    static_assert(std::is_same<IC_Uint_ffffffffff::Type, uint64_t>::value);
    static_assert(IC_Uint_ffffffffff::Value == 0xffffffffff);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, BoolConst)
{
    static_assert(std::is_same<BoolConst<true>::Type, bool>::value);
    static_assert(BoolConst<true>::Value == true);

    static_assert(std::is_same<BoolConst<false>::Type, bool>::value);
    static_assert(BoolConst<false>::Value == false);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, TrueType)
{
    static_assert(std::is_same<TrueType::Type, bool>::value);
    static_assert(TrueType::Value == true);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, FalseType)
{
    static_assert(std::is_same<FalseType::Type, bool>::value);
    static_assert(FalseType::Value == false);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, EnableIf)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsTriviallyConstructible)
{
    struct TestCtor
    {
        TestCtor(int n) : v1(n), v2() {}
        TestCtor(int n, double f) noexcept : v1(n), v2(f) {}
        int v1;
        double v2;
    };

    static_assert(IsTriviallyConstructible<TestCtor, const TestCtor&>);
    static_assert(!IsTriviallyConstructible<TestCtor>);
    static_assert(!IsTriviallyConstructible<TestCtor, int>);
    static_assert(!IsTriviallyConstructible<TestCtor, int, double>);

    struct TestCtor2
    {
        int v1;
        double v2;
    };

    static_assert(IsTriviallyConstructible<TestCtor2, const TestCtor2&>);
    static_assert(IsTriviallyConstructible<TestCtor2>);
    static_assert(IsTriviallyConstructible<TestCtor2, int>);
    static_assert(IsTriviallyConstructible<TestCtor2, int, double>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsTriviallyDestructible)
{
    struct TestDtor
    {
        std::string str;
    };
    static_assert(!IsTriviallyDestructible<TestDtor>);

    struct TestDtor2
    {
        ~TestDtor2() = default;
    };
    static_assert(IsTriviallyDestructible<TestDtor2>);

    struct TestDtor3
    {
        int v1;
        double v2;
    };
    static_assert(IsTriviallyDestructible<TestDtor3>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsTriviallyCopyable)
{
    struct A { int m; };
    struct B { B(B const&) {} };
    struct C { virtual void foo() {} };

    struct D
    {
        int m;

        D(D const&) = default; // -> trivially copyable
        D(int x): m(x+1) {}
    };

    static_assert(IsTriviallyCopyable<A>);
    static_assert(!IsTriviallyCopyable<B>);
    static_assert(!IsTriviallyCopyable<C>);
    static_assert(IsTriviallyCopyable<D>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, RemoveReference)
{
    static_assert(std::is_same<int, RemoveReference<int&>>::value);
    static_assert(std::is_same<int, RemoveReference<int&&>>::value);

    static_assert(std::is_same<const int, RemoveReference<const int&>>::value);
    static_assert(std::is_same<const int, RemoveReference<const int&&>>::value);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, AddReference)
{
    static_assert(std::is_same<int&, AddReference<int>>::value);
    static_assert(std::is_same<const int&, AddReference<const int>>::value);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, AddRValueReference)
{
    static_assert(std::is_same<int&&, AddRValueReference<int>>::value);
    static_assert(std::is_same<const int&&, AddRValueReference<const int>>::value);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, RemovePointer)
{
    static_assert(std::is_same<int, RemovePointer<int*>>::value);
    static_assert(std::is_same<int, RemovePointer<int* const>>::value);
    static_assert(std::is_same<int, RemovePointer<int* volatile>>::value);
    static_assert(std::is_same<int, RemovePointer<int* const volatile>>::value);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, AddPointer)
{
    static_assert(std::is_same<int*, AddPointer<int>>::value);
    static_assert(std::is_same<const int*, AddPointer<const int>>::value);

    static_assert(std::is_same<int*, AddPointer<int&>>::value);
    static_assert(std::is_same<const int*, AddPointer<const int&>>::value);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, RemoveCV)
{
    static_assert(std::is_same<int, RemoveCV<int>>::value);
    static_assert(std::is_same<int, RemoveCV<const int>>::value);
    static_assert(std::is_same<int, RemoveCV<volatile int>>::value);
    static_assert(std::is_same<int, RemoveCV<const volatile int>>::value);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsConvertible)
{
    static_assert(IsConvertible<int*, int*>);
    static_assert(IsConvertible<uint32_t, size_t>);
    static_assert(!IsConvertible<int32_t*, uint32_t*>);
    static_assert(!IsConvertible<void*, uint32_t*>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsSame)
{
    static_assert(IsSame<int, int>);
    static_assert(!IsSame<int, bool>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, ProvidesStdContiguousRange)
{
    static_assert(ProvidesStdContiguousRange<std::string, char>);
    static_assert(ProvidesStdContiguousRange<std::vector<int>, int>);

    static_assert(!ProvidesStdContiguousRange<std::string, int>);
    static_assert(!ProvidesStdContiguousRange<std::vector<int>, char>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, ProvidesContiguousRange)
{
    static_assert(ProvidesContiguousRange<String, char>);
    static_assert(ProvidesContiguousRange<Vector<int>, int>);

    static_assert(!ProvidesContiguousRange<String, int>);
    static_assert(!ProvidesContiguousRange<Vector<int>, char>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsSpecialization)
{
    static_assert(IsSpecialization<std::vector<int>, std::vector>);
    static_assert(IsSpecialization<Vector<int>, Vector>);
    static_assert(!IsSpecialization<Vector<int>, std::vector>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, DeclVal)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, IsEnum)
{
    enum TestEnum { A };
    static_assert(IsEnum<TestEnum>);
    static_assert(!IsEnum<int>);
    static_assert(!IsEnum<Vector<int>>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, type_traits, EnumType)
{
    enum class TestEnumInt { A };
    enum class TestEnumUint : uint32_t { A };

    static_assert(std::is_same<EnumType<TestEnumInt>, int32_t>::value);
    static_assert(std::is_same<EnumType<TestEnumUint>, uint32_t>::value);

    static_assert(!std::is_same<EnumType<TestEnumInt>, bool>::value);
    static_assert(!std::is_same<EnumType<TestEnumUint>, int8_t>::value);
}
