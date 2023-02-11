// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/tuple.h"
#include "he/core/tuple_fmt.h"

#include "he/core/fmt.h"
#include "he/core/memory_ops.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/type_traits.h"
#include "he/core/test.h"
#include "he/core/unique_ptr.h"
#include "he/core/utils.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, tuple, Tuple)
{
    // Empty
    {
        using T = Tuple<>;
        static_assert(T::Size == 0);
        static_assert(IsEmpty<T>);
        static_assert(IsTrivial<T>);
        static_assert(IsTriviallyAssignable<T, T>);
        static_assert(IsTriviallyCopyAssignable<T>);
        static_assert(IsTriviallyMoveAssignable<T>);
        static_assert(IsSame<typename T::ElementList, TypeList<>>);
    }

    // Trivial
    {
        using T = Tuple<int, unsigned, Trivial>;
        static_assert(T::Size == 3);
        static_assert(IsSame<TupleElement<0, T>, int>);
        static_assert(IsSame<TupleElement<1, T>, unsigned>);
        static_assert(IsSame<TupleElement<2, T>, Trivial>);
        static_assert(!IsEmpty<T>);
        static_assert(IsTrivial<T>);
        static_assert(IsTriviallyAssignable<T, T>);
        static_assert(IsTriviallyCopyAssignable<T>);
        static_assert(IsTriviallyMoveAssignable<T>);
        static_assert(IsSame<typename T::ElementList, TypeList<int, unsigned, Trivial>>);
    }

    // Non-trivial
    {
        using T = Tuple<int, unsigned, NonTrivial>;
        static_assert(T::Size == 3);
        static_assert(IsSame<TupleElement<0, T>, int>);
        static_assert(IsSame<TupleElement<1, T>, unsigned>);
        static_assert(IsSame<TupleElement<2, T>, NonTrivial>);
        static_assert(!IsEmpty<T>);
        static_assert(!IsTrivial<T>);
        static_assert(!IsTriviallyAssignable<T, T>);
        static_assert(!IsTriviallyCopyAssignable<T>);
        static_assert(!IsTriviallyMoveAssignable<T>);
        static_assert(IsSame<typename T::ElementList, TypeList<int, unsigned, NonTrivial>>);
    }
}

template <class T>
uintptr_t PtrToUint(const T* pointer)
{
    uintptr_t dest{};
    static_assert(sizeof(const T*) == sizeof(dest));
    memcpy(&dest, &pointer, sizeof(const T*));
    return dest;
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, tuple, alignment)
{
    using A = AlignedStorage<40, 64>;
    using B = AlignedStorage<10, 16>;
    using C = AlignedStorage<15, 32>;
    using D = AlignedStorage<1, 16>;
    using E = AlignedStorage<13, 8>;

    using T = Tuple<A, B, C, D, E>;

    T t{};
    const uintptr_t baseAddr = PtrToUint(&t);
    size_t offset = 0;

    TupleForEach(t, [&](auto& entry)
    {
        using EntryType = Decay<decltype(entry)>;

        const uintptr_t addr = reinterpret_cast<uintptr_t>(&entry);
        const size_t entryOffset = addr - baseAddr;
        constexpr size_t entrySize = sizeof(EntryType);

        offset = AlignUp(offset, alignof(EntryType));

        HE_EXPECT_EQ(entryOffset, offset);
        offset += entrySize;
    });
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, tuple, operators_assignment)
{
    // Same types
    {
        Tuple<int, int, StringView> t{ 0, 0, StringView{} };
        HE_EXPECT_EQ(TupleGet<0>(t), 0);
        HE_EXPECT_EQ(TupleGet<1>(t), 0);
        HE_EXPECT_EQ(TupleGet<2>(t), ""_sv);

        t = Tuple{ 1, 2, "Hello!"_sv };
        HE_EXPECT_EQ(TupleGet<0>(t), 1);
        HE_EXPECT_EQ(TupleGet<1>(t), 2);
        HE_EXPECT_EQ(TupleGet<2>(t), "Hello!");
    }

    // Different types
    {
        Tuple t{ 0, 0, StringView{} };
        HE_EXPECT_EQ(TupleGet<0>(t), 0);
        HE_EXPECT_EQ(TupleGet<1>(t), 0);
        HE_EXPECT_EQ(TupleGet<2>(t), ""_sv);

        t = Tuple{ 1, 2, "const char*" };
        HE_EXPECT_EQ(TupleGet<0>(t), 1);
        HE_EXPECT_EQ(TupleGet<1>(t), 2);
        HE_EXPECT_EQ(TupleGet<2>(t), "const char*");
    }

    // Moves
    {
        Tuple<UniquePtr<int>, String> t;
        t = { MakeUnique<int>(5), "Hello!" };

        Tuple<UniquePtr<int>, String> t1;
        t1 = Move(t);

        HE_EXPECT(TupleGet<0>(t).Get() == nullptr);
        HE_EXPECT_EQ(*TupleGet<0>(t1), 5);
        HE_EXPECT_EQ(TupleGet<1>(t1), "Hello!");
    }

    // Empty
    {
        Tuple<> e;
        e = e;
    }

    // Nested
    {
        Tuple<int, Tuple<int>> t;
        t = { 1, MakeTuple(2) };
        HE_EXPECT_EQ(t, (Tuple{ 1, Tuple{ 2 } }));
    }

    // Deeply nested with mixed types
    {
        Tuple<int, Tuple<Tuple<double>, int>> t;
        t = { 1, MakeTuple(MakeTuple(2.0), 3) };

        HE_EXPECT_EQ(TupleGet<0>(t), 1);
        HE_EXPECT_EQ(TupleGet<0>(TupleGet<1>(t)), (Tuple{ 2.0 }));
        HE_EXPECT_EQ(TupleGet<1>(TupleGet<1>(t)), 3);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, tuple, operators_comparison)
{
    // constexpr
    {
        constexpr Tuple t0{ 10, 1 };
        constexpr Tuple t1{ 10, 2 };
        constexpr Tuple t2{ 10, 2ull };
        static_assert(t0 == t0);
        static_assert(t0 != t1);
        static_assert(t0 < t1);
        static_assert(t1 == t2);
        static_assert(t0 != t2);
        static_assert(t0 < t2);
    }

    // Mixed types
    {
        Tuple t0{ 1, 2 };
        Tuple t1{ '\1', '\2' };
        Tuple t2{ 1ll, 2.0 };

        HE_EXPECT(t0 == t0);
        HE_EXPECT(t0 == t1);
        HE_EXPECT(t0 == t2);
        HE_EXPECT(t1 == t0);
        HE_EXPECT(t1 == t1);
        HE_EXPECT(t1 == t2);
        HE_EXPECT(t2 == t0);
        HE_EXPECT(t2 == t1);
        HE_EXPECT(t2 == t2);
    }

    // lexicographic ordering
    {
        Tuple t0{ 0, 0 };
        Tuple t1{ 0, 1 };
        Tuple t2{ 1, 0 };

        HE_EXPECT(t0 == t0);
        HE_EXPECT(t0 < t1);
        HE_EXPECT(t1 < t2);
        HE_EXPECT(t0 < t2);
        HE_EXPECT(t1 > t0);
        HE_EXPECT(t2 > t1);
        HE_EXPECT(t2 > t0);

        HE_EXPECT(t0 <= t1);
        HE_EXPECT(t1 <= t2);
        HE_EXPECT(t2 >= t0);
        HE_EXPECT(t1 >= t0);
        HE_EXPECT(t2 >= t1);
    }

    // lexicographic ordering for strings
    {
        Tuple t0{ "0"_sv, "0"_sv };
        Tuple t1{ "0"_sv, "00"_sv };
        Tuple t2{ "00"_sv, "0"_sv };

        HE_EXPECT(t0 == t0);
        HE_EXPECT(t0 < t1);
        HE_EXPECT(t1 < t2);
        HE_EXPECT(t0 < t2);
        HE_EXPECT(t1 > t0);
        HE_EXPECT(t2 > t1);
        HE_EXPECT(t2 > t0);

        HE_EXPECT(t0 <= t1);
        HE_EXPECT(t1 <= t2);
        HE_EXPECT(t2 >= t0);
        HE_EXPECT(t1 >= t0);
        HE_EXPECT(t2 >= t1);
    }

    // lexicographic ordering for mixed types
    {
        Tuple t0{ 0, 0 };
        Tuple t1{ 0ll, 1 };
        Tuple t2{ 1.0, 0 };

        HE_EXPECT(t0 == t0);
        HE_EXPECT(t0 < t1);
        HE_EXPECT(t1 < t2);
        HE_EXPECT(t0 < t2);
        HE_EXPECT(t1 > t0);
        HE_EXPECT(t2 > t1);
        HE_EXPECT(t2 > t0);

        HE_EXPECT(t0 <= t1);
        HE_EXPECT(t1 <= t2);
        HE_EXPECT(t2 >= t0);
        HE_EXPECT(t1 >= t0);
        HE_EXPECT(t2 >= t1);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, tuple, TupleGet)
{
    constexpr Tuple t{ 1, 2, 3 };
    static_assert(TupleGet<0>(t) == 1);
    static_assert(TupleGet<1>(t) == 2);
    static_assert(TupleGet<2>(t) == 3);
    HE_EXPECT_EQ(TupleGet<0>(t), 1);
    HE_EXPECT_EQ(TupleGet<1>(t), 2);
    HE_EXPECT_EQ(TupleGet<2>(t), 3);

    constexpr Tuple t1{ "Hello!"_sv, 3.141592, 5ul };
    static_assert(TupleGet<0>(t1) == "Hello!"_sv);
    static_assert(TupleGet<1>(t1) == 3.141592);
    static_assert(TupleGet<2>(t1) == 5ul);
    static_assert(TupleGet<StringView>(t1) == "Hello!"_sv);
    static_assert(TupleGet<double>(t1) == 3.141592);
    static_assert(TupleGet<unsigned long>(t1) == 5ul);
    HE_EXPECT_EQ(TupleGet<0>(t1), "Hello!");
    HE_EXPECT_EQ(TupleGet<1>(t1), 3.141592);
    HE_EXPECT_EQ(TupleGet<2>(t1), 5ul);
    HE_EXPECT_EQ(TupleGet<StringView>(t1), "Hello!");
    HE_EXPECT_EQ(TupleGet<double>(t1), 3.141592);
    HE_EXPECT_EQ(TupleGet<unsigned long>(t1), 5ul);

    using T2 = Tuple<int, char, double, StringView>;
    T2 t2;
    static_assert(IsSame<decltype(TupleGet<int>(static_cast<const T2&>(t2))), const int&>);
    static_assert(IsSame<decltype(TupleGet<int>(static_cast<T2&>(t2))), int&>);
    static_assert(IsSame<decltype(TupleGet<int>(static_cast<T2&&>(t2))), int&&>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, tuple, MakeTuple)
{
    static_assert(IsSame<decltype(MakeTuple(1, 2.0, ""_sv)), Tuple<int, double, StringView>>);

    auto t0 = MakeTuple(1, 2.0, ""_sv);
    Tuple t1{ 1, 2.0, ""_sv };

    static_assert(IsSame<decltype(t0), decltype(t1)>);
    HE_EXPECT(t0 == t1);
    HE_EXPECT(t1 == t0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, tuple, ForwardAsTuple)
{
    int a = 10;
    char b = 'x';
    String c = "test";

    static_assert(IsSame<decltype(ForwardAsTuple(a, b, Move(c))), Tuple<int&, char&, String&&>>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, tuple, TupleForEach)
{
    // Basic
    {
        String result;

        Tuple t{ "Hello!", "test"_sv, 10, 'x', 20, 3.141592 };
        TupleForEach(t, [&](const auto& value)
        {
            FormatTo(result, "{}|", value);
        });

        HE_EXPECT_EQ(result, "Hello!|test|10|x|20|3.141592|");
    }

    // Moved tuple
    {
        Tuple t1{ MakeUnique<int>(10), MakeUnique<int>(20), MakeUnique<int>(30) };
        HE_EXPECT(TupleAll(t1, [](const auto& value) { return !!value; }));

        int sum = 0;
        TupleForEach(Move(t1), [&](auto value) { sum += *value; });

        // Should be empty now that the values were moved from
        HE_EXPECT(TupleAll(t1, [](const auto& value) { return !value; }));
        HE_EXPECT_EQ(sum, 60);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, tuple, TupleAny)
{
    // Empty
    {
        Tuple<> t;
        HE_EXPECT(!TupleAny(t, [](auto value) { return value < 10; }));
    }

    // Trivial
    {
        Tuple t{ 1, 2, 3, 4.3, 5 };
        HE_EXPECT(TupleAny(t, [](auto value) { return value < 10; }));
        HE_EXPECT(TupleAny(t, [](auto value) { return value < 4; }));
        HE_EXPECT(!TupleAny(t, [](auto value) { return value < 0; }));
    }

    // Move
    {
        Tuple t{ MakeUnique<int>(10), MakeUnique<int>(20), MakeUnique<int>(30) };
        HE_EXPECT(TupleAny(t, [](auto& value) { return !!value; }));
        HE_EXPECT(TupleAny(Move(t), [](auto value) { return !!value; }));
        HE_EXPECT(TupleAny(t, [](auto& value) { return !!value; }));
        HE_EXPECT(TupleAny(Move(t), [](auto value) { return !!value; }));
        HE_EXPECT(TupleAny(t, [](auto& value) { return !!value; }));
        HE_EXPECT(TupleAny(Move(t), [](auto value) { return !!value; }));
        HE_EXPECT(!TupleAny(t, [](auto& value) { return !!value; }));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, tuple, TupleAll)
{
    // Empty
    {
        Tuple<> t;
        HE_EXPECT(TupleAll(t, [](auto value) { return value < 10; }));
    }

    // Trivial
    {
        Tuple t{ 1, 2, 3, 4.3, 5 };
        HE_EXPECT(TupleAll(t, [](auto value) { return value < 10; }));
        HE_EXPECT(!TupleAll(t, [](auto value) { return value < 4; }));
    }

    // Move
    {
        Tuple t{ MakeUnique<int>(10), MakeUnique<int>(20), MakeUnique<int>(30) };
        HE_EXPECT(TupleAll(t, [](auto& value) { return !!value; }));
        HE_EXPECT(TupleAll(Move(t), [](auto value) { return !!value; }));
        HE_EXPECT(!TupleAll(t, [](auto& value) { return !!value; }));
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, tuple, TupleApply)
{
    // Empty
    {
        constexpr auto test = []() { return 0; };
        static_assert(TupleApply(Tuple<>{}, test) == 0);
        int result = TupleApply(Tuple<>{}, test);
        HE_EXPECT_EQ(result, 0);
    }

    // Trivial
    {
        Tuple t{ 0, 0, ""_sv };
        HE_EXPECT_EQ(TupleGet<0>(t), 0);
        HE_EXPECT_EQ(TupleGet<1>(t), 0);
        HE_EXPECT_EQ(TupleGet<2>(t), "");

        TupleApply(t, [](auto& a, auto& b, auto& c)
        {
            a = 1;
            b = 2;
            c = "Hello!";
        });

        HE_EXPECT_EQ(TupleGet<0>(t), 1);
        HE_EXPECT_EQ(TupleGet<1>(t), 2);
        HE_EXPECT_EQ(TupleGet<2>(t), "Hello!");
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, tuple, TupleCat)
{
    static_assert(IsSame<decltype(TupleCat()), Tuple<>>);
    static_assert(IsSame<decltype(TupleCat(Tuple<>())), Tuple<>>);
    static_assert(IsSame<decltype(TupleCat(Tuple<int>())), Tuple<int>>);
    static_assert(IsSame<decltype(TupleCat(Tuple<int, char>())), Tuple<int, char>>);
    static_assert(IsSame<decltype(TupleCat(Tuple<char>(), Tuple<int>())), Tuple<char, int>>);
    static_assert(IsSame<decltype(TupleCat(Tuple<char>(), Tuple<int>(), Tuple<String>())), Tuple<char, int, String>>);

    Tuple<UniquePtr<int>, String, char, char, char> tup = TupleCat(
        Tuple{ MakeUnique<int>(69420) },
        Tuple{ String{ "Hello, world!" } },
        Tuple{ 'a', 'b', 'c' });

    HE_EXPECT_EQ(*TupleGet<0>(tup), 69420);
    HE_EXPECT_EQ(TupleGet<1>(tup), "Hello, world!");
    HE_EXPECT_EQ(TupleGet<2>(tup), 'a');
    HE_EXPECT_EQ(TupleGet<3>(tup), 'b');
    HE_EXPECT_EQ(TupleGet<4>(tup), 'c');
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, tuple, TupleMap)
{
    // Basic
    {
        Tuple t{ 10, 20.4, "Hello!" };
        Tuple actual = TupleMap(t, [](auto v) { return Format("{}", v); });
        Tuple expected{ String("10"), String("20.4"), String("Hello!") };
        HE_EXPECT_EQ(actual, expected);
    }

    // Forwarding
    {
        Tuple t0{ 10 };
        Tuple t1 = TupleMap(t0, [](auto& x) -> decltype(auto) { return x; });
        Tuple t2 = TupleMap(t0, [](auto& x) -> decltype(auto) { return Move(x); });

        static_assert(IsSame<decltype(t1), Tuple<int&>>);
        static_assert(IsSame<decltype(t2), Tuple<int&&>>);
    }

    // Move
    {
        Tuple t{ 10, 20.4, "Hello!"_sv, MakeUnique<int>(5) };
        Tuple actual = TupleMap(Move(t), [](auto v) { return v; });
        Tuple expected{ 10, 20.4, "Hello!"_sv, MakeUnique<int>(5) };

        static_assert(IsSame<decltype(actual), decltype(expected)>);

        HE_EXPECT(!TupleGet<3>(t));
        HE_EXPECT(TupleGet<3>(actual));
        HE_EXPECT_EQ(*TupleGet<3>(actual), *TupleGet<3>(expected));
        HE_EXPECT(actual == expected);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, tuple, TupleReduce)
{
    // Basic
    {
        Tuple t{ 10, 20.4, 50ull };
        const bool r0 = TupleReduce<bool>(t, [](bool prev, auto item) { return prev && item < 100; });
        const bool r1 = TupleReduce<bool>(t, [](bool prev, auto item) { return prev && item > 100; });
        const double sum = TupleReduce<double>(t, [](double prev, auto item) { return prev + item; });
        HE_EXPECT(r0);
        HE_EXPECT(!r1);
        HE_EXPECT_EQ(sum, 80.4);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, tuple, fmt)
{
    HE_EXPECT_EQ(Format("{}", Tuple<>{}), "()");
    HE_EXPECT_EQ(Format("{}", Tuple{ 1, 2 }), "(1, 2)");
    HE_EXPECT_EQ(Format("{}", Tuple{ 1, "test"_sv, 2 }), "(1, test, 2)");
}
