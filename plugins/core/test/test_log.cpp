// Copyright Chad Engler

#include "he/core/log.h"

#include "he/core/test.h"
#include "he/core/type_traits.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
template <typename T, typename V = void> struct TypeToLogKVKind;
template <> struct TypeToLogKVKind<bool> { static constexpr auto Kind = LogKV::Kind::Bool; };
template <> struct TypeToLogKVKind<int8_t> { static constexpr auto Kind = LogKV::Kind::Int; };
template <> struct TypeToLogKVKind<int16_t> { static constexpr auto Kind = LogKV::Kind::Int; };
template <> struct TypeToLogKVKind<int32_t> { static constexpr auto Kind = LogKV::Kind::Int; };
template <> struct TypeToLogKVKind<int64_t> { static constexpr auto Kind = LogKV::Kind::Int; };
template <> struct TypeToLogKVKind<uint8_t> { static constexpr auto Kind = LogKV::Kind::Uint; };
template <> struct TypeToLogKVKind<uint16_t> { static constexpr auto Kind = LogKV::Kind::Uint; };
template <> struct TypeToLogKVKind<uint32_t> { static constexpr auto Kind = LogKV::Kind::Uint; };
template <> struct TypeToLogKVKind<uint64_t> { static constexpr auto Kind = LogKV::Kind::Uint; };
template <> struct TypeToLogKVKind<float> { static constexpr auto Kind = LogKV::Kind::Double; };
template <> struct TypeToLogKVKind<double> { static constexpr auto Kind = LogKV::Kind::Double; };
template <> struct TypeToLogKVKind<const char*> { static constexpr auto Kind = LogKV::Kind::String; };

template <typename T> struct TypeToLogKVKind<T, std::enable_if_t<std::is_enum_v<T>>>
{
    static constexpr auto Kind = TypeToLogKVKind<std::underlying_type_t<T>>::Kind;
};

template <LogKV::Kind V> struct TestLogKVType;

template <> struct TestLogKVType<LogKV::Kind::Bool>
{
    static void Test(const LogKV& kv, bool value)
    {
        HE_EXPECT_EQ(kv.value.b, value);
        HE_EXPECT_EQ(kv.GetBool(), value);
    }
};

template <> struct TestLogKVType<LogKV::Kind::Int>
{
    static void Test(const LogKV& kv, int64_t value)
    {
        HE_EXPECT_EQ(kv.value.i, value);
        HE_EXPECT_EQ(kv.GetInt(), value);
    }
};

template <> struct TestLogKVType<LogKV::Kind::Uint>
{
    static void Test(const LogKV& kv, uint64_t value)
    {
        HE_EXPECT_EQ(kv.value.u, value);
        HE_EXPECT_EQ(kv.GetUint(), value);
    }
};

template <> struct TestLogKVType<LogKV::Kind::Double>
{
    static void Test(const LogKV& kv, double value)
    {
        HE_EXPECT_EQ(kv.value.d, value);
        HE_EXPECT_EQ(kv.GetDouble(), value);
    }
};

template <> struct TestLogKVType<LogKV::Kind::String>
{
    static void Test(const LogKV& kv, const char* value)
    {
        HE_EXPECT_EQ_STR(kv.value.s.Data(), value);
        HE_EXPECT_EQ_STR(kv.GetString(), value);
    }
};

template <typename T>
static LogKV TestLogKVCtor(const char* key, T value)
{
    LogKV kv(key, value);

    constexpr LogKV::Kind ExpectedKind = TypeToLogKVKind<T>::Kind;

    HE_EXPECT_EQ_PTR(kv.key, key);
    HE_EXPECT_EQ(kv.kind, ExpectedKind);

    if constexpr (std::is_enum_v<T>)
    {
        TestLogKVType<ExpectedKind>::Test(kv, static_cast<std::underlying_type_t<T>>(value));
    }
    else
    {
        TestLogKVType<ExpectedKind>::Test(kv, value);
    }

    return kv;
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, log, LogLevel)
{
    // These values changing is potentially breaking, so checking them here.
    static_assert(HE_LOG_LEVEL_TRACE == 0);
    static_assert(HE_LOG_LEVEL_DEBUG == 1);
    static_assert(HE_LOG_LEVEL_INFO == 2);
    static_assert(HE_LOG_LEVEL_WARN == 3);
    static_assert(HE_LOG_LEVEL_ERROR == 4);

    static_assert(static_cast<uint8_t>(LogLevel::Trace) == HE_LOG_LEVEL_TRACE);
    static_assert(static_cast<uint8_t>(LogLevel::Debug) == HE_LOG_LEVEL_DEBUG);
    static_assert(static_cast<uint8_t>(LogLevel::Info) == HE_LOG_LEVEL_INFO);
    static_assert(static_cast<uint8_t>(LogLevel::Warn) == HE_LOG_LEVEL_WARN);
    static_assert(static_cast<uint8_t>(LogLevel::Error) == HE_LOG_LEVEL_ERROR);

    static_assert(std::is_same_v<std::underlying_type_t<LogLevel>, uint8_t>);

    static_assert(HE_LOG_LEVEL_ENABLED >= HE_LOG_LEVEL_TRACE && HE_LOG_LEVEL_ENABLED <= HE_LOG_LEVEL_ERROR);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, log, LOG_MESSAGE_KEY)
{
    // Changing this affects a lot of places, so want to make sure such a change is intentional.
    HE_EXPECT_EQ_STR(HE_LOG_MESSAGE_KEY, "message");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, log, MSG)
{
    LogKV msg = HE_MSG("Testing {}", true);
    HE_EXPECT_EQ_STR(msg.key, HE_LOG_MESSAGE_KEY);
    HE_EXPECT_EQ(msg.kind, LogKV::Kind::String);
    HE_EXPECT_EQ(msg.value.s, "Testing true");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, log, LogKV)
{
    TestLogKVCtor("key0", true);
    TestLogKVCtor("key1", false);
    TestLogKVCtor("key2", int8_t(0));
    TestLogKVCtor("key3", int16_t(0));
    TestLogKVCtor("key4", int32_t(0));
    TestLogKVCtor("key5", int64_t(0));
    TestLogKVCtor("key6", uint8_t(0));
    TestLogKVCtor("key7", uint16_t(0));
    TestLogKVCtor("key8", uint32_t(0));
    TestLogKVCtor("key9", uint64_t(0));
    TestLogKVCtor("key10", 0.0f);
    TestLogKVCtor("key11", 0.0);
    TestLogKVCtor("key15", "testing");

    // Enum ctor
    enum class TestEnum { A, B, C };
    enum class TestEnum_Int8 : int8_t { A, B, C };
    enum class TestEnum_Uint64 : uint64_t { A, B, C };

    TestLogKVCtor("key12", TestEnum::A);
    TestLogKVCtor("key13", TestEnum_Int8::A);
    TestLogKVCtor("key14", TestEnum_Uint64::A);

    // Format ctor
    LogKV fmtKv = HE_KV(key16, "testing {}", 5);
    HE_EXPECT_EQ_STR(fmtKv.key, "key16");
    HE_EXPECT_EQ(fmtKv.kind, LogKV::Kind::String);
    HE_EXPECT_EQ_STR(fmtKv.value.s.Data(), "testing 5");
    HE_EXPECT_EQ_STR(fmtKv.GetString(), "testing 5");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, log, AddLogSink_RemoveLogSink)
{
    auto sink = [](void*, const LogSource& source, const LogKV* kvs, uint32_t count)
    {
        HE_EXPECT_EQ(source.level, LogLevel::Info);
        HE_EXPECT(source.line == 186 || source.line == 187);
        HE_EXPECT_EQ_STR(source.category, "log_test");
        HE_EXPECT_EQ(count, 1);
        HE_EXPECT_EQ_STR(kvs[0].key, "message");
        HE_EXPECT_EQ(kvs[0].kind, LogKV::Kind::String);
        HE_EXPECT_EQ_STR(kvs[0].GetString(), "testing");
    };

    AddLogSink(sink, nullptr);

    HE_LOG_INFO(log_test, HE_MSG("testing"));
    HE_LOGF_INFO(log_test, "testing");

    RemoveLogSink(sink, nullptr);
}
