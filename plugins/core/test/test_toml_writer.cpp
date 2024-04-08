// Copyright Chad Engler

#include "he/core/toml_writer.h"

#include "he/core/limits.h"
#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/core/string_view.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
class TomlWriterFixture : public TestFixture
{
public:
    void Validate(StringView expected)
    {
        HE_EXPECT_EQ(m_output, expected);
    }

    void ValidateKey(StringView input, StringView expected)
    {
        m_writer.Clear();
        m_writer.Key(input);
        m_writer.Uint(123);

        String doc = expected;
        doc += " = 123";
        Validate(doc);
    }

    void ValidateKey(Span<StringView> inputs, StringView expected)
    {
        m_writer.Clear();
        m_writer.Key(inputs);
        m_writer.Uint(123);

        String doc = expected;
        doc += " = 123";
        Validate(doc);
    }

    template <typename F>
    void ValidateValue(StringView expected, F&& func)
    {
        m_writer.Clear();
        m_writer.Key("key");
        func();

        String doc = "key = ";
        doc += expected;
        Validate(doc);
    }

    void ValidateBool(bool input, StringView expected)
    {
        ValidateValue(expected, [&]() { m_writer.Bool(input); });
    }

    void ValidateInt(int64_t input, StringView expected)
    {
        ValidateValue(expected, [&]() { m_writer.Int(input); });
    }

    void ValidateUint(uint64_t input, TomlUintFormat format, StringView expected)
    {
        ValidateValue(expected, [&]() { m_writer.Uint(input, format); });
    }

    void ValidateFloat(double input, TomlFloatFormat format, int32_t precision, StringView expected)
    {
        ValidateValue(expected, [&]() { m_writer.Float(input, format, precision); });
    }

    void ValidateBasicString(StringView input, StringView expected)
    {
        ValidateValue(expected, [&]() { m_writer.String(input, TomlStringFormat::Basic); });
    }

    void ValidateLiteralString(StringView input, StringView expected)
    {
        ValidateValue(expected, [&]() { m_writer.String(input, TomlStringFormat::Literal); });
    }

    void ValidateDateTime(SystemTime input, TomlDateTimeFormat format, StringView expected)
    {
        ValidateValue(expected, [&]() { m_writer.DateTime(input, format); });
    }

    void ValidateTime(Duration input, StringView expected)
    {
        ValidateValue(expected, [&]() { m_writer.Time(input); });
    }

    String m_output;
    TomlWriter m_writer{ m_output };
};

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_writer, array_empty, TomlWriterFixture)
{
    m_writer.Key("key");
    m_writer.StartArray();
    m_writer.EndArray();
    Validate("key = []");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_writer, array_items, TomlWriterFixture)
{
    m_writer.Key("key");
    m_writer.StartArray();
    m_writer.Uint(1);
    m_writer.Uint(0x9ffffffe, TomlUintFormat::Hex);
    m_writer.Comment(" this is a comment");
    m_writer.String("literal", TomlStringFormat::Literal);
    m_writer.String("basic", TomlStringFormat::Basic);
    m_writer.Float(1.5);
    m_writer.Bool(true);
    m_writer.String("multiline\n\r\n\nliteral", TomlStringFormat::Literal);
    m_writer.String("multiline\n\r\n\nbasic", TomlStringFormat::Basic);
    m_writer.EndArray();
    Validate("key = [1, 0x9ffffffe# this is a comment, 'literal', \"basic\", 1.5, true, '''\nmultiline\n\r\n\nliteral''', \"\"\"\nmultiline\n\r\n\nbasic\"\"\"]");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_writer, array_nested, TomlWriterFixture)
{
    m_writer.Key("key");
    m_writer.StartArray();
    m_writer.Uint(1);
    m_writer.StartArray();
    m_writer.Uint(2);
    m_writer.Uint(3);
    m_writer.EndArray();
    m_writer.String("4", TomlStringFormat::Literal);
    m_writer.StartArray();
    m_writer.String("5");
    m_writer.String("6");
    m_writer.EndArray();
    m_writer.EndArray();
    Validate("key = [1, [2, 3], '4', [\"5\", \"6\"]]");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_writer, table, TomlWriterFixture)
{
    m_writer.Table("key");
    Validate("[key]");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_writer, table_quoted, TomlWriterFixture)
{
    StringView keys[] = { "test", "123", "name with spaces" };
    m_writer.Table(keys);
    Validate("[test.123.\"name with spaces\"]");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_writer, table_array, TomlWriterFixture)
{
    m_writer.Table("key", true);
    Validate("[[key]]");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_writer, table_array_quoted, TomlWriterFixture)
{
    StringView keys[] = { "test", "123", "name with spaces" };
    m_writer.Table(keys, true);
    Validate("[[test.123.\"name with spaces\"]]");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_writer, inline_table_empty, TomlWriterFixture)
{
    m_writer.Key("key");
    m_writer.StartInlineTable();
    m_writer.EndInlineTable();
    Validate("key = {}");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_writer, inline_table, TomlWriterFixture)
{
    m_writer.Key("key");
    m_writer.StartInlineTable();
    m_writer.Key("value");
    m_writer.Uint(123);
    m_writer.Key("key\tkey");
    m_writer.Uint(456);
    m_writer.EndInlineTable();
    Validate("key = {value = 123, \"key\\tkey\" = 456}");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_writer, inline_table_with_array, TomlWriterFixture)
{
    m_writer.Key("key");
    m_writer.StartInlineTable();
    m_writer.Key("value");
    m_writer.Uint(123);
    m_writer.Key("arr");
    m_writer.StartArray();
    m_writer.Uint(1);
    m_writer.Uint(2);
    m_writer.Uint(3);
    m_writer.EndArray();
    m_writer.EndInlineTable();
    Validate("key = {value = 123, arr = [1, 2, 3]}");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_writer, value_boolean, TomlWriterFixture)
{
    ValidateBool(true, "true");
    ValidateBool(false, "false");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_writer, value_int, TomlWriterFixture)
{
    ValidateInt(0, "0");
    ValidateInt(-1, "-1");
    ValidateInt(-456789, "-456789");
    ValidateInt(Limits<int64_t>::Min, "-9223372036854775808");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_writer, value_uint, TomlWriterFixture)
{
    // Decimal
    ValidateUint(0, TomlUintFormat::Decimal, "0");
    ValidateUint(1, TomlUintFormat::Decimal, "1");
    ValidateUint(456789, TomlUintFormat::Decimal, "456789");
    ValidateUint(Limits<int64_t>::Max, TomlUintFormat::Decimal, "9223372036854775807");
    ValidateUint(Limits<uint64_t>::Max, TomlUintFormat::Decimal, "18446744073709551615");

    // Hex
    ValidateUint(0x0, TomlUintFormat::Hex, "0x0");
    ValidateUint(0xdeadbeef, TomlUintFormat::Hex, "0xdeadbeef");
    ValidateUint(Limits<uint32_t>::Max, TomlUintFormat::Hex, "0xffffffff");
    ValidateUint(Limits<uint64_t>::Max, TomlUintFormat::Hex, "0xffffffffffffffff");

    // Octal
    ValidateUint(0, TomlUintFormat::Octal, "0o0");
    ValidateUint(0755, TomlUintFormat::Octal, "0o755");
    ValidateUint(0644, TomlUintFormat::Octal, "0o644");

    // Bin
    ValidateUint(0, TomlUintFormat::Binary, "0b0");
    ValidateUint(0b1, TomlUintFormat::Binary, "0b1");
    ValidateUint(0b11, TomlUintFormat::Binary, "0b11");
    ValidateUint(0b111, TomlUintFormat::Binary, "0b111");
    ValidateUint(0b1111, TomlUintFormat::Binary, "0b1111");
    ValidateUint(0b1010101, TomlUintFormat::Binary, "0b1010101");
    ValidateUint(0b01010101, TomlUintFormat::Binary, "0b1010101");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_writer, value_float, TomlWriterFixture)
{
    // Zeroes
    ValidateFloat(0.0, TomlFloatFormat::General, -1, "0");
    ValidateFloat(0.0, TomlFloatFormat::Fixed, -1, "0.000000");
    ValidateFloat(0.0, TomlFloatFormat::Exponent, -1, "0.000000e+00");

    // Fixed
    ValidateFloat(14.1345, TomlFloatFormat::Fixed, 6, "14.134500");
    ValidateFloat(-14.0001345, TomlFloatFormat::Fixed, 7, "-14.0001345");
    ValidateFloat(0.0001345, TomlFloatFormat::Fixed, 12, "0.000134500000");
    ValidateFloat(10.0, TomlFloatFormat::Fixed, 3, "10.000");

    // Exponent
    ValidateFloat(2000, TomlFloatFormat::Exponent, -1, "2.000000e+03");
    ValidateFloat(0.002, TomlFloatFormat::Exponent, 1, "2.0e-03");
    ValidateFloat(2, TomlFloatFormat::Exponent, 2, "2.00e+00");
    ValidateFloat(2, TomlFloatFormat::Exponent, 4, "2.0000e+00");
    ValidateFloat(0.2, TomlFloatFormat::Exponent, 1, "2.0e-01");
    ValidateFloat(-0.2, TomlFloatFormat::Exponent, 1, "-2.0e-01");
    ValidateFloat(1e123, TomlFloatFormat::Exponent, 1, "1.0e+123");
    ValidateFloat(1e-123, TomlFloatFormat::Exponent, 1, "1.0e-123");

    // Special float values
    ValidateFloat(Limits<double>::Infinity, TomlFloatFormat::General, -1, "inf");
    ValidateFloat(-Limits<double>::Infinity, TomlFloatFormat::General, -1, "-inf");
    ValidateFloat(Limits<double>::NaN, TomlFloatFormat::General, -1, "nan");
    ValidateFloat(-Limits<double>::NaN, TomlFloatFormat::General, -1, "-nan");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_writer, value_string_basic, TomlWriterFixture)
{
    // Basic encodings
    ValidateBasicString("", "\"\"");
    ValidateBasicString("test", "\"test\"");
    ValidateBasicString("\x01", "\"\\u0001\"");
    ValidateBasicString("é", "\"é\"");
    ValidateBasicString("test\xed\x9f\xbftest", "\"test\xed\x9f\xbftest\"");
    ValidateBasicString("test\xee\x80\x80test", "\"test\xee\x80\x80test\"");
    ValidateBasicString("test\xf4\x8f\xbf\xbftest", "\"test\xf4\x8f\xbf\xbftest\"");
    ValidateBasicString("Fuß", "\"Fuß\"");
    ValidateBasicString("😂", "\"😂\"");
    ValidateBasicString("汉语大字典", "\"汉语大字典\"");
    ValidateBasicString("辭源", "\"辭源\"");
    ValidateBasicString("பெண்டிரேம்", "\"பெண்டிரேம்\"");

    // Multiline basic strings
    ValidateBasicString("test\ntest", "\"\"\"\ntest\ntest\"\"\"");
    ValidateBasicString("test\b\t\n\f\r\n\"\\", "\"\"\"\ntest\\b\\t\n\\f\r\n\\\"\\\\\"\"\"");
    ValidateBasicString("test  \n   test \n   ", "\"\"\"\ntest  \n   test \n   \"\"\"");
    ValidateBasicString("test  xxx\nyyy\nzzz", "\"\"\"\ntest  xxx\nyyy\nzzz\"\"\"");

    // Escape
    ValidateBasicString("test\b\t\n\f\r\n\"\\", "\"\"\"\ntest\\b\\t\n\\f\r\n\\\"\\\\\"\"\"");
    ValidateBasicString("\x01\n", "\"\"\"\n\\u0001\n\"\"\"");
    ValidateBasicString("é\n", "\"\"\"\né\n\"\"\"");
    ValidateBasicString("test\xed\x9f\xbf\ntest", "\"\"\"\ntest\xed\x9f\xbf\ntest\"\"\"");
    ValidateBasicString("test\xee\x80\x80\ntest", "\"\"\"\ntest\xee\x80\x80\ntest\"\"\"");
    ValidateBasicString("test\xf4\x8f\xbf\xbf\ntest", "\"\"\"\ntest\xf4\x8f\xbf\xbf\ntest\"\"\"");
    ValidateBasicString("Fuß\n", "\"\"\"\nFuß\n\"\"\"");
    ValidateBasicString("😂\n", "\"\"\"\n😂\n\"\"\"");
    ValidateBasicString("汉语大字典\n", "\"\"\"\n汉语大字典\n\"\"\"");
    ValidateBasicString("辭源\n", "\"\"\"\n辭源\n\"\"\"");
    ValidateBasicString("பெண்டிரேம்\n", "\"\"\"\nபெண்டிரேம்\n\"\"\"");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_writer, value_string_literal, TomlWriterFixture)
{
    ValidateLiteralString("", "''");
    ValidateLiteralString("test", "'test'");
    ValidateLiteralString("  \t test \\test\"test", "'  \t test \\test\"test'");
    ValidateLiteralString("Fuß", "'Fuß'");
    ValidateLiteralString("😂", "'😂'");
    ValidateLiteralString("汉语大字典", "'汉语大字典'");
    ValidateLiteralString("辭源", "'辭源'");
    ValidateLiteralString("பெண்டிரேம்", "'பெண்டிரேம்'");

    ValidateLiteralString(" ' ", "'''\n ' '''");
    ValidateLiteralString(" '' ", "'''\n '' '''");
    ValidateLiteralString("' ", "'''\n' '''");
    ValidateLiteralString("'' ", "'''\n'' '''");
    ValidateLiteralString(" '", "'''\n ''''");
    ValidateLiteralString(" ''", "'''\n '''''");
    ValidateLiteralString("test\ntest", "'''\ntest\ntest'''");
    ValidateLiteralString("Fuß\n", "'''\nFuß\n'''");
    ValidateLiteralString("😂\n", "'''\n😂\n'''");
    ValidateLiteralString("汉语大字典\n", "'''\n汉语大字典\n'''");
    ValidateLiteralString("辭源\n", "'''\n辭源\n'''");
    ValidateLiteralString("பெண்டிரேம்\n", "'''\nபெண்டிரேம்\n'''");

    ValidateLiteralString("  test\ntest", "'''\n  test\ntest'''");
    ValidateLiteralString("test\ntest", "'''\ntest\ntest'''");

    ValidateLiteralString("'That,' she said, 'is still pointless.'", "'''\n'That,' she said, 'is still pointless.''''");
    ValidateLiteralString("''That,' she said, 'is still pointless.''", "'''\n''That,' she said, 'is still pointless.'''''");

    ValidateLiteralString("  \t test \\test\"test\\\n  test\\\n  test", "'''\n  \t test \\test\"test\\\n  test\\\n  test'''");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_writer, value_datetime, TomlWriterFixture)
{
    // Sun, 27 May 1979 07:32:00 GMT
    constexpr SystemTime Expected{ 296638320000ull * Milliseconds::Ratio };

    ValidateDateTime(Expected, TomlDateTimeFormat::OffsetUtc, "1979-05-27T07:32:00Z");
    ValidateDateTime(Expected + FromPeriod<Microseconds>(999999), TomlDateTimeFormat::OffsetUtc, "1979-05-27T07:32:00.999999000Z");

    ValidateDateTime(Expected, TomlDateTimeFormat::OffsetLocal, "1979-05-27T00:32:00-07:00");
    ValidateDateTime(Expected + FromPeriod<Microseconds>(999999), TomlDateTimeFormat::OffsetLocal, "1979-05-27T00:32:00.999999000-07:00");

    ValidateDateTime(Expected, TomlDateTimeFormat::Local, "1979-05-27T00:32:00");
    ValidateDateTime(Expected + FromPeriod<Microseconds>(999999), TomlDateTimeFormat::Local, "1979-05-27T00:32:00.999999000");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_writer, value_time, TomlWriterFixture)
{
    ValidateTime(FromPeriod<Hours>(7) + FromPeriod<Minutes>(32), "07:32:00");
    ValidateTime(FromPeriod<Minutes>(32) + FromPeriod<Microseconds>(999999), "00:32:00.999999000");
    ValidateTime(FromPeriod<Hours>(20) + FromPeriod<Minutes>(30) + FromPeriod<Seconds>(40), "20:30:40");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_writer, keys, TomlWriterFixture)
{
    // Bare keys
    ValidateKey("key", "key");
    ValidateKey("bare_key", "bare_key");
    ValidateKey("bare-key", "bare-key");
    ValidateKey("1234", "1234");
    ValidateKey("Fuß", "Fuß");
    ValidateKey("😂", "😂");
    ValidateKey("汉语大字典", "汉语大字典");
    ValidateKey("辭源", "辭源");
    ValidateKey("பெண்டிரேம்", "பெண்டிரேம்");
    ValidateKey("key_-23_-", "key_-23_-");

    // Basic string quoted keys
    ValidateKey("127.0.0.1", "\"127.0.0.1\"");
    ValidateKey("character encoding", "\"character encoding\"");
    ValidateKey("╠═╣", "\"╠═╣\"");
    ValidateKey("⋰∫∬∭⋱", "\"⋰∫∬∭⋱\"");
    ValidateKey("", "\"\"");

    // Whitespace
    ValidateKey("key\n\t123", "\"key\\n\\t123\"");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, toml_writer, complex_document, TomlWriterFixture)
{
    // Sun, 27 May 1979 07:32:00 GMT
    constexpr SystemTime ExpectedDateTime{ 296638320000ull * Milliseconds::Ratio };

    m_writer.Comment(" Copyright Chad Engler");
    m_writer.Table("boolean");
    m_writer.Key("bool1");
    m_writer.Bool(true);
    m_writer.Key("bool2");
    m_writer.Bool(false);
    m_writer.Table("integer");
    m_writer.Key("int1");
    m_writer.Uint(99);
    m_writer.Key("int2");
    m_writer.Int(-17);
    m_writer.Key("int3");
    m_writer.Uint(5349221);
    m_writer.Key("hex");
    m_writer.Uint(0xdeadbeef, TomlUintFormat::Hex);
    m_writer.Key("oct");
    m_writer.Uint(0755, TomlUintFormat::Octal);
    m_writer.Key("bin");
    m_writer.Uint(0b11010110, TomlUintFormat::Binary);
    m_writer.Table("float");
    m_writer.Key("flt1");
    m_writer.Float(3.1415);
    m_writer.Key("flt2");
    m_writer.Float(-0.01);
    m_writer.Key("flt3");
    m_writer.Float(5e+22, TomlFloatFormat::Exponent);
    m_writer.Key("flt4");
    m_writer.Float(1e06, TomlFloatFormat::Exponent);
    m_writer.Key("flt5");
    m_writer.Float(-2E-2, TomlFloatFormat::Exponent);
    m_writer.Key("sf1");
    m_writer.Float(Limits<double>::Infinity);
    m_writer.Key("sf2");
    m_writer.Float(-Limits<double>::Infinity);
    m_writer.Key("sf3");
    m_writer.Float(Limits<double>::NaN);
    m_writer.Key("sf4");
    m_writer.Float(-Limits<double>::NaN);
    m_writer.Table("string");
    m_writer.Key("str");
    m_writer.String("I'm a string. \"You can quote me\". Name\tJosé\nLocation\tSF.");
    m_writer.Key("str1");
    m_writer.String("Roses are red\nViolets are blue");
    m_writer.Key("str2");
    m_writer.String("Here are two quotation marks: \"\". Simple enough.");
    m_writer.Key("str3");
    m_writer.String("Here are three quotation marks: \"\"\".");
    m_writer.Key("str4");
    m_writer.String("Here are fifteen quotation marks: \"\"\"\"\"\"\"\"\"\"\"\"\"\"\".");
    m_writer.Key("str5");
    m_writer.String("C:\\Users\\nodejs\\templates", TomlStringFormat::Literal);
    m_writer.Key("str6");
    m_writer.String("\\\\ServerX\\admin$\\system32\\", TomlStringFormat::Literal);
    m_writer.Key("str7");
    m_writer.String("<\\i\\c*\\s*>", TomlStringFormat::Literal);
    m_writer.Key("str8");
    m_writer.String("The first newline is\ntrimmed in literal strings.\n   All other whitespace\n   is preserved.\n", TomlStringFormat::Literal);
    m_writer.Table("datetime");
    m_writer.Key("odt1");
    m_writer.DateTime(ExpectedDateTime);
    m_writer.Key("odt2");
    m_writer.DateTime((ExpectedDateTime + FromPeriod<Microseconds>(999999)));
    m_writer.Key("odt3");
    m_writer.DateTime(ExpectedDateTime, TomlDateTimeFormat::OffsetLocal);
    m_writer.Key("odt4");
    m_writer.DateTime((ExpectedDateTime + FromPeriod<Microseconds>(999999)), TomlDateTimeFormat::OffsetLocal);
    m_writer.Key("ldt1");
    m_writer.DateTime(ExpectedDateTime, TomlDateTimeFormat::Local);
    m_writer.Key("ldt2");
    m_writer.DateTime((ExpectedDateTime + FromPeriod<Microseconds>(999999)), TomlDateTimeFormat::Local);
    m_writer.Table("time");
    m_writer.Key("lt1");
    m_writer.Time((FromPeriod<Hours>(7) + FromPeriod<Minutes>(32)));
    m_writer.Key("lt2");
    m_writer.Time((FromPeriod<Minutes>(32) + FromPeriod<Microseconds>(999999)));
    m_writer.Table("array");
    m_writer.Key("integers");
    m_writer.StartArray();
    m_writer.Uint(1);
    m_writer.Uint(2);
    m_writer.Uint(3);
    m_writer.EndArray();
    m_writer.Key("colors");
    m_writer.StartArray();
    m_writer.String("red");
    m_writer.String("yellow");
    m_writer.String("green");
    m_writer.EndArray();
    m_writer.Key("nested_arrays_of_ints");
    m_writer.StartArray();
    m_writer.StartArray();
    m_writer.Uint(1);
    m_writer.Uint(2);
    m_writer.EndArray();
    m_writer.StartArray();
    m_writer.Uint(3);
    m_writer.Uint(4);
    m_writer.Uint(5);
    m_writer.EndArray();
    m_writer.EndArray();
    m_writer.Key("contributors");
    m_writer.StartArray();
    m_writer.String("Foo Bar <foo@example.com>");
    m_writer.StartInlineTable();
    m_writer.Key("name");
    m_writer.String("Baz Qux");
    m_writer.Key("email");
    m_writer.String("bazqux@example.com");
    m_writer.Key("url");
    m_writer.String("https://example.com/bazqux");
    m_writer.EndInlineTable();
    m_writer.EndArray();
    StringView tableKeys[] = { "dog", "tater.man" };
    m_writer.Table(tableKeys);
    StringView valueKeys[] = { "type", "name" };
    m_writer.Key(valueKeys);
    m_writer.String("pug");
    m_writer.Table("product", true);
    m_writer.Key("name");
    m_writer.String("Hammer");
    m_writer.Key("sku");
    m_writer.Uint(738594937);
    m_writer.Table("product", true);
    m_writer.Table("product", true);
    m_writer.Key("name");
    m_writer.String("Nail");
    m_writer.Key("sku");
    m_writer.Uint(284758393);
    m_writer.Key("color");
    m_writer.String("gray");
    m_writer.Key("points");
    m_writer.StartArray();
    m_writer.StartInlineTable();
    m_writer.Key("x");
    m_writer.Int(1);
    m_writer.Key("y");
    m_writer.Int(2);
    m_writer.Key("z");
    m_writer.Int(3);
    m_writer.EndInlineTable();
    m_writer.StartInlineTable();
    m_writer.Key("x");
    m_writer.Int(7);
    m_writer.Key("y");
    m_writer.Int(8);
    m_writer.Key("z");
    m_writer.Int(9);
    m_writer.EndInlineTable();
    m_writer.StartInlineTable();
    m_writer.Key("x");
    m_writer.Int(2);
    m_writer.Key("y");
    m_writer.Int(4);
    m_writer.Key("z");
    m_writer.Int(8);
    m_writer.EndInlineTable();
    m_writer.EndArray();

    Validate(R"(# Copyright Chad Engler
[boolean]
bool1 = true
bool2 = false
[integer]
int1 = 99
int2 = -17
int3 = 5349221
hex = 0xdeadbeef
oct = 0o755
bin = 0b11010110
[float]
flt1 = 3.1415
flt2 = -0.01
flt3 = 5.000000e+22
flt4 = 1.000000e+06
flt5 = -2.000000e-02
sf1 = inf
sf2 = -inf
sf3 = nan
sf4 = -nan
[string]
str = """
I'm a string. \"You can quote me\". Name\tJosé
Location\tSF."""
str1 = """
Roses are red
Violets are blue"""
str2 = "Here are two quotation marks: \"\". Simple enough."
str3 = "Here are three quotation marks: \"\"\"."
str4 = "Here are fifteen quotation marks: \"\"\"\"\"\"\"\"\"\"\"\"\"\"\"."
str5 = 'C:\Users\nodejs\templates'
str6 = '\\ServerX\admin$\system32\'
str7 = '<\i\c*\s*>'
str8 = '''
The first newline is
trimmed in literal strings.
   All other whitespace
   is preserved.
'''
[datetime]
odt1 = 1979-05-27T07:32:00Z
odt2 = 1979-05-27T07:32:00.999999000Z
odt3 = 1979-05-27T00:32:00-07:00
odt4 = 1979-05-27T00:32:00.999999000-07:00
ldt1 = 1979-05-27T00:32:00
ldt2 = 1979-05-27T00:32:00.999999000
[time]
lt1 = 07:32:00
lt2 = 00:32:00.999999000
[array]
integers = [1, 2, 3]
colors = ["red", "yellow", "green"]
nested_arrays_of_ints = [[1, 2], [3, 4, 5]]
contributors = ["Foo Bar <foo@example.com>", {name = "Baz Qux", email = "bazqux@example.com", url = "https://example.com/bazqux"}]
[dog."tater.man"]
type.name = "pug"
[[product]]
name = "Hammer"
sku = 738594937
[[product]]
[[product]]
name = "Nail"
sku = 284758393
color = "gray"
points = [{x = 1, y = 2, z = 3}, {x = 7, y = 8, z = 9}, {x = 2, y = 4, z = 8}])");
}
