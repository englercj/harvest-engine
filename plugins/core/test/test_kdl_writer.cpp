// Copyright Chad Engler

#include "he/core/kdl_writer.h"

#include "he/core/test.h"
#include "he/core/types.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
class KdlWriterFixture : public TestFixture
{
public:
    void Validate(StringView expected)
    {
        HE_EXPECT_EQ(m_output, expected);
    }

    template <typename F>
    void ValidateValue(StringView expected, F&& func)
    {
        m_writer.Clear();
        m_writer.Node("node");
        func();

        String doc = "node ";
        doc += expected;
        Validate(doc);
    }

    void ValidateBool(bool input, StringView expected)
    {
        ValidateValue(expected, [&]() { m_writer.Argument(input); });
    }

    void ValidateInt(int64_t input, KdlIntFormat format, StringView expected)
    {
        ValidateValue(expected, [&]() { m_writer.Argument(input, {}, format); });
    }

    void ValidateUint(uint64_t input, KdlIntFormat format, StringView expected)
    {
        ValidateValue(expected, [&]() { m_writer.Argument(input, {}, format); });
    }

    void ValidateFloat(double input, KdlFloatFormat format, int32_t precision, StringView expected)
    {
        ValidateValue(expected, [&]() { m_writer.Argument(input, {}, format, precision); });
    }

    void ValidateString(StringView input, StringView expected)
    {
        ValidateValue(expected, [&]() { m_writer.Argument(input, {}, KdlStringFormat::Escaped); });
    }

    void ValidateRawString(StringView input, StringView expected)
    {
        ValidateValue(expected, [&]() { m_writer.Argument(input, {}, KdlStringFormat::Raw); });
    }

    String m_output;
    KdlWriter m_writer{ m_output };
};

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_writer, node_empty, KdlWriterFixture)
{
    m_writer.Node("node");
    Validate("node");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_writer, node_args, KdlWriterFixture)
{
    m_writer.Node("node");
    m_writer.Argument(true);
    m_writer.Argument(-123);
    m_writer.Argument(123u);
    m_writer.Argument(5.0);
    m_writer.Argument("arg");
    m_writer.Argument(nullptr);
    Validate("node #true -123 123 5.0 arg #null");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_writer, node_props, KdlWriterFixture)
{
    m_writer.Node("node");
    m_writer.Property("bool", true);
    m_writer.Property("int", -123);
    m_writer.Property("uint", 123u);
    m_writer.Property("float", 5.0);
    m_writer.Property("str", "arg");
    m_writer.Property("nullptr", nullptr);
    Validate("node bool=#true int=-123 uint=123 float=5.0 str=arg nullptr=#null");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_writer, node_children_empty, KdlWriterFixture)
{
    m_writer.Node("node");
    m_writer.StartNodeChildren();
    m_writer.EndNodeChildren();
    Validate("node {\n}\n");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_writer, node_children, KdlWriterFixture)
{
    m_writer.Node("node");
    m_writer.StartNodeChildren();
    m_writer.Node("child");
    m_writer.EndNodeChildren();
    Validate("node {\n    child\n}\n");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_writer, value_boolean, KdlWriterFixture)
{
    ValidateBool(true, "#true");
    ValidateBool(false, "#false");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_writer, value_int, KdlWriterFixture)
{
    // Decimal
    ValidateInt(0, KdlIntFormat::Decimal, "0");
    ValidateInt(1, KdlIntFormat::Decimal, "1");
    ValidateInt(456789, KdlIntFormat::Decimal, "456789");
    ValidateInt(Limits<int64_t>::Max, KdlIntFormat::Decimal, "9223372036854775807");
    ValidateInt(Limits<uint64_t>::Max, KdlIntFormat::Decimal, "18446744073709551615");

    // Hex
    ValidateInt(0x0, KdlIntFormat::Hex, "0x0");
    ValidateInt(0xdeadbeef, KdlIntFormat::Hex, "0xdeadbeef");
    ValidateInt(Limits<uint32_t>::Max, KdlIntFormat::Hex, "0xffffffff");
    ValidateInt(Limits<uint64_t>::Max, KdlIntFormat::Hex, "0xffffffffffffffff");

    // Octal
    ValidateInt(0, KdlIntFormat::Octal, "0o0");
    ValidateInt(0755, KdlIntFormat::Octal, "0o755");
    ValidateInt(0644, KdlIntFormat::Octal, "0o644");

    // Bin
    ValidateInt(0, KdlIntFormat::Binary, "0b0");
    ValidateInt(0b1, KdlIntFormat::Binary, "0b1");
    ValidateInt(0b11, KdlIntFormat::Binary, "0b11");
    ValidateInt(0b111, KdlIntFormat::Binary, "0b111");
    ValidateInt(0b1111, KdlIntFormat::Binary, "0b1111");
    ValidateInt(0b1010101, KdlIntFormat::Binary, "0b1010101");
    ValidateInt(0b01010101, KdlIntFormat::Binary, "0b1010101");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_writer, value_uint, KdlWriterFixture)
{
    // Decimal
    ValidateUint(0, KdlIntFormat::Decimal, "0");
    ValidateUint(1, KdlIntFormat::Decimal, "1");
    ValidateUint(456789, KdlIntFormat::Decimal, "456789");
    ValidateUint(Limits<int64_t>::Max, KdlIntFormat::Decimal, "9223372036854775807");
    ValidateUint(Limits<uint64_t>::Max, KdlIntFormat::Decimal, "18446744073709551615");

    // Hex
    ValidateUint(0x0, KdlIntFormat::Hex, "0x0");
    ValidateUint(0xdeadbeef, KdlIntFormat::Hex, "0xdeadbeef");
    ValidateUint(Limits<uint32_t>::Max, KdlIntFormat::Hex, "0xffffffff");
    ValidateUint(Limits<uint64_t>::Max, KdlIntFormat::Hex, "0xffffffffffffffff");

    // Octal
    ValidateUint(0, KdlIntFormat::Octal, "0o0");
    ValidateUint(0755, KdlIntFormat::Octal, "0o755");
    ValidateUint(0644, KdlIntFormat::Octal, "0o644");

    // Bin
    ValidateUint(0, KdlIntFormat::Binary, "0b0");
    ValidateUint(0b1, KdlIntFormat::Binary, "0b1");
    ValidateUint(0b11, KdlIntFormat::Binary, "0b11");
    ValidateUint(0b111, KdlIntFormat::Binary, "0b111");
    ValidateUint(0b1111, KdlIntFormat::Binary, "0b1111");
    ValidateUint(0b1010101, KdlIntFormat::Binary, "0b1010101");
    ValidateUint(0b01010101, KdlIntFormat::Binary, "0b1010101");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_writer, value_float, KdlWriterFixture)
{
    // Zeroes
    ValidateFloat(0.0, KdlFloatFormat::General, -1, "0");
    ValidateFloat(0.0, KdlFloatFormat::Fixed, -1, "0.000000");
    ValidateFloat(0.0, KdlFloatFormat::Exponent, -1, "0.000000e+00");

    // Fixed
    ValidateFloat(14.1345, KdlFloatFormat::Fixed, 6, "14.134500");
    ValidateFloat(-14.0001345, KdlFloatFormat::Fixed, 7, "-14.0001345");
    ValidateFloat(0.0001345, KdlFloatFormat::Fixed, 12, "0.000134500000");
    ValidateFloat(10.0, KdlFloatFormat::Fixed, 3, "10.000");

    // Exponent
    ValidateFloat(2000, KdlFloatFormat::Exponent, -1, "2.000000e+03");
    ValidateFloat(0.002, KdlFloatFormat::Exponent, 1, "2.0e-03");
    ValidateFloat(2, KdlFloatFormat::Exponent, 2, "2.00e+00");
    ValidateFloat(2, KdlFloatFormat::Exponent, 4, "2.0000e+00");
    ValidateFloat(0.2, KdlFloatFormat::Exponent, 1, "2.0e-01");
    ValidateFloat(-0.2, KdlFloatFormat::Exponent, 1, "-2.0e-01");
    ValidateFloat(1e123, KdlFloatFormat::Exponent, 1, "1.0e+123");
    ValidateFloat(1e-123, KdlFloatFormat::Exponent, 1, "1.0e-123");

    // Special float values
    ValidateFloat(Limits<double>::Infinity, KdlFloatFormat::General, -1, "#inf");
    ValidateFloat(-Limits<double>::Infinity, KdlFloatFormat::General, -1, "#-inf");
    ValidateFloat(Limits<double>::NaN, KdlFloatFormat::General, -1, "#nan");
    ValidateFloat(-Limits<double>::NaN, KdlFloatFormat::General, -1, "#nan");
    ValidateFloat(Limits<double>::SignalingNaN, KdlFloatFormat::General, -1, "#nan");
    ValidateFloat(-Limits<double>::SignalingNaN, KdlFloatFormat::General, -1, "#nan");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_writer, value_string_escaped, KdlWriterFixture)
{
    // Identifier strings
    ValidateString("test", "test");
    ValidateString("é", "é");
    ValidateString("bare_key", "bare_key");
    ValidateString("bare-key", "bare-key");
    ValidateString("Fuß", "Fuß");
    ValidateString("😂", "😂");
    ValidateString("key_-23_-", "key_-23_-");
    ValidateString("-.stuff", "-.stuff");
    ValidateString(".stuff", ".stuff");
    ValidateString("-stuff", "-stuff");
    ValidateString("Fuß", "Fuß");
    ValidateString("😂", "😂");
    ValidateString("汉语大字典", "汉语大字典");
    ValidateString("辭源", "辭源");
    ValidateString("பெண்டிரேம்", "பெண்டிரேம்");

    // Quoted strings
    ValidateString("", "\"\"");
    ValidateString("1234", "\"1234\"");
    ValidateString("\x01", "\"\\u0001\"");
    ValidateString("test\xed\x9f\xbftest", "\"test\xed\x9f\xbftest\"");
    ValidateString("test\xee\x80\x80test", "\"test\xee\x80\x80test\"");
    ValidateString("test\xf4\x8f\xbf\xbftest", "\"test\xf4\x8f\xbf\xbftest\"");
    ValidateString("127.0.0.1", "\"127.0.0.1\"");
    ValidateString("character encoding", "\"character encoding\"");
    ValidateString("╠═╣", "\"╠═╣\"");
    ValidateString("⋰∫∬∭⋱", "\"⋰∫∬∭⋱\"");
    ValidateString("C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe", "\"C:\\\\Program Files (x86)\\\\Microsoft\\\\Edge\\\\Application\\\\msedge.exe\"");

    // Multiline quoted strings
    ValidateString("test\ntest", "\"\ntest\ntest\n\"");
    ValidateString("test\b\t\n\f\r\n\"\\", "\"\ntest\\b\\t\n\\f\r\n\\\"\\\\\n\"");
    ValidateString("test  \n   test \n   ", "\"\ntest  \n   test \n   \n\"");
    ValidateString("test  xxx\nyyy\nzzz", "\"\ntest  xxx\nyyy\nzzz\n\"");
    ValidateString("test\b\t\n\f\r\n\"\\", "\"\ntest\\b\\t\n\\f\r\n\\\"\\\\\n\"");
    ValidateString("\x01\n", "\"\n\\u0001\n\n\"");
    ValidateString("é\n", "\"\né\n\n\"");
    ValidateString("test\xed\x9f\xbf\ntest", "\"\ntest\xed\x9f\xbf\ntest\n\"");
    ValidateString("test\xee\x80\x80\ntest", "\"\ntest\xee\x80\x80\ntest\n\"");
    ValidateString("test\xf4\x8f\xbf\xbf\ntest", "\"\ntest\xf4\x8f\xbf\xbf\ntest\n\"");
    ValidateString("Fuß\n", "\"\nFuß\n\n\"");
    ValidateString("😂\n", "\"\n😂\n\n\"");
    ValidateString("汉语大字典\n", "\"\n汉语大字典\n\n\"");
    ValidateString("辭源\n", "\"\n辭源\n\n\"");
    ValidateString("பெண்டிரேம்\n", "\"\nபெண்டிரேம்\n\n\"");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_writer, value_string_raw, KdlWriterFixture)
{
    ValidateRawString("", "#\"\"#");
    ValidateRawString("test", "#\"test\"#");
    ValidateRawString("  \t test \\test\"test", "#\"  \t test \\test\"test\"#");
    ValidateRawString("Fuß", "#\"Fuß\"#");
    ValidateRawString("😂", "#\"😂\"#");
    ValidateRawString("汉语大字典", "#\"汉语大字典\"#");
    ValidateRawString("辭源", "#\"辭源\"#");
    ValidateRawString("பெண்டிரேம்", "#\"பெண்டிரேம்\"#");
    ValidateRawString("C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe", "#\"C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe\"#");
    ValidateRawString("\"That,\" she said, \"is still pointless.\"", "#\"\"That,\" she said, \"is still pointless.\"\"#");
    ValidateRawString("\"\"That,\" she said, \"is still pointless.\"\"", "#\"\"\"That,\" she said, \"is still pointless.\"\"\"#");

    ValidateRawString(" \" ", "#\" \" \"#");
    ValidateRawString(" \"\" ", "#\" \"\" \"#");
    ValidateRawString("\" ", "#\"\" \"#");
    ValidateRawString("\"\" ", "#\"\"\" \"#");
    ValidateRawString(" \"", "#\" \"\"#");
    ValidateRawString(" \"\"", "#\" \"\"\"#");
    ValidateRawString("test\ntest", "#\"\ntest\ntest\n\"#");
    ValidateRawString("Fuß\n", "#\"\nFuß\n\n\"#");
    ValidateRawString("😂\n", "#\"\n😂\n\n\"#");
    ValidateRawString("汉语大字典\n", "#\"\n汉语大字典\n\n\"#");
    ValidateRawString("辭源\n", "#\"\n辭源\n\n\"#");
    ValidateRawString("பெண்டிரேம்\n", "#\"\nபெண்டிரேம்\n\n\"#");

    ValidateRawString("  test\ntest", "#\"\n  test\ntest\n\"#");
    ValidateRawString("test\ntest", "#\"\ntest\ntest\n\"#");
    ValidateRawString("  \t test \\test\"test\\\n  test\\\n  test", "#\"\n  \t test \\test\"test\\\n  test\\\n  test\n\"#");
}

// ------------------------------------------------------------------------------------------------
HE_TEST_F(core, kdl_writer, document, KdlWriterFixture)
{
    m_writer.Comment("Copyright Chad Engler");
    m_writer.Node("boolean");
    m_writer.Argument(true);
    m_writer.Argument(false);

    m_writer.Node("null");
    m_writer.Argument(nullptr);

    m_writer.Node("integers");
    m_writer.StartNodeChildren();
    m_writer.Node("decimal");
    m_writer.Property("int_min", Limits<int32_t>::Min, "i32");
    m_writer.Property("int_max", Limits<int32_t>::Max, "i32");
    m_writer.Node("hex");
    m_writer.Property("uint_min", Limits<uint32_t>::Min, "u32", KdlIntFormat::Hex);
    m_writer.Property("uint_max", Limits<uint32_t>::Max, "u32", KdlIntFormat::Hex);
    m_writer.Node("octal");
    m_writer.Property("int_min", Limits<int16_t>::Min, "i16", KdlIntFormat::Octal);
    m_writer.Property("int_max", Limits<int16_t>::Max, "i16", KdlIntFormat::Octal);
    m_writer.Node("binary");
    m_writer.Property("uint_min", Limits<uint8_t>::Min, "u8", KdlIntFormat::Binary);
    m_writer.Property("uint_max", Limits<uint8_t>::Max, "u8", KdlIntFormat::Binary);
    m_writer.EndNodeChildren();

    m_writer.Node("floats");
    m_writer.StartNodeChildren();
    m_writer.Node("general");
    m_writer.Property("zero", 0.0, "f64");
    m_writer.Property("positive", 3.1415, "f64");
    m_writer.Property("negative", -0.01, "f64");
    m_writer.Property("large", 5e+22, "f64");
    m_writer.Property("small", 5e-22, "f64");
    m_writer.Node("fixed");
    m_writer.Property("zero", 0.0, "f64", KdlFloatFormat::Fixed, 6);
    m_writer.Property("positive", 3.1415, "f64", KdlFloatFormat::Fixed, 6);
    m_writer.Property("negative", -0.01, "f64", KdlFloatFormat::Fixed, 6);
    m_writer.Property("large", 5e+22, "f64", KdlFloatFormat::Fixed, 6);
    m_writer.Property("small", 5e-22, "f64", KdlFloatFormat::Fixed, 6);
    m_writer.Node("exponent");
    m_writer.Property("zero", 0.0, "f64", KdlFloatFormat::Exponent, 6);
    m_writer.Property("positive", 3.1415, "f64", KdlFloatFormat::Exponent, 6);
    m_writer.Property("negative", -0.01, "f64", KdlFloatFormat::Exponent, 6);
    m_writer.Property("large", 5e+22, "f64", KdlFloatFormat::Exponent, 6);
    m_writer.Property("small", 5e-22, "f64", KdlFloatFormat::Exponent, 6);
    m_writer.Node("special");
    m_writer.Property("inf", Limits<double>::Infinity, "f64");
    m_writer.Property("ninf", -Limits<double>::Infinity, "f64");
    m_writer.Property("nan", Limits<double>::NaN, "f64");
    m_writer.Property("snan", Limits<double>::SignalingNaN, "f64");
    m_writer.EndNodeChildren();

    m_writer.Node("strings");
    m_writer.StartNodeChildren();
    m_writer.Node("escaped");
    m_writer.StartNodeChildren();
    m_writer.Node("str1");
    m_writer.Argument("I'm a string. \"You can quote me\".\nName\tJosé\nLocation\tSF.");
    m_writer.Node("str2");
    m_writer.Argument("Roses are red\nViolets are blue");
    m_writer.Node("str3");
    m_writer.Argument("Here are two quotation marks: \"\". Simple enough.");
    m_writer.Node("str4");
    m_writer.Argument("Here are fifteen quotation marks: \"\"\"\"\"\"\"\"\"\"\"\"\"\"\".");
    m_writer.Node("str5");
    m_writer.Argument("\\\\ServerX\\admin$\\system32\\");
    m_writer.Node("str6");
    m_writer.Argument("<\\i\\c*\\s*>");
    m_writer.EndNodeChildren();
    m_writer.Node("raw");
    m_writer.StartNodeChildren();
    m_writer.Node("str1");
    m_writer.Argument("I'm a string. \"You can quote me\". Name\tJosé\nLocation\tSF.", {}, KdlStringFormat::Raw);
    m_writer.Node("str2");
    m_writer.Argument("Roses are red\nViolets are blue", {}, KdlStringFormat::Raw);
    m_writer.Node("str3");
    m_writer.Argument("Here are two quotation marks: \"\". Simple enough.", {}, KdlStringFormat::Raw);
    m_writer.Node("str4");
    m_writer.Argument("Here are fifteen quotation marks: \"\"\"\"\"\"\"\"\"\"\"\"\"\"\".", {}, KdlStringFormat::Raw);
    m_writer.Node("str5");
    m_writer.Argument("\\\\ServerX\\admin$\\system32\\", {}, KdlStringFormat::Raw);
    m_writer.Node("str6");
    m_writer.Argument("<\\i\\c*\\s*>", {}, KdlStringFormat::Raw);
    m_writer.EndNodeChildren();
    m_writer.EndNodeChildren();

    Validate(R"(// Copyright Chad Engler
boolean #true #false
"null" #null
integers {
    decimal int_min=(i32)-2147483648 int_max=(i32)2147483647
    hex uint_min=(u32)0x00000000 uint_max=(u32)0xffffffff
    octal int_min=(i16)-0o100000 int_max=(i16)0o77777
    binary uint_min=(u8)0b00000000 uint_max=(u8)0b11111111
}
floats {
    general zero=0 positive=3.1415 negative=-0.01 large=5.0e+22 small=5.0e-22
    fixed zero=0.000000 positive=3.1415 negative=-0.010000 large=50000000000000000000000.000000 small=0.000000
    exponent zero=0.000000e+00 positive=3.1415 negative=-1.000000e-02 large=5.000000e+22 small=5.000000e-22
    special inf=#inf ninf=#-inf nan=#nan snan=#nan
}
strings {
    escaped {
        str1 "
I'm a string. \"You can quote me\".
Name	José
Location	SF.
"
        str2 "
Roses are red
Violets are blue"
        str3 "Here are two quotation marks: \"\". Simple enough."
        str4 "Here are fifteen quotation marks: \"\"\"\"\"\"\"\"\"\"\"\"\"\"\""
        str5 "\\\\ServerX\\admin$\\system32\\"
        str6 "<\\i\\c*\\s*>"
    }
    raw {
        str1 #"
I'm a string. "You can quote me".
Name	José
Location	SF.
"#
        str2 #"
Roses are red
Violets are blue
"#
        str3 #"Here are two quotation marks: "". Simple enough."#
        str4 #"Here are fifteen quotation marks: """"""""""""""""#
        str5 #"\\ServerX\admin$\system32\"#
        str6 #"<\i\c*\s*>"#
    }
}
)");
}
