// Copyright Chad Engler

#include "he/core/toml_writer.h"

#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/core/string_view.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, toml_writer, test)
{
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, toml_writer, complex_document)
{
    // Sun, 27 May 1979 07:32:00 GMT
    constexpr SystemTime ExpectedDateTime{ 296638320000ull * Milliseconds::Ratio };

    String output;
    TomlWriter writer(output);

    writer.Comment(" Copyright Chad Engler");
    writer.Table("boolean");
    writer.Key("bool1");
    writer.Bool(true);
    writer.Key("bool2");
    writer.Bool(false);
    writer.Table("integer");
    writer.Key("int1");
    writer.Uint(99);
    writer.Key("int2");
    writer.Int(-17);
    writer.Key("int3");
    writer.Uint(5349221);
    writer.Key("hex");
    writer.Uint(0xdeadbeef, TomlIntFormat::Hex);
    writer.Key("oct");
    writer.Uint(0755, TomlIntFormat::Octal);
    writer.Key("bin");
    writer.Uint(0b11010110, TomlIntFormat::Binary);
    writer.Table("float");
    writer.Key("flt1");
    writer.Float(3.1415);
    writer.Key("flt2");
    writer.Float(-0.01);
    writer.Key("flt3");
    writer.Float(5e+22, TomlFloatFormat::Exponent);
    writer.Key("flt4");
    writer.Float(1e06, TomlFloatFormat::Exponent);
    writer.Key("flt5");
    writer.Float(-2E-2, TomlFloatFormat::Exponent);
    writer.Key("sf1");
    writer.Float(Limits<double>::Infinity);
    writer.Key("sf2");
    writer.Float(-Limits<double>::Infinity);
    writer.Key("sf3");
    writer.Float(Limits<double>::NaN);
    writer.Key("sf4");
    writer.Float(-Limits<double>::NaN);
    writer.Table("string");
    writer.Key("str");
    writer.String("I'm a string. \"You can quote me\". Name\tJos\xE9\nLocation\tSF.");
    writer.Key("str1");
    writer.String("Roses are red\nViolets are blue");
    writer.Key("str2");
    writer.String("Here are two quotation marks: \"\". Simple enough.");
    writer.Key("str3");
    writer.String("Here are three quotation marks: \"\"\".");
    writer.Key("str4");
    writer.String("Here are fifteen quotation marks: \"\"\"\"\"\"\"\"\"\"\"\"\"\"\".");
    writer.Key("str5");
    writer.String("C:\\Users\\nodejs\\templates", TomlStringFormat::Literal);
    writer.Key("str6");
    writer.String("\\\\ServerX\\admin$\\system32\\", TomlStringFormat::Literal);
    writer.Key("str7");
    writer.String("<\\i\\c*\\s*>", TomlStringFormat::Literal);
    writer.Key("str8");
    writer.String("The first newline is\ntrimmed in literal strings.\n   All other whitespace\n   is preserved.\n", TomlStringFormat::Literal);
    // TODO: set the local timezone to -07:00 for these to pass
    writer.Table("datetime");
    writer.Key("odt1");
    writer.DateTime(ExpectedDateTime);
    writer.Key("odt2");
    writer.DateTime((ExpectedDateTime + FromPeriod<Microseconds>(999999)));
    writer.Key("odt3");
    writer.DateTime(ExpectedDateTime, TomlDateTimeFormat::Local);
    writer.Key("odt4");
    writer.DateTime((ExpectedDateTime + FromPeriod<Microseconds>(999999)), TomlDateTimeFormat::Local);
    writer.Table("time");
    writer.Key("lt1");
    writer.Time((FromPeriod<Hours>(7) + FromPeriod<Minutes>(32)));
    writer.Key("lt2");
    writer.Time((FromPeriod<Minutes>(32) + FromPeriod<Microseconds>(999999)));
    writer.Table("array");
    writer.Key("integers");
    writer.StartArray();
    writer.Uint(1);
    writer.Uint(2);
    writer.Uint(3);
    writer.EndArray();
    writer.Key("colors");
    writer.StartArray();
    writer.String("red");
    writer.String("yellow");
    writer.String("green");
    writer.EndArray();
    writer.Key("nested_arrays_of_ints");
    writer.StartArray();
    writer.StartArray();
    writer.Uint(1);
    writer.Uint(2);
    writer.EndArray();
    writer.StartArray();
    writer.Uint(3);
    writer.Uint(4);
    writer.Uint(5);
    writer.EndArray();
    writer.EndArray();
    writer.Key("contributors");
    writer.StartArray();
    writer.String("Foo Bar <foo@example.com>");
    writer.StartInlineTable();
    writer.Key("name");
    writer.String("Baz Qux");
    writer.Key("email");
    writer.String("bazqux@example.com");
    writer.Key("url");
    writer.String("https://example.com/bazqux");
    writer.EndInlineTable();
    writer.EndArray();
    StringView tableKeys[] = { "dog", "tater.man" };
    writer.Table(tableKeys);
    StringView valueKeys[] = { "type", "name" };
    writer.Key(valueKeys);
    writer.String("pug");
    writer.Table("product", true);
    writer.Key("name");
    writer.String("Hammer");
    writer.Key("sku");
    writer.Uint(738594937);
    writer.Table("product", true);
    writer.Table("product", true);
    writer.Key("name");
    writer.String("Nail");
    writer.Key("sku");
    writer.Uint(284758393);
    writer.Key("color");
    writer.String("gray");
    writer.Key("points");
    writer.StartArray();
    writer.StartInlineTable();
    writer.Key("x");
    writer.Int(1);
    writer.Key("y");
    writer.Int(2);
    writer.Key("z");
    writer.Int(3);
    writer.EndInlineTable();
    writer.StartInlineTable();
    writer.Key("x");
    writer.Int(7);
    writer.Key("y");
    writer.Int(8);
    writer.Key("z");
    writer.Int(9);
    writer.EndInlineTable();
    writer.StartInlineTable();
    writer.Key("x");
    writer.Int(2);
    writer.Key("y");
    writer.Int(4);
    writer.Key("z");
    writer.Int(8);
    writer.EndInlineTable();
    writer.EndArray();

    const StringView expected = R"(#  Copyright Chad Engler
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
I'm a string. \"You can quote me\". Name\tJos\xE9
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
[time]
lt1 = 07:32:00.000000000
lt2 = 07:32:00.999999000
[array]
integers = [1, 2, 3]
colors = ["red", "yellow", "green"]
nested_arrays_of_ints = [[1, 2], [3, 4, 5]]
contributors = ["Foo Bar <foo@example.com>", {name = "Baz Qux", email = "bazqux@example.com", url = "httpos://example.com/bazqux"}]
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
points = [{x = 1, y = 2, z = 3}, {x = 7, y = 8, z = 9}, {x = 2, y = 4, z = 8}]
)";

    HE_EXPECT_EQ(output, expected);
}
