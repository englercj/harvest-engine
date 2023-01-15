// Copyright Chad Engler

#include "he/core/hash.h"

#include "he/core/random.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
template <typename H>
static void TestHashBool(bool value)
{
    Hash<H> a, b;
    typename H::ValueType v0, v1;

    a.Update(value);
    v0 = a.Value();

    uint8_t expected = value ? 1 : 0;
    b.Update(&expected, sizeof(expected));
    v1 = b.Value();

    HE_EXPECT_EQ(v0, v1);
}

// ------------------------------------------------------------------------------------------------
template <typename H, typename T>
static void TestHashScalar(T value)
{
    Hash<H> a, b;
    typename H::ValueType v0, v1;

    a.Update(value);
    v0 = a.Value();

    b.Update(&value, sizeof(value));
    v1 = b.Value();

    HE_EXPECT_EQ(v0, v1);
}

// ------------------------------------------------------------------------------------------------
template <typename H>
static void TestHashString(const char* value)
{
    Hash<H> a, b;
    typename H::ValueType v0, v1;

    a.Update(value);
    v0 = a.Value();

    b.Update(value, String::Length(value));
    v1 = b.Value();

    HE_EXPECT_EQ(v0, v1);
}

// ------------------------------------------------------------------------------------------------
template <typename H>
static void TestHashAlgorithm()
{
    TestHashBool<H>(true);
    TestHashBool<H>(false);

    TestHashScalar<H, bool>(true);
    TestHashScalar<H, bool>(false);

    TestHashScalar<H, int8_t>(0x12);
    TestHashScalar<H, int16_t>(0x1234);
    TestHashScalar<H, int32_t>(0x12345678);
    TestHashScalar<H, int64_t>(0x12345678901234);

    TestHashScalar<H, uint8_t>(0x12);
    TestHashScalar<H, uint16_t>(0x1234);
    TestHashScalar<H, uint32_t>(0x12345678);
    TestHashScalar<H, uint64_t>(0x12345678901234);

    TestHashString<H>("foobar");
    TestHashString<H>("foobarabcdefghiglmnop");

    // Reset allows for reuse
    Hash<H> a;
    typename H::ValueType v0, v1;

    const char* x = "foobar";
    a.Update(x);
    v0 = a.Value();

    a.Reset();

    a.Update(x);
    v1 = a.Value();

    HE_EXPECT_EQ(v0, v1);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, CombineHash)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, Mix)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, FNV32)
{
    TestHashAlgorithm<FNV32>();

    HE_EXPECT_EQ(FNV32::String(""), 0x811c9dc5);
    HE_EXPECT_EQ(FNV32::String("a"), 0xe40c292c);
    HE_EXPECT_EQ(FNV32::String("foobar"), 0xbf9cf968);
    HE_EXPECT_EQ(FNV32::String("foobarabcdefghiglmnop"), 0xaec8b08c);
    HE_EXPECT_EQ(FNV32::String("\200"), 0x850b939f);

    HE_EXPECT_EQ(FNV32::Mem("foobar", 6), 0xbf9cf968);
    HE_EXPECT_EQ(FNV32::Mem("\200", 1), 0x850b939f);

    static_assert(FNV32::String("foobar") == 0xbf9cf968, "");
    static_assert(FNV32::String("foobarabcdefghiglmnop") == 0xaec8b08c, "");
    static_assert(FNV32::String("\200") == 0x850b939f, "");
    static_assert(FNV32::String({ "apple", 1 }) == 0xe40c292c, "");

    uint32_t h = FNV32::String("abc");
    h = FNV32::String("def", h);
    HE_EXPECT_EQ(h, FNV32::String("abcdef"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, FNV64)
{
    TestHashAlgorithm<FNV64>();

    HE_EXPECT_EQ(FNV64::String(""), 0xcbf29ce484222325);
    HE_EXPECT_EQ(FNV64::String("a"), 0xaf63dc4c8601ec8c);
    HE_EXPECT_EQ(FNV64::String("foobar"), 0x85944171f73967e8);
    HE_EXPECT_EQ(FNV64::String("foobarabcdefghiglmnop"), 0x9c6beb459f40f26c);
    HE_EXPECT_EQ(FNV64::String("\200"), 0xaf643d4c8602915f);

    HE_EXPECT_EQ(FNV64::Mem("foobar", 6), 0x85944171f73967e8);
    HE_EXPECT_EQ(FNV64::Mem("\200", 1), 0xaf643d4c8602915f);

    static_assert(FNV64::String("foobar") == 0x85944171f73967e8, "");
    static_assert(FNV64::String("foobarabcdefghiglmnop") == 0x9c6beb459f40f26c, "");
    static_assert(FNV64::String("\200") == 0xaf643d4c8602915f, "");
    static_assert(FNV64::String({ "apple", 1 }) == 0xaf63dc4c8601ec8c, "");

    uint64_t h = FNV64::String("abc");
    h = FNV64::String("def", h);
    HE_EXPECT_EQ(h, FNV64::String("abcdef"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, CRC32C)
{
    TestHashAlgorithm<CRC32C>();

    HE_EXPECT_EQ(CRC32C::Mem("", 0), 0);
    HE_EXPECT_EQ(CRC32C::Mem("a", 1), 0xc1d04330);
    HE_EXPECT_EQ(CRC32C::Mem("foobar", 6), 0x0d5f5c7f);
    HE_EXPECT_EQ(CRC32C::Mem("foobarabcdefghiglmnop", 21), 0xfef874d9);
    HE_EXPECT_EQ(CRC32C::Mem("\200", 1), 0xd08b6829);

    uint32_t h = CRC32C::Mem("abc", 3);
    h = CRC32C::Mem("def", 3, h);
    HE_EXPECT_EQ(h, CRC32C::Mem("abcdef", 6));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, WyHash)
{
    TestHashAlgorithm<WyHash>();

    HE_EXPECT_EQ(WyHash::Mem("", 0), 0x0409638ee2bde459);
    HE_EXPECT_EQ(WyHash::Mem("a", 1), 0x28d2053309d28531);
    HE_EXPECT_EQ(WyHash::Mem("foobar", 6), 0x0b9d35b96e1f6fe2);
    HE_EXPECT_EQ(WyHash::Mem("foobarabcdefghiglmnop", 21), 0x698f1bc80e749f46);
    HE_EXPECT_EQ(WyHash::Mem("\200", 1), 0x26f0dff3757426db);

    HE_EXPECT_EQ(WyHash::Mem("foobar", 6), 0x0b9d35b96e1f6fe2);
    HE_EXPECT_EQ(WyHash::Mem("\200", 1), 0x26f0dff3757426db);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, Hasher)
{
    enum TestEnum { V = 123 };
    enum class TestEnumClass { V = 123 };

    static_assert(Hasher<bool>()(true) == 0xffffffffffffffff);
    static_assert(Hasher<bool>()(false) == 0);
    static_assert(Hasher<char>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<unsigned char>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<signed char>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<wchar_t>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<char8_t>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<char16_t>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<char32_t>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<short>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<unsigned short>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<int>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<unsigned int>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<long>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<unsigned long>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<long long>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<unsigned long long>()(123) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<TestEnum>()(TestEnum::V) == 0x7fcb4990c961f6a8);
    static_assert(Hasher<TestEnumClass>()(TestEnumClass::V) == 0x7fcb4990c961f6a8);

    HE_EXPECT_EQ(Hasher<bool>()(true), 0xffffffffffffffff);
    HE_EXPECT_EQ(Hasher<bool>()(false), 0);
    HE_EXPECT_EQ(Hasher<char>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<unsigned char>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<signed char>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<wchar_t>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<char8_t>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<char16_t>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<char32_t>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<short>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<unsigned short>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<int>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<unsigned int>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<long>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<unsigned long>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<long long>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<unsigned long long>()(123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<TestEnum>()(TestEnum::V), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<TestEnumClass>()(TestEnumClass::V), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<void*>()((void*)123), 0x7fcb4990c961f6a8);
    HE_EXPECT_EQ(Hasher<float>()(123), 0xf8450ab74f865b48);
    HE_EXPECT_EQ(Hasher<double>()(123), 0xfde89695555aaf6f);
    HE_EXPECT_EQ(Hasher<long double>()(123), 0xfde89695555aaf6f);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, GetHashCode)
{
    enum TestEnum { V = 123 };
    enum class TestEnumClass { V = 123 };

    static_assert(GetHashCode<bool>(true) == Hasher<bool>()(true));
    static_assert(GetHashCode<bool>(false) == Hasher<bool>()(false));
    static_assert(GetHashCode<char>(123) == Hasher<char>()(123));
    static_assert(GetHashCode<unsigned >(123) == Hasher<unsigned char>()(123));
    static_assert(GetHashCode<signed >(123) == Hasher<signed char>()(123));
    static_assert(GetHashCode<wchar_t>(123) == Hasher<wchar_t>()(123));
    static_assert(GetHashCode<char8_t>(123) == Hasher<char8_t>()(123));
    static_assert(GetHashCode<char16_t>(123) == Hasher<char16_t>()(123));
    static_assert(GetHashCode<char32_t>(123) == Hasher<char32_t>()(123));
    static_assert(GetHashCode<short>(123) == Hasher<short>()(123));
    static_assert(GetHashCode<unsigned >(123) == Hasher<unsigned short>()(123));
    static_assert(GetHashCode<int>(123) == Hasher<int>()(123));
    static_assert(GetHashCode<unsigned >(123) == Hasher<unsigned int>()(123));
    static_assert(GetHashCode<long>(123) == Hasher<long>()(123));
    static_assert(GetHashCode<unsigned >(123) == Hasher<unsigned long>()(123));
    static_assert(GetHashCode<long >(123) == Hasher<long long>()(123));
    static_assert(GetHashCode<unsigned >(123) == Hasher<unsigned long long>()(123));
    static_assert(GetHashCode<TestEnum>(TestEnum::V) == Hasher<TestEnum>()(TestEnum::V));
    static_assert(GetHashCode<TestEnumClass>(TestEnumClass::V) == Hasher<TestEnumClass>()(TestEnumClass::V));

    HE_EXPECT_EQ(GetHashCode<bool>(true), Hasher<bool>()(true));
    HE_EXPECT_EQ(GetHashCode<bool>(false), Hasher<bool>()(false));
    HE_EXPECT_EQ(GetHashCode<char>(123), Hasher<char>()(123));
    HE_EXPECT_EQ(GetHashCode<unsigned >(123), Hasher<unsigned char>()(123));
    HE_EXPECT_EQ(GetHashCode<signed >(123), Hasher<signed char>()(123));
    HE_EXPECT_EQ(GetHashCode<wchar_t>(123), Hasher<wchar_t>()(123));
    HE_EXPECT_EQ(GetHashCode<char8_t>(123), Hasher<char8_t>()(123));
    HE_EXPECT_EQ(GetHashCode<char16_t>(123), Hasher<char16_t>()(123));
    HE_EXPECT_EQ(GetHashCode<char32_t>(123), Hasher<char32_t>()(123));
    HE_EXPECT_EQ(GetHashCode<short>(123), Hasher<short>()(123));
    HE_EXPECT_EQ(GetHashCode<unsigned >(123), Hasher<unsigned short>()(123));
    HE_EXPECT_EQ(GetHashCode<int>(123), Hasher<int>()(123));
    HE_EXPECT_EQ(GetHashCode<unsigned >(123), Hasher<unsigned int>()(123));
    HE_EXPECT_EQ(GetHashCode<long>(123), Hasher<long>()(123));
    HE_EXPECT_EQ(GetHashCode<unsigned >(123), Hasher<unsigned long>()(123));
    HE_EXPECT_EQ(GetHashCode<long >(123), Hasher<long long>()(123));
    HE_EXPECT_EQ(GetHashCode<unsigned >(123), Hasher<unsigned long long>()(123));
    HE_EXPECT_EQ(GetHashCode<TestEnum>(TestEnum::V), Hasher<TestEnum>()(TestEnum::V));
    HE_EXPECT_EQ(GetHashCode<TestEnumClass>(TestEnumClass::V), Hasher<TestEnumClass>()(TestEnumClass::V));
    HE_EXPECT_EQ(GetHashCode<void*>((void*)123), Hasher<void*>()((void*)123));
    HE_EXPECT_EQ(GetHashCode<float>(123), Hasher<float>()(123));
    HE_EXPECT_EQ(GetHashCode<double>(123), Hasher<double>()(123));
    HE_EXPECT_EQ(GetHashCode<long double>(123), Hasher<long double>()(123));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, HasHashCode)
{
    struct Nope1 {};
    struct Nope2 { uint32_t HashCode() const; };
    struct Nope3 { uint64_t HashCode(); };
    struct Nope4 { int64_t HashCode() const; };
    struct Yup1 { uint64_t HashCode() const; };
    struct Yup2 { uint64_t HashCode() const noexcept; };

    static_assert(!HasHashCode<Nope1>);
    static_assert(!HasHashCode<Nope2>);
    static_assert(!HasHashCode<Nope3>);
    static_assert(!HasHashCode<Nope4>);
    static_assert(HasHashCode<Yup1>);
    static_assert(HasHashCode<Yup2>);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, hash, Hashable)
{
    enum TestEnum {};
    enum class TestEnumClass {};

    static_assert(Hashable<bool>);
    static_assert(Hashable<char>);
    static_assert(Hashable<unsigned char>);
    static_assert(Hashable<signed char>);
    static_assert(Hashable<wchar_t>);
    static_assert(Hashable<char8_t>);
    static_assert(Hashable<char16_t>);
    static_assert(Hashable<char32_t>);
    static_assert(Hashable<short>);
    static_assert(Hashable<unsigned short>);
    static_assert(Hashable<int>);
    static_assert(Hashable<unsigned int>);
    static_assert(Hashable<long>);
    static_assert(Hashable<unsigned long>);
    static_assert(Hashable<long long>);
    static_assert(Hashable<unsigned long long>);
    static_assert(Hashable<TestEnum>);
    static_assert(Hashable<TestEnumClass>);
    static_assert(Hashable<void*>);
    static_assert(Hashable<float>);
    static_assert(Hashable<double>);
    static_assert(Hashable<long double>);

    struct HashableObj { uint64_t HashCode() const; };
    static_assert(Hashable<HashableObj>);

    struct NotHashableObj {};
    static_assert(!Hashable<NotHashableObj>);
}
