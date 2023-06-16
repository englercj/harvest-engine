// Copyright Chad Engler

#include "he/core/key_value.h"
#include "he/core/key_value_fmt.h"

#include "he/core/enum_ops.h"
#include "he/core/fmt.h"
#include "he/core/type_traits.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
template <typename T> struct TypeToKeyValueKind;
template <> struct TypeToKeyValueKind<bool> { static constexpr auto Kind = KeyValue::ValueKind::Bool; };
template <> struct TypeToKeyValueKind<int8_t> { static constexpr auto Kind = KeyValue::ValueKind::Int; };
template <> struct TypeToKeyValueKind<int16_t> { static constexpr auto Kind = KeyValue::ValueKind::Int; };
template <> struct TypeToKeyValueKind<int32_t> { static constexpr auto Kind = KeyValue::ValueKind::Int; };
template <> struct TypeToKeyValueKind<int64_t> { static constexpr auto Kind = KeyValue::ValueKind::Int; };
template <> struct TypeToKeyValueKind<uint8_t> { static constexpr auto Kind = KeyValue::ValueKind::Uint; };
template <> struct TypeToKeyValueKind<uint16_t> { static constexpr auto Kind = KeyValue::ValueKind::Uint; };
template <> struct TypeToKeyValueKind<uint32_t> { static constexpr auto Kind = KeyValue::ValueKind::Uint; };
template <> struct TypeToKeyValueKind<uint64_t> { static constexpr auto Kind = KeyValue::ValueKind::Uint; };
template <> struct TypeToKeyValueKind<float> { static constexpr auto Kind = KeyValue::ValueKind::Double; };
template <> struct TypeToKeyValueKind<double> { static constexpr auto Kind = KeyValue::ValueKind::Double; };
template <> struct TypeToKeyValueKind<const char*> { static constexpr auto Kind = KeyValue::ValueKind::String; };
template <Enum T> struct TypeToKeyValueKind<T> { static constexpr auto Kind = KeyValue::ValueKind::Enum; };

template <KeyValue::ValueKind V> struct TestKeyValueType;

template <> struct TestKeyValueType<KeyValue::ValueKind::Bool>
{
    static void Test(const KeyValue& kv, bool value) { HE_EXPECT_EQ(kv.Bool(), value); }
};

template <> struct TestKeyValueType<KeyValue::ValueKind::Enum>
{
    template <Enum T>
    static void Test(const KeyValue& kv, T value) { HE_EXPECT_EQ(kv.Enum().As<T>(), value); }
};

template <> struct TestKeyValueType<KeyValue::ValueKind::Int>
{
    static void Test(const KeyValue& kv, int64_t value) { HE_EXPECT_EQ(kv.Int(), value); }
};

template <> struct TestKeyValueType<KeyValue::ValueKind::Uint>
{
    static void Test(const KeyValue& kv, uint64_t value) { HE_EXPECT_EQ(kv.Uint(), value); }
};

template <> struct TestKeyValueType<KeyValue::ValueKind::Double>
{
    static void Test(const KeyValue& kv, double value) { HE_EXPECT_EQ(kv.Double(), value); }
};

template <> struct TestKeyValueType<KeyValue::ValueKind::String>
{
    static void Test(const KeyValue& kv, const char* value) { HE_EXPECT_EQ(kv.String(), value); }
};

template <> struct TestKeyValueType<KeyValue::ValueKind::Empty>
{
    static void Test(const KeyValue& kv) { HE_EXPECT(kv.IsEmpty()); }
};

template <typename T>
static KeyValue TestKeyValueCtor(const char* key, T value)
{
    KeyValue kv(key, value);

    constexpr KeyValue::ValueKind ExpectedKind = TypeToKeyValueKind<T>::Kind;

    HE_EXPECT_EQ_PTR(kv.Key(), key);
    HE_EXPECT_EQ(kv.Kind(), ExpectedKind);

    TestKeyValueType<ExpectedKind>::Test(kv, value);

    return kv;
}

// ------------------------------------------------------------------------------------------------
enum class TestEnum { A, B, C };

template <>
const char* he::AsString(TestEnum x)
{
    switch (x)
    {
        case TestEnum::A: return "A";
        case TestEnum::B: return "B";
        case TestEnum::C: return "C";
    }

    return "<unknown>";
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, key_value, MSG_KEY)
{
    // Changing this affects a lot of places, so want to make sure such a change is intentional.
    HE_EXPECT_EQ_STR(HE_MSG_KEY, "message");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, key_value, MSG)
{
    KeyValue msg = HE_MSG("Testing {}", true);
    HE_EXPECT_EQ_STR(msg.Key(), HE_MSG_KEY);
    HE_EXPECT_EQ(msg.Kind(), KeyValue::ValueKind::String);
    HE_EXPECT_EQ(msg.String(), "Testing true");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, key_value, VAL)
{
    const bool test = true;
    KeyValue msg = HE_VAL(test);
    HE_EXPECT_EQ_STR(msg.Key(), "test");
    HE_EXPECT_EQ(msg.Kind(), KeyValue::ValueKind::Bool);
    HE_EXPECT_EQ(msg.Bool(), true);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, key_value, KeyValue)
{
    TestKeyValueCtor("key0", true);
    TestKeyValueCtor("key1", false);
    TestKeyValueCtor("key2", static_cast<int8_t>(0));
    TestKeyValueCtor("key3", static_cast<int16_t>(0));
    TestKeyValueCtor("key4", static_cast<int32_t>(0));
    TestKeyValueCtor("key5", static_cast<int64_t>(0));
    TestKeyValueCtor("key6", static_cast<uint8_t>(0));
    TestKeyValueCtor("key7", static_cast<uint16_t>(0));
    TestKeyValueCtor("key8", static_cast<uint32_t>(0));
    TestKeyValueCtor("key9", static_cast<uint64_t>(0));
    TestKeyValueCtor("key10", 0.0f);
    TestKeyValueCtor("key11", 0.0);
    TestKeyValueCtor("key15", "testing");
    TestKeyValueCtor("key12", TestEnum::A);

    // Format ctor
    KeyValue fmtKv = HE_KV(key16, "testing {}", 5);
    HE_EXPECT_EQ_STR(fmtKv.Key(), "key16");
    HE_EXPECT_EQ(fmtKv.Kind(), KeyValue::ValueKind::String);
    HE_EXPECT_EQ(fmtKv.String(), "testing 5");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, key_value, fmt)
{
    const KeyValue kvs[] =
    {
        { "bool", true },
        { "enum", KeyValue::ValueKind::Bool },
        { "int", 10 },
        { "uint", 20u },
        { "double", 50.12 },
        { "str", "test" },
    };

    const String values = Format("{}", FmtJoin(kvs, kvs + HE_LENGTH_OF(kvs), ", "));

    HE_EXPECT_EQ(values, "bool = true, enum = Bool(0), int = 10, uint = 20, double = 50.12, str = test");
}
