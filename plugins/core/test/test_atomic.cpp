// Copyright Chad Engler

#include "he/core/atomic.h"

#include "he/core/test.h"
#include "he/core/type_traits.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
template <template <typename> typename TestType>
void ApplyAtomicTest_Integral()
{
    TestType<char>::Run();
    TestType<signed char>::Run();
    TestType<unsigned char>::Run();
    TestType<char16_t>::Run();
    TestType<char32_t>::Run();
    TestType<wchar_t>::Run();
    TestType<short>::Run();
    TestType<int>::Run();
    TestType<long>::Run();
    TestType<long long>::Run();
    TestType<unsigned short>::Run();
    TestType<unsigned int>::Run();
    TestType<unsigned long>::Run();
    TestType<unsigned long long>::Run();
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, atomic, ElementType)
{
    static_assert(IsSame<Atomic<int>::ElementType, int>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, atomic, IsLockFree)
{
    static_assert(Atomic<bool>::IsLockFree());
    static_assert(Atomic<char>::IsLockFree());
    static_assert(Atomic<char16_t>::IsLockFree());
    static_assert(Atomic<char32_t>::IsLockFree());
    static_assert(Atomic<wchar_t>::IsLockFree());
    static_assert(Atomic<short>::IsLockFree());
    static_assert(Atomic<int>::IsLockFree());
    static_assert(Atomic<long>::IsLockFree());
    static_assert(Atomic<long long>::IsLockFree());
    static_assert(Atomic<void*>::IsLockFree());
}

// ------------------------------------------------------------------------------------------------
template <typename T>
struct TestAtomicCtor
{
    static void Run()
    {
        constexpr T Zero{};
        constexpr T Value{ -1 };

        // Default construction
        {
            Atomic<T> a;
            HE_EXPECT_EQ(a.Load(), Zero);
        }

        // Aggregate construction
        {
            Atomic<T> a{ Value };
            HE_EXPECT_EQ(a.Load(), Value);
        }

        // Value construction
        {
            Atomic<T> a = Value;
            HE_EXPECT_EQ(a.Load(), Value);
        }

        // Value construction
        {
            Atomic<T> a = { Value };
            HE_EXPECT_EQ(a.Load(), Value);
        }
    }
};

HE_TEST(core, atomic, Construct)
{
    TestAtomicCtor<bool>::Run();
    TestAtomicCtor<void*>::Run();
    ApplyAtomicTest_Integral<TestAtomicCtor>();
}

// ------------------------------------------------------------------------------------------------
template <typename T>
struct TestAtomicLoad
{
    static void Run()
    {
        constexpr T Zero{};
        constexpr T Value{ -1 };

        Atomic<T> a;
        HE_EXPECT_EQ(a.Load(), Zero);
        HE_EXPECT_EQ(a.Load(MemoryOrder::Relaxed), Zero);
        HE_EXPECT_EQ(static_cast<T>(a), Zero);
    }
};

HE_TEST(core, atomic, Load)
{
    TestAtomicLoad<bool>::Run();
    TestAtomicLoad<void*>::Run();
    ApplyAtomicTest_Integral<TestAtomicLoad>();
}

// ------------------------------------------------------------------------------------------------
template <typename T>
struct TestAtomicStore
{
    static void Run()
    {
        constexpr T Zero{};
        constexpr T Value{ -1 };

        Atomic<T> a;
        HE_EXPECT_EQ(a.Load(), Zero);
        a.Store(Value);
        HE_EXPECT_EQ(a.Load(), Value);
        a.Store(Zero, MemoryOrder::SeqCst);
        HE_EXPECT_EQ(a.Load(), Zero);
    }
};

HE_TEST(core, atomic, Store)
{
    TestAtomicStore<bool>::Run();
    TestAtomicStore<void*>::Run();
    ApplyAtomicTest_Integral<TestAtomicStore>();
}

// ------------------------------------------------------------------------------------------------
template <typename T>
struct TestAtomicExchange
{
    static void Run()
    {
        constexpr T Zero{};
        constexpr T Value{ -1 };

        Atomic<T> a;
        HE_EXPECT_EQ(a.Load(), Zero);
        HE_EXPECT_EQ(a.Exchange(Value), Zero);
        HE_EXPECT_EQ(a.Load(), Value);
        HE_EXPECT_EQ(a.Exchange(Zero, MemoryOrder::Relaxed), Value);
        HE_EXPECT_EQ(a.Load(), Zero);
    }
};

HE_TEST(core, atomic, Exchange)
{
    TestAtomicExchange<bool>::Run();
    TestAtomicExchange<void*>::Run();
    ApplyAtomicTest_Integral<TestAtomicExchange>();
}

// ------------------------------------------------------------------------------------------------
template <typename T>
struct TestAtomicCASWeak
{
    static void Run()
    {
        constexpr T Zero{};
        constexpr T Value{ -1 };

        Atomic<T> a;
        T expected = Zero;
        HE_EXPECT_EQ(a.Load(), Zero);
        HE_EXPECT_EQ(a.CompareExchangeWeak(expected, Value), true);
        HE_EXPECT_EQ(expected, Zero);
        HE_EXPECT_EQ(a.Load(), Value);
        HE_EXPECT_EQ(a.CompareExchangeWeak(expected, Zero), false);
        HE_EXPECT_EQ(expected, Value);
        HE_EXPECT_EQ(a.Load(), Value);

        expected = Zero;
        a = Zero;
        HE_EXPECT_EQ(a.CompareExchangeWeak(expected, Value, MemoryOrder::SeqCst), true);
        HE_EXPECT_EQ(expected, Zero);
        HE_EXPECT_EQ(a.Load(), Value);
        HE_EXPECT_EQ(a.CompareExchangeWeak(expected, Zero, MemoryOrder::Relaxed, MemoryOrder::Relaxed), false);
        HE_EXPECT_EQ(expected, Value);
        HE_EXPECT_EQ(a.Load(), Value);
    }
};

HE_TEST(core, atomic, CompareExchangeWeak)
{
    TestAtomicCASWeak<bool>::Run();
    TestAtomicCASWeak<void*>::Run();
    ApplyAtomicTest_Integral<TestAtomicCASWeak>();
}

// ------------------------------------------------------------------------------------------------
template <typename T>
struct TestAtomicCASStrong
{
    static void Run()
    {
        constexpr T Zero{};
        constexpr T Value{ -1 };

        Atomic<T> a;
        T expected = Zero;
        HE_EXPECT_EQ(a.Load(), Zero);
        HE_EXPECT_EQ(a.CompareExchangeStrong(expected, Value), true);
        HE_EXPECT_EQ(expected, Zero);
        HE_EXPECT_EQ(a.Load(), Value);
        HE_EXPECT_EQ(a.CompareExchangeStrong(expected, Zero), false);
        HE_EXPECT_EQ(expected, Value);
        HE_EXPECT_EQ(a.Load(), Value);

        expected = Zero;
        a = Zero;
        HE_EXPECT_EQ(a.CompareExchangeStrong(expected, Value, MemoryOrder::SeqCst), true);
        HE_EXPECT_EQ(expected, Zero);
        HE_EXPECT_EQ(a.Load(), Value);
        HE_EXPECT_EQ(a.CompareExchangeStrong(expected, Zero, MemoryOrder::Relaxed, MemoryOrder::Relaxed), false);
        HE_EXPECT_EQ(expected, Value);
        HE_EXPECT_EQ(a.Load(), Value);
    }
};

HE_TEST(core, atomic, CompareExchangeStrong)
{
    TestAtomicCASStrong<bool>::Run();
    TestAtomicCASStrong<void*>::Run();
    ApplyAtomicTest_Integral<TestAtomicCASStrong>();
}

// ------------------------------------------------------------------------------------------------
template <typename T>
struct TestAtomicFetchAdd
{
    static void Run()
    {
        Atomic<T> a;
        HE_EXPECT_EQ(a.Load(), 0);
        HE_EXPECT_EQ(a.FetchAdd(1), 0);
        HE_EXPECT_EQ(a.Load(), 1);
        HE_EXPECT_EQ(a.FetchAdd(1, MemoryOrder::Relaxed), 1);
        HE_EXPECT_EQ(a.Load(), 2);
    }
};

template <typename T>
struct TestAtomicFetchAdd<T*>
{
    static void Run()
    {
        typename T data[2]{};
        Atomic<T*> a{ data };
        HE_EXPECT_EQ(a.Load(), data);
        HE_EXPECT_EQ(a.FetchAdd(1), data);
        HE_EXPECT_EQ(a.Load(), data + 1);
        HE_EXPECT_EQ(a.FetchAdd(1, MemoryOrder::Relaxed), data + 1);
        HE_EXPECT_EQ(a.Load(), data + 2);
    }
};

HE_TEST(core, atomic, FetchAdd)
{
    TestAtomicFetchAdd<short*>::Run();
    TestAtomicFetchAdd<long*>::Run();
    ApplyAtomicTest_Integral<TestAtomicFetchAdd>();
}

// ------------------------------------------------------------------------------------------------
template <typename T>
struct TestAtomicFetchSub
{
    static void Run()
    {
        Atomic<T> a{ 2 };
        HE_EXPECT_EQ(a.Load(), 2);
        HE_EXPECT_EQ(a.FetchSub(1), 2);
        HE_EXPECT_EQ(a.Load(), 1);
        HE_EXPECT_EQ(a.FetchSub(1, MemoryOrder::Relaxed), 1);
        HE_EXPECT_EQ(a.Load(), 0);
    }
};

template <typename T>
struct TestAtomicFetchSub<T*>
{
    static void Run()
    {
        typename T data[2]{};
        Atomic<T*> a{ data + 2 };
        HE_EXPECT_EQ(a.Load(), data + 2);
        HE_EXPECT_EQ(a.FetchSub(1), data + 2);
        HE_EXPECT_EQ(a.Load(), data + 1);
        HE_EXPECT_EQ(a.FetchSub(1, MemoryOrder::Relaxed), data + 1);
        HE_EXPECT_EQ(a.Load(), data);
    }
};

HE_TEST(core, atomic, FetchSub)
{
    TestAtomicFetchSub<short*>::Run();
    TestAtomicFetchSub<long*>::Run();
    ApplyAtomicTest_Integral<TestAtomicFetchSub>();
}

// ------------------------------------------------------------------------------------------------
template <typename T>
struct TestAtomicFetchAnd
{
    static void Run()
    {
        Atomic<T> a{ 0x07 };
        HE_EXPECT_EQ(a.Load(), 0x07);
        HE_EXPECT_EQ(a.FetchAnd(0x0e), 0x07);
        HE_EXPECT_EQ(a.Load(), 0x06);
        HE_EXPECT_EQ(a.FetchAnd(0x04, MemoryOrder::Relaxed), 0x06);
        HE_EXPECT_EQ(a.Load(), 0x04);
    }
};

HE_TEST(core, atomic, FetchAnd)
{
    ApplyAtomicTest_Integral<TestAtomicFetchAnd>();
}

// ------------------------------------------------------------------------------------------------
template <typename T>
struct TestAtomicFetchOr
{
    static void Run()
    {
        Atomic<T> a{ 0x04 };
        HE_EXPECT_EQ(a.Load(), 0x04);
        HE_EXPECT_EQ(a.FetchOr(0x06), 0x04);
        HE_EXPECT_EQ(a.Load(), 0x06);
        HE_EXPECT_EQ(a.FetchOr(0x03, MemoryOrder::Relaxed), 0x06);
        HE_EXPECT_EQ(a.Load(), 0x07);
    }
};

HE_TEST(core, atomic, FetchOr)
{
    ApplyAtomicTest_Integral<TestAtomicFetchOr>();
}

// ------------------------------------------------------------------------------------------------
template <typename T>
struct TestAtomicFetchXor
{
    static void Run()
    {
        Atomic<T> a{ 0x07 };
        HE_EXPECT_EQ(a.Load(), 0x07);
        HE_EXPECT_EQ(a.FetchXor(0x0d), 0x07);
        HE_EXPECT_EQ(a.Load(), 0x0a);
        HE_EXPECT_EQ(a.FetchXor(0x08, MemoryOrder::Relaxed), 0x0a);
        HE_EXPECT_EQ(a.Load(), 0x02);
    }
};

HE_TEST(core, atomic, FetchXor)
{
    ApplyAtomicTest_Integral<TestAtomicFetchXor>();
}

// ------------------------------------------------------------------------------------------------
template <typename T>
struct TestAtomicOperatorAssign
{
    static void Run()
    {
        constexpr T Zero{};
        constexpr T Value{ -1 };

        Atomic<T> a;
        HE_EXPECT_EQ(a.Load(), Zero);
        a = Value;
        HE_EXPECT_EQ(a.Load(), Value);
        a = Zero;
        HE_EXPECT_EQ(a.Load(), Zero);
    }
};

HE_TEST(core, atomic, Operator_Assign)
{
    TestAtomicOperatorAssign<void*>::Run();
    ApplyAtomicTest_Integral<TestAtomicOperatorAssign>();
}

// ------------------------------------------------------------------------------------------------
template <typename T>
struct TestAtomicOperatorInc
{
    static void Run()
    {
        Atomic<T> a;
        HE_EXPECT_EQ(a++, 0);
        HE_EXPECT_EQ(a.Load(), 1);
        HE_EXPECT_EQ(++a, 2);
        HE_EXPECT_EQ(a.Load(), 2);
    }
};

template <typename T>
struct TestAtomicOperatorInc<T*>
{
    static void Run()
    {
        typename T data[2]{};
        Atomic<T*> a{ data };
        HE_EXPECT_EQ(a++, data);
        HE_EXPECT_EQ(a.Load(), data + 1);
        HE_EXPECT_EQ(++a, data + 2);
        HE_EXPECT_EQ(a.Load(), data + 2);
    }
};

HE_TEST(core, atomic, Operator_Increment)
{
    TestAtomicOperatorInc<short*>::Run();
    TestAtomicOperatorInc<long*>::Run();
    ApplyAtomicTest_Integral<TestAtomicOperatorInc>();
}

// ------------------------------------------------------------------------------------------------
template <typename T>
struct TestAtomicOperatorDec
{
    static void Run()
    {
        Atomic<T> a{ 2 };
        HE_EXPECT_EQ(a--, 2);
        HE_EXPECT_EQ(a.Load(), 1);
        HE_EXPECT_EQ(--a, 0);
        HE_EXPECT_EQ(a.Load(), 0);
    }
};

template <typename T>
struct TestAtomicOperatorDec<T*>
{
    static void Run()
    {
        typename T data[2]{};
        Atomic<T*> a{ data + 2 };
        HE_EXPECT_EQ(a--, data + 2);
        HE_EXPECT_EQ(a.Load(), data + 1);
        HE_EXPECT_EQ(--a, data);
        HE_EXPECT_EQ(a.Load(), data);
    }
};

HE_TEST(core, atomic, Operator_Decrement)
{
    TestAtomicOperatorDec<short*>::Run();
    TestAtomicOperatorDec<long*>::Run();
    ApplyAtomicTest_Integral<TestAtomicOperatorDec>();
}

// ------------------------------------------------------------------------------------------------
template <typename T>
struct TestAtomicOperatorAddEq
{
    static void Run()
    {
        Atomic<T> a;
        HE_EXPECT_EQ(a += 1, 1);
        HE_EXPECT_EQ(a.Load(), 1);
        HE_EXPECT_EQ(a += 1, 2);
        HE_EXPECT_EQ(a.Load(), 2);
    }
};

template <typename T>
struct TestAtomicOperatorAddEq<T*>
{
    static void Run()
    {
        typename T data[2]{};
        Atomic<T*> a{ data };
        HE_EXPECT_EQ(a += 1, data + 1);
        HE_EXPECT_EQ(a.Load(), data + 1);
        HE_EXPECT_EQ(a += 1, data + 2);
        HE_EXPECT_EQ(a.Load(), data + 2);
    }
};

HE_TEST(core, atomic, Operator_AddEq)
{
    TestAtomicOperatorAddEq<short*>::Run();
    TestAtomicOperatorAddEq<long*>::Run();
    ApplyAtomicTest_Integral<TestAtomicOperatorAddEq>();
}

// ------------------------------------------------------------------------------------------------
template <typename T>
struct TestAtomicOperatorSubEq
{
    static void Run()
    {
        Atomic<T> a{ 2 };
        HE_EXPECT_EQ(a -= 1, 1);
        HE_EXPECT_EQ(a.Load(), 1);
        HE_EXPECT_EQ(a -= 1, 0);
        HE_EXPECT_EQ(a.Load(), 0);
    }
};

template <typename T>
struct TestAtomicOperatorSubEq<T*>
{
    static void Run()
    {
        typename T data[2]{};
        Atomic<T*> a{ data + 2 };
        HE_EXPECT_EQ(a -= 1, data + 1);
        HE_EXPECT_EQ(a.Load(), data + 1);
        HE_EXPECT_EQ(a -= 1, data);
        HE_EXPECT_EQ(a.Load(), data);
    }
};

HE_TEST(core, atomic, Operator_SubEq)
{
    TestAtomicOperatorSubEq<short*>::Run();
    TestAtomicOperatorSubEq<long*>::Run();
    ApplyAtomicTest_Integral<TestAtomicOperatorSubEq>();
}

// ------------------------------------------------------------------------------------------------
template <typename T>
struct TestAtomicOperatorAndEq
{
    static void Run()
    {
        Atomic<T> a{ 0x07 };
        HE_EXPECT_EQ(a &= 0x0e, 0x06);
        HE_EXPECT_EQ(a.Load(), 0x06);
        HE_EXPECT_EQ(a &= 0x04, 0x04);
        HE_EXPECT_EQ(a.Load(), 0x04);
    }
};

HE_TEST(core, atomic, Operator_AndEq)
{
    ApplyAtomicTest_Integral<TestAtomicOperatorAndEq>();
}

// ------------------------------------------------------------------------------------------------
template <typename T>
struct TestAtomicOperatorOrEq
{
    static void Run()
    {
        Atomic<T> a{ 0x04 };
        HE_EXPECT_EQ(a |= 0x06, 0x06);
        HE_EXPECT_EQ(a.Load(), 0x06);
        HE_EXPECT_EQ(a |= 0x03, 0x07);
        HE_EXPECT_EQ(a.Load(), 0x07);
    }
};

HE_TEST(core, atomic, Operator_OrEq)
{
    ApplyAtomicTest_Integral<TestAtomicOperatorOrEq>();
}

// ------------------------------------------------------------------------------------------------
template <typename T>
struct TestAtomicOperatorXorEq
{
    static void Run()
    {
        Atomic<T> a{ 0x07 };
        HE_EXPECT_EQ(a ^= 0x0d, 0x0a);
        HE_EXPECT_EQ(a.Load(), 0x0a);
        HE_EXPECT_EQ(a ^= 0x08, 0x02);
        HE_EXPECT_EQ(a.Load(), 0x02);
    }
};

HE_TEST(core, atomic, Operator_XorEq)
{
    ApplyAtomicTest_Integral<TestAtomicOperatorXorEq>();
}
