// Copyright Chad Engler

// TODO: Negative testing for invalid documents

#include "fixtures.h"

#include "he/core/toml_document.h"

#include "he/core/clock_fmt.h"
#include "he/core/limits.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, toml_document, Construct)
{
    TomlDocument doc;
    HE_EXPECT_EQ_PTR(&doc.GetAllocator(), &Allocator::GetDefault());
    HE_EXPECT(doc.Root().IsTable());
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, toml_document, Read)
{
    TomlDocument doc;
    const TomlReadResult result = doc.Read("a = true\nb = 50\n[c]\nd = \"yes\"\n[[e]]\nf = true\n[[e]]\nf = false");
    HE_EXPECT(result);
    HE_EXPECT(doc.Root().IsTable());

    HE_EXPECT(doc.Contains("a"));
    HE_EXPECT(doc["a"].IsBool());
    HE_EXPECT_EQ(doc["a"].Bool(), true);

    HE_EXPECT(doc.Contains("b"));
    HE_EXPECT(doc["b"].IsUint());
    HE_EXPECT_EQ(doc["b"].Uint(), 50);

    HE_EXPECT(doc.Contains("c"));
    HE_EXPECT(doc["c"].IsTable());
    HE_EXPECT(doc["c"]["d"].IsString());
    HE_EXPECT_EQ(doc["c"]["d"].String(), "yes");

    HE_EXPECT(doc.Contains("e"));
    HE_EXPECT(doc["e"].IsArray());
    HE_EXPECT_EQ(doc["e"].Array().Size(), 2);

    TomlValue& e0 = doc["e"][0];
    HE_EXPECT(e0.IsTable());
    HE_EXPECT(e0.Table().Contains("f"));
    HE_EXPECT(e0["f"].IsBool());
    HE_EXPECT_EQ(e0["f"].Bool(), true);

    TomlValue& e1 = doc["e"][1];
    HE_EXPECT(e1.IsTable());
    HE_EXPECT(e1.Table().Contains("f"));
    HE_EXPECT(e1["f"].IsBool());
    HE_EXPECT_EQ(e1["f"].Bool(), false);

    HE_EXPECT(!doc.Contains("d"));
    HE_EXPECT(!doc.Contains("f"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, toml_document, Read_Complex)
{
    // Sun, 27 May 1979 07:32:00 GMT
    constexpr SystemTime ExpectedDateTime{ 296638320000ull * Milliseconds::Ratio };

    const StringView tomlDoc = GetTestTomlDocument();
    TomlDocument doc;
    const TomlReadResult result = doc.Read(tomlDoc);
    HE_EXPECT(result);
    HE_EXPECT(doc.Root().IsTable());

    HE_EXPECT(doc.Contains("boolean"));
    HE_EXPECT(doc["boolean"].IsTable());
    HE_EXPECT(doc["boolean"].Contains("bool1"));
    HE_EXPECT(doc["boolean"]["bool1"].IsBool());
    HE_EXPECT_EQ(doc["boolean"]["bool1"].Bool(), true);
    HE_EXPECT(doc["boolean"].Contains("bool2"));
    HE_EXPECT(doc["boolean"]["bool2"].IsBool());
    HE_EXPECT_EQ(doc["boolean"]["bool2"].Bool(), false);

    HE_EXPECT(doc.Contains("integer"));
    HE_EXPECT(doc["integer"].IsTable());
    HE_EXPECT(doc["integer"].Contains("int1"));
    HE_EXPECT(doc["integer"]["int1"].IsUint());
    HE_EXPECT_EQ(doc["integer"]["int1"].Uint(), 99);
    HE_EXPECT(doc["integer"].Contains("int2"));
    HE_EXPECT(doc["integer"]["int2"].IsUint());
    HE_EXPECT_EQ(doc["integer"]["int2"].Uint(), 42);
    HE_EXPECT(doc["integer"].Contains("int3"));
    HE_EXPECT(doc["integer"]["int3"].IsUint());
    HE_EXPECT_EQ(doc["integer"]["int3"].Uint(), 0);
    HE_EXPECT(doc["integer"].Contains("int4"));
    HE_EXPECT(doc["integer"]["int4"].IsInt());
    HE_EXPECT_EQ(doc["integer"]["int4"].Int(), -17);
    HE_EXPECT(doc["integer"].Contains("int5"));
    HE_EXPECT(doc["integer"]["int5"].IsUint());
    HE_EXPECT_EQ(doc["integer"]["int5"].Uint(), 1000);
    HE_EXPECT(doc["integer"].Contains("int6"));
    HE_EXPECT(doc["integer"]["int6"].IsUint());
    HE_EXPECT_EQ(doc["integer"]["int6"].Uint(), 5349221);
    HE_EXPECT(doc["integer"].Contains("int7"));
    HE_EXPECT(doc["integer"]["int7"].IsUint());
    HE_EXPECT_EQ(doc["integer"]["int7"].Uint(), 5349221);
    HE_EXPECT(doc["integer"].Contains("int8"));
    HE_EXPECT(doc["integer"]["int8"].IsUint());
    HE_EXPECT_EQ(doc["integer"]["int8"].Uint(), 12345);
    HE_EXPECT(doc["integer"].Contains("hex1"));
    HE_EXPECT(doc["integer"]["hex1"].IsUint());
    HE_EXPECT_EQ(doc["integer"]["hex1"].Uint(), 0xdeadbeef);
    HE_EXPECT(doc["integer"].Contains("hex2"));
    HE_EXPECT(doc["integer"]["hex2"].IsUint());
    HE_EXPECT_EQ(doc["integer"]["hex2"].Uint(), 0xdeadbeef);
    HE_EXPECT(doc["integer"].Contains("hex3"));
    HE_EXPECT(doc["integer"]["hex3"].IsUint());
    HE_EXPECT_EQ(doc["integer"]["hex3"].Uint(), 0xdeadbeef);
    HE_EXPECT(doc["integer"].Contains("oct1"));
    HE_EXPECT(doc["integer"]["oct1"].IsUint());
    HE_EXPECT_EQ(doc["integer"]["oct1"].Uint(), 01234567);
    HE_EXPECT(doc["integer"].Contains("oct2"));
    HE_EXPECT(doc["integer"]["oct2"].IsUint());
    HE_EXPECT_EQ(doc["integer"]["oct2"].Uint(), 0755);
    HE_EXPECT(doc["integer"].Contains("bin1"));
    HE_EXPECT(doc["integer"]["bin1"].IsUint());
    HE_EXPECT_EQ(doc["integer"]["bin1"].Uint(), 0b11010110);

    HE_EXPECT(doc.Contains("float"));
    HE_EXPECT(doc["float"].IsTable());
    HE_EXPECT(doc["float"].Contains("flt1"));
    HE_EXPECT(doc["float"]["flt1"].IsFloat());
    HE_EXPECT_EQ(doc["float"]["flt1"].Float(), 1.0);
    HE_EXPECT(doc["float"].Contains("flt2"));
    HE_EXPECT(doc["float"]["flt2"].IsFloat());
    HE_EXPECT_EQ(doc["float"]["flt2"].Float(), 3.1415);
    HE_EXPECT(doc["float"].Contains("flt3"));
    HE_EXPECT(doc["float"]["flt3"].IsFloat());
    HE_EXPECT_EQ(doc["float"]["flt3"].Float(), -0.01);
    HE_EXPECT(doc["float"].Contains("flt4"));
    HE_EXPECT(doc["float"]["flt4"].IsFloat());
    HE_EXPECT_EQ(doc["float"]["flt4"].Float(), 5e22);
    HE_EXPECT(doc["float"].Contains("flt5"));
    HE_EXPECT(doc["float"]["flt5"].IsFloat());
    HE_EXPECT_EQ(doc["float"]["flt5"].Float(), 1e06);
    HE_EXPECT(doc["float"].Contains("flt6"));
    HE_EXPECT(doc["float"]["flt6"].IsFloat());
    HE_EXPECT_EQ(doc["float"]["flt6"].Float(), -2E-2);
    HE_EXPECT(doc["float"].Contains("flt7"));
    HE_EXPECT(doc["float"]["flt7"].IsFloat());
    HE_EXPECT_EQ(doc["float"]["flt7"].Float(), 6.626e-34);
    HE_EXPECT(doc["float"].Contains("flt8"));
    HE_EXPECT(doc["float"]["flt8"].IsFloat());
    HE_EXPECT_EQ(doc["float"]["flt8"].Float(), 224617.445991228);
    HE_EXPECT(doc["float"].Contains("sf1"));
    HE_EXPECT(doc["float"]["sf1"].IsFloat());
    HE_EXPECT_EQ(doc["float"]["sf1"].Float(), Limits<double>::Infinity);
    HE_EXPECT(doc["float"].Contains("sf2"));
    HE_EXPECT(doc["float"]["sf2"].IsFloat());
    HE_EXPECT_EQ(doc["float"]["sf2"].Float(), Limits<double>::Infinity);
    HE_EXPECT(doc["float"].Contains("sf3"));
    HE_EXPECT(doc["float"]["sf3"].IsFloat());
    HE_EXPECT_EQ(doc["float"]["sf3"].Float(), -Limits<double>::Infinity);
    HE_EXPECT(doc["float"].Contains("sf4"));
    HE_EXPECT(doc["float"]["sf4"].IsFloat());
    HE_EXPECT_EQ(doc["float"]["sf4"].Float(), Limits<double>::NaN);
    HE_EXPECT(doc["float"].Contains("sf5"));
    HE_EXPECT(doc["float"]["sf5"].IsFloat());
    HE_EXPECT_EQ(doc["float"]["sf5"].Float(), Limits<double>::NaN);
    HE_EXPECT(doc["float"].Contains("sf6"));
    HE_EXPECT(doc["float"]["sf6"].IsFloat());
    HE_EXPECT_EQ(doc["float"]["sf6"].Float(), -Limits<double>::NaN);

    HE_EXPECT(doc.Contains("string"));
    HE_EXPECT(doc["string"].IsTable());
    HE_EXPECT(doc["string"].Contains("str"));
    HE_EXPECT(doc["string"]["str"].IsString());
    HE_EXPECT_EQ(doc["string"]["str"].String(), "I'm a string. \"You can quote me\". Name\tJos\xE9\nLocation\tSF.");
    HE_EXPECT(doc["string"].Contains("str1"));
    HE_EXPECT(doc["string"]["str1"].IsString());
    HE_EXPECT_EQ(doc["string"]["str1"].String(), "Roses are red\nViolets are blue");
    HE_EXPECT(doc["string"].Contains("str2"));
    HE_EXPECT(doc["string"]["str2"].IsString());
    HE_EXPECT_EQ(doc["string"]["str2"].String(), "Roses are red\nViolets are blue");
    HE_EXPECT(doc["string"].Contains("str3"));
    HE_EXPECT(doc["string"]["str3"].IsString());
    HE_EXPECT_EQ(doc["string"]["str3"].String(), "Roses are red\r\nViolets are blue");
    HE_EXPECT(doc["string"].Contains("str4"));
    HE_EXPECT(doc["string"]["str4"].IsString());
    HE_EXPECT_EQ(doc["string"]["str4"].String(), "The quick brown fox jumps over the lazy dog.");
    HE_EXPECT(doc["string"].Contains("str5"));
    HE_EXPECT(doc["string"]["str5"].IsString());
    HE_EXPECT_EQ(doc["string"]["str5"].String(), "The quick brown fox jumps over the lazy dog.");
    HE_EXPECT(doc["string"].Contains("str6"));
    HE_EXPECT(doc["string"]["str6"].IsString());
    HE_EXPECT_EQ(doc["string"]["str6"].String(), "The quick brown fox jumps over the lazy dog.");
    HE_EXPECT(doc["string"].Contains("str7"));
    HE_EXPECT(doc["string"]["str7"].IsString());
    HE_EXPECT_EQ(doc["string"]["str7"].String(), "Here are two quotation marks: \"\". Simple enough.");
    HE_EXPECT(doc["string"].Contains("str8"));
    HE_EXPECT(doc["string"]["str8"].IsString());
    HE_EXPECT_EQ(doc["string"]["str8"].String(), "Here are three quotation marks: \"\"\".");
    HE_EXPECT(doc["string"].Contains("str9"));
    HE_EXPECT(doc["string"]["str9"].IsString());
    HE_EXPECT_EQ(doc["string"]["str9"].String(), "Here are fifteen quotation marks: \"\"\"\"\"\"\"\"\"\"\"\"\"\"\".");
    HE_EXPECT(doc["string"].Contains("winpath"));
    HE_EXPECT(doc["string"]["winpath"].IsString());
    HE_EXPECT_EQ(doc["string"]["winpath"].String(), "C:\\Users\\nodejs\\templates");
    HE_EXPECT(doc["string"].Contains("winpath2"));
    HE_EXPECT(doc["string"]["winpath2"].IsString());
    HE_EXPECT_EQ(doc["string"]["winpath2"].String(), "\\\\ServerX\\admin$\\system32\\");
    HE_EXPECT(doc["string"].Contains("quoted"));
    HE_EXPECT(doc["string"]["quoted"].IsString());
    HE_EXPECT_EQ(doc["string"]["quoted"].String(), "Tom \"Dubs\" Preston-Werner");
    HE_EXPECT(doc["string"].Contains("regex"));
    HE_EXPECT(doc["string"]["regex"].IsString());
    HE_EXPECT_EQ(doc["string"]["regex"].String(), "<\\i\\c*\\s*>");
    HE_EXPECT(doc["string"].Contains("regex2"));
    HE_EXPECT(doc["string"]["regex2"].IsString());
    HE_EXPECT_EQ(doc["string"]["regex2"].String(), "I [dw]on't need \\d{2} apples");
    HE_EXPECT(doc["string"].Contains("lines"));
    HE_EXPECT(doc["string"]["lines"].IsString());
    HE_EXPECT_EQ(doc["string"]["lines"].String(), "The first newline is\ntrimmed in literal strings.\n   All other whitespace\n   is preserved.\n");
    HE_EXPECT(doc["string"].Contains("quot15"));
    HE_EXPECT(doc["string"]["quot15"].IsString());
    HE_EXPECT_EQ(doc["string"]["quot15"].String(), "Here are fifteen quotation marks: \"\"\"\"\"\"\"\"\"\"\"\"\"\"\"");
    HE_EXPECT(doc["string"].Contains("apos15"));
    HE_EXPECT(doc["string"]["apos15"].IsString());
    HE_EXPECT_EQ(doc["string"]["apos15"].String(), "Here are fifteen apostrophes: '''''''''''''''");
    HE_EXPECT(doc["string"].Contains("str11"));
    HE_EXPECT(doc["string"]["str11"].IsString());
    HE_EXPECT_EQ(doc["string"]["str11"].String(), "'That,' she said, 'is still pointless.'");
    HE_EXPECT(doc["string"].Contains("str12"));
    HE_EXPECT(doc["string"]["str12"].IsString());
    HE_EXPECT_EQ(doc["string"]["str12"].String(), "'That,' she said, 'is still pointless.''");

    HE_EXPECT(doc.Contains("datetime"));
    HE_EXPECT(doc["datetime"].IsTable());
    HE_EXPECT(doc["datetime"].Contains("odt1"));
    HE_EXPECT(doc["datetime"]["odt1"].IsDateTime());
    HE_EXPECT_EQ(doc["datetime"]["odt1"].DateTime(), ExpectedDateTime);
    HE_EXPECT(doc["datetime"].Contains("odt2"));
    HE_EXPECT(doc["datetime"]["odt2"].IsDateTime());
    HE_EXPECT_EQ(doc["datetime"]["odt2"].DateTime(), ExpectedDateTime);
    HE_EXPECT(doc["datetime"].Contains("odt3"));
    HE_EXPECT(doc["datetime"]["odt3"].IsDateTime());
    HE_EXPECT_EQ(doc["datetime"]["odt3"].DateTime(), (ExpectedDateTime + FromPeriod<Microseconds>(999999)));
    HE_EXPECT(doc["datetime"].Contains("odt4"));
    HE_EXPECT(doc["datetime"]["odt4"].IsDateTime());
    HE_EXPECT_EQ(doc["datetime"]["odt4"].DateTime(), ExpectedDateTime);
    HE_EXPECT(doc["datetime"].Contains("odt5"));
    HE_EXPECT(doc["datetime"]["odt5"].IsDateTime());
    HE_EXPECT_EQ(doc["datetime"]["odt5"].DateTime(), ExpectedDateTime);
    HE_EXPECT(doc["datetime"].Contains("odt6"));
    HE_EXPECT(doc["datetime"]["odt6"].IsDateTime());
    HE_EXPECT_EQ(doc["datetime"]["odt6"].DateTime(), ExpectedDateTime);
    HE_EXPECT(doc["datetime"].Contains("ldt1"));
    HE_EXPECT(doc["datetime"]["ldt1"].IsDateTime());
    HE_EXPECT_EQ(doc["datetime"]["ldt1"].DateTime(), ExpectedDateTime);
    HE_EXPECT(doc["datetime"].Contains("ldt2"));
    HE_EXPECT(doc["datetime"]["ldt2"].IsDateTime());
    HE_EXPECT_EQ(doc["datetime"]["ldt2"].DateTime(), (ExpectedDateTime + FromPeriod<Microseconds>(999999)));
    HE_EXPECT(doc["datetime"].Contains("ldt3"));
    HE_EXPECT(doc["datetime"]["ldt3"].IsDateTime());
    HE_EXPECT_EQ(doc["datetime"]["ldt3"].DateTime(), ExpectedDateTime);
    HE_EXPECT(doc["datetime"].Contains("ld1"));
    HE_EXPECT(doc["datetime"]["ld1"].IsDateTime());
    HE_EXPECT_EQ(doc["datetime"]["ld1"].DateTime(), (ExpectedDateTime - FromPeriod<Minutes>(32)));

    HE_EXPECT(doc.Contains("time"));
    HE_EXPECT(doc["time"].IsTable());
    HE_EXPECT(doc["time"].Contains("lt1"));
    HE_EXPECT(doc["time"]["lt1"].IsTime());
    HE_EXPECT_EQ(doc["time"]["lt1"].Time(), (FromPeriod<Hours>(7) + FromPeriod<Minutes>(32)));
    HE_EXPECT(doc["time"].Contains("lt2"));
    HE_EXPECT(doc["time"]["lt2"].IsTime());
    HE_EXPECT_EQ(doc["time"]["lt2"].Time(), (FromPeriod<Minutes>(32) + FromPeriod<Microseconds>(999999)));
    HE_EXPECT(doc["time"].Contains("lt3"));
    HE_EXPECT(doc["time"]["lt3"].IsTime());
    HE_EXPECT_EQ(doc["time"]["lt3"].Time(), (FromPeriod<Hours>(7) + FromPeriod<Minutes>(32)));

    HE_EXPECT(doc.Contains("array"));
    HE_EXPECT(doc["array"].IsTable());
    HE_EXPECT(doc["array"].Contains("integers"));
    HE_EXPECT(doc["array"]["integers"].IsArray());
    HE_EXPECT_EQ(doc["array"]["integers"].Array().Size(), 3);
    HE_EXPECT(doc["array"]["integers"][0].IsUint());
    HE_EXPECT_EQ(doc["array"]["integers"][0].Uint(), 1);
    HE_EXPECT(doc["array"]["integers"][1].IsUint());
    HE_EXPECT_EQ(doc["array"]["integers"][1].Uint(), 2);
    HE_EXPECT(doc["array"]["integers"][2].IsUint());
    HE_EXPECT_EQ(doc["array"]["integers"][2].Uint(), 3);
    HE_EXPECT(doc["array"].Contains("colors"));
    HE_EXPECT(doc["array"]["colors"].IsArray());
    HE_EXPECT_EQ(doc["array"]["colors"].Array().Size(), 3);
    HE_EXPECT(doc["array"]["colors"][0].IsString());
    HE_EXPECT_EQ(doc["array"]["colors"][0].String(), "red");
    HE_EXPECT(doc["array"]["colors"][1].IsString());
    HE_EXPECT_EQ(doc["array"]["colors"][1].String(), "yellow");
    HE_EXPECT(doc["array"]["colors"][2].IsString());
    HE_EXPECT_EQ(doc["array"]["colors"][2].String(), "green");
    HE_EXPECT(doc["array"].Contains("nested_arrays_of_ints"));
    HE_EXPECT(doc["array"]["nested_arrays_of_ints"].IsArray());
    HE_EXPECT_EQ(doc["array"]["nested_arrays_of_ints"].Array().Size(), 2);
    HE_EXPECT(doc["array"]["nested_arrays_of_ints"][0].IsArray());
    HE_EXPECT_EQ(doc["array"]["nested_arrays_of_ints"][0].Array().Size(), 2);
    HE_EXPECT(doc["array"]["nested_arrays_of_ints"][0][0].IsUint());
    HE_EXPECT_EQ(doc["array"]["nested_arrays_of_ints"][0][0].Uint(), 1);
    HE_EXPECT(doc["array"]["nested_arrays_of_ints"][0][1].IsUint());
    HE_EXPECT_EQ(doc["array"]["nested_arrays_of_ints"][0][1].Uint(), 2);
    HE_EXPECT(doc["array"]["nested_arrays_of_ints"][1].IsArray());
    HE_EXPECT_EQ(doc["array"]["nested_arrays_of_ints"][1].Array().Size(), 3);
    HE_EXPECT(doc["array"]["nested_arrays_of_ints"][1][0].IsUint());
    HE_EXPECT_EQ(doc["array"]["nested_arrays_of_ints"][1][0].Uint(), 3);
    HE_EXPECT(doc["array"]["nested_arrays_of_ints"][1][1].IsUint());
    HE_EXPECT_EQ(doc["array"]["nested_arrays_of_ints"][1][1].Uint(), 4);
    HE_EXPECT(doc["array"]["nested_arrays_of_ints"][1][2].IsUint());
    HE_EXPECT_EQ(doc["array"]["nested_arrays_of_ints"][1][2].Uint(), 5);
    HE_EXPECT(doc["array"].Contains("nested_mixed_array"));
    HE_EXPECT(doc["array"]["nested_mixed_array"].IsArray());
    HE_EXPECT_EQ(doc["array"]["nested_mixed_array"].Array().Size(), 2);
    HE_EXPECT(doc["array"]["nested_mixed_array"][0].IsArray());
    HE_EXPECT_EQ(doc["array"]["nested_mixed_array"][0].Array().Size(), 2);
    HE_EXPECT(doc["array"]["nested_mixed_array"][0][0].IsUint());
    HE_EXPECT_EQ(doc["array"]["nested_mixed_array"][0][0].Uint(), 1);
    HE_EXPECT(doc["array"]["nested_mixed_array"][0][1].IsUint());
    HE_EXPECT_EQ(doc["array"]["nested_mixed_array"][0][1].Uint(), 2);
    HE_EXPECT(doc["array"]["nested_mixed_array"][1].IsArray());
    HE_EXPECT_EQ(doc["array"]["nested_mixed_array"][1].Array().Size(), 3);
    HE_EXPECT(doc["array"]["nested_mixed_array"][1][0].IsString());
    HE_EXPECT_EQ(doc["array"]["nested_mixed_array"][1][0].String(), "a");
    HE_EXPECT(doc["array"]["nested_mixed_array"][1][1].IsString());
    HE_EXPECT_EQ(doc["array"]["nested_mixed_array"][1][1].String(), "b");
    HE_EXPECT(doc["array"]["nested_mixed_array"][1][2].IsString());
    HE_EXPECT_EQ(doc["array"]["nested_mixed_array"][1][2].String(), "c");
    HE_EXPECT(doc["array"].Contains("string_array"));
    HE_EXPECT(doc["array"]["string_array"].IsArray());
    HE_EXPECT_EQ(doc["array"]["string_array"].Array().Size(), 4);
    HE_EXPECT(doc["array"]["string_array"][0].IsString());
    HE_EXPECT_EQ(doc["array"]["string_array"][0].String(), "all");
    HE_EXPECT(doc["array"]["string_array"][1].IsString());
    HE_EXPECT_EQ(doc["array"]["string_array"][1].String(), "strings");
    HE_EXPECT(doc["array"]["string_array"][2].IsString());
    HE_EXPECT_EQ(doc["array"]["string_array"][2].String(), "are the same");
    HE_EXPECT(doc["array"]["string_array"][3].IsString());
    HE_EXPECT_EQ(doc["array"]["string_array"][3].String(), "type");
    HE_EXPECT(doc["array"].Contains("numbers"));
    HE_EXPECT(doc["array"]["numbers"].IsArray());
    HE_EXPECT_EQ(doc["array"]["numbers"].Array().Size(), 6);
    HE_EXPECT(doc["array"]["numbers"][0].IsFloat());
    HE_EXPECT_EQ(doc["array"]["numbers"][0].Float(), 0.1);
    HE_EXPECT(doc["array"]["numbers"][1].IsFloat());
    HE_EXPECT_EQ(doc["array"]["numbers"][1].Float(), 0.2);
    HE_EXPECT(doc["array"]["numbers"][2].IsFloat());
    HE_EXPECT_EQ(doc["array"]["numbers"][2].Float(), 0.5);
    HE_EXPECT(doc["array"]["numbers"][3].IsUint());
    HE_EXPECT_EQ(doc["array"]["numbers"][3].Uint(), 1);
    HE_EXPECT(doc["array"]["numbers"][4].IsUint());
    HE_EXPECT_EQ(doc["array"]["numbers"][4].Uint(), 2);
    HE_EXPECT(doc["array"]["numbers"][5].IsUint());
    HE_EXPECT_EQ(doc["array"]["numbers"][5].Uint(), 5);
    HE_EXPECT(doc["array"].Contains("contributors"));
    HE_EXPECT(doc["array"]["contributors"].IsArray());
    HE_EXPECT_EQ(doc["array"]["contributors"].Array().Size(), 2);
    HE_EXPECT(doc["array"]["contributors"][0].IsString());
    HE_EXPECT_EQ(doc["array"]["contributors"][0].String(), "Foo Bar <foo@example.com>");
    HE_EXPECT(doc["array"]["contributors"][1].IsTable());
    HE_EXPECT(doc["array"]["contributors"][1].Contains("name"));
    HE_EXPECT(doc["array"]["contributors"][1]["name"].IsString());
    HE_EXPECT_EQ(doc["array"]["contributors"][1]["name"].String(), "Baz Qux");
    HE_EXPECT(doc["array"]["contributors"][1].Contains("email"));
    HE_EXPECT(doc["array"]["contributors"][1]["email"].IsString());
    HE_EXPECT_EQ(doc["array"]["contributors"][1]["email"].String(), "bazqux@example.com");
    HE_EXPECT(doc["array"]["contributors"][1].Contains("url"));
    HE_EXPECT(doc["array"]["contributors"][1]["url"].IsString());
    HE_EXPECT_EQ(doc["array"]["contributors"][1]["url"].String(), "https://example.com/bazqux");
    HE_EXPECT(doc["array"].Contains("integers2"));
    HE_EXPECT(doc["array"]["integers2"].IsArray());
    HE_EXPECT_EQ(doc["array"]["integers2"].Array().Size(), 3);
    HE_EXPECT(doc["array"]["integers2"][0].IsUint());
    HE_EXPECT_EQ(doc["array"]["integers2"][0].Uint(), 1);
    HE_EXPECT(doc["array"]["integers2"][1].IsUint());
    HE_EXPECT_EQ(doc["array"]["integers2"][1].Uint(), 2);
    HE_EXPECT(doc["array"]["integers2"][2].IsUint());
    HE_EXPECT_EQ(doc["array"]["integers2"][2].Uint(), 3);
    HE_EXPECT(doc["array"].Contains("integers3"));
    HE_EXPECT(doc["array"]["integers3"].IsArray());
    HE_EXPECT_EQ(doc["array"]["integers3"].Array().Size(), 2);
    HE_EXPECT(doc["array"]["integers3"][0].IsUint());
    HE_EXPECT_EQ(doc["array"]["integers3"][0].Uint(), 1);
    HE_EXPECT(doc["array"]["integers3"][1].IsUint());
    HE_EXPECT_EQ(doc["array"]["integers3"][1].Uint(), 2);

    HE_EXPECT(doc.Contains("table"));
    HE_EXPECT(doc["table"].IsTable());
    HE_EXPECT(doc["table"].Table().IsEmpty());

    HE_EXPECT(doc.Contains("table-1"));
    HE_EXPECT(doc["table-1"].IsTable());
    HE_EXPECT(doc["table-1"].Contains("key1"));
    HE_EXPECT(doc["table-1"]["key1"].IsString());
    HE_EXPECT_EQ(doc["table-1"]["key1"].String(), "some string");
    HE_EXPECT(doc["table-1"].Contains("key2"));
    HE_EXPECT(doc["table-1"]["key2"].IsUint());
    HE_EXPECT_EQ(doc["table-1"]["key2"].Uint(), 123);

    HE_EXPECT(doc.Contains("table-2"));
    HE_EXPECT(doc["table-2"].IsTable());
    HE_EXPECT(doc["table-2"].Contains("key1"));
    HE_EXPECT(doc["table-2"]["key1"].IsString());
    HE_EXPECT_EQ(doc["table-2"]["key1"].String(), "another string");
    HE_EXPECT(doc["table-2"].Contains("key2"));
    HE_EXPECT(doc["table-2"]["key2"].IsUint());
    HE_EXPECT_EQ(doc["table-2"]["key2"].Uint(), 456);

    HE_EXPECT(doc.Contains("dog"));
    HE_EXPECT(doc["dog"].IsTable());
    HE_EXPECT(doc["dog"].Contains("tater.man"));
    HE_EXPECT(doc["dog"]["tater.man"].IsTable());
    HE_EXPECT(doc["dog"]["tater.man"].Contains("type"));
    HE_EXPECT(doc["dog"]["tater.man"]["type"].IsTable());
    HE_EXPECT(doc["dog"]["tater.man"]["type"].Contains("name"));
    HE_EXPECT(doc["dog"]["tater.man"]["type"]["name"].IsString());
    HE_EXPECT_EQ(doc["dog"]["tater.man"]["type"]["name"].String(), "pug");

    HE_EXPECT(doc.Contains("a"));
    HE_EXPECT(doc["a"].IsTable());
    HE_EXPECT(doc["a"].Contains("b"));
    HE_EXPECT(doc["a"]["b"].IsTable());
    HE_EXPECT(doc["a"]["b"].Contains("c"));
    HE_EXPECT(doc["a"]["b"]["c"].IsTable());
    HE_EXPECT(doc["a"]["b"]["c"].Table().IsEmpty());

    HE_EXPECT(doc.Contains("d"));
    HE_EXPECT(doc["d"].IsTable());
    HE_EXPECT(doc["d"].Contains("e"));
    HE_EXPECT(doc["d"]["e"].IsTable());
    HE_EXPECT(doc["d"]["e"].Contains("f"));
    HE_EXPECT(doc["d"]["e"]["f"].IsTable());
    HE_EXPECT(doc["d"]["e"]["f"].Table().IsEmpty());

    HE_EXPECT(doc.Contains("g"));
    HE_EXPECT(doc["g"].IsTable());
    HE_EXPECT(doc["g"].Contains("h"));
    HE_EXPECT(doc["g"]["h"].IsTable());
    HE_EXPECT(doc["g"]["h"].Contains("i"));
    HE_EXPECT(doc["g"]["h"]["i"].IsTable());
    HE_EXPECT(doc["g"]["h"]["i"].Table().IsEmpty());

    HE_EXPECT(doc.Contains("j"));
    HE_EXPECT(doc["j"].IsTable());
    HE_EXPECT(doc["j"].Contains("ʞ"));
    HE_EXPECT(doc["j"]["ʞ"].IsTable());
    HE_EXPECT(doc["j"]["ʞ"].Contains("l"));
    HE_EXPECT(doc["j"]["ʞ"]["l"].IsTable());
    HE_EXPECT(doc["j"]["ʞ"]["l"].Table().IsEmpty());

    HE_EXPECT(doc.Contains("x"));
    HE_EXPECT(doc["x"].IsTable());
    HE_EXPECT(doc["x"].Contains("y"));
    HE_EXPECT(doc["x"]["y"].IsTable());
    HE_EXPECT(doc["x"]["y"].Contains("z"));
    HE_EXPECT(doc["x"]["y"]["z"].IsTable());
    HE_EXPECT(doc["x"]["y"]["z"].Contains("w"));
    HE_EXPECT(doc["x"]["y"]["z"]["w"].IsTable());
    HE_EXPECT(doc["x"]["y"]["z"]["w"].Table().IsEmpty());

    HE_EXPECT(doc.Contains("fruit"));
    HE_EXPECT(doc["fruit"].IsTable());
    HE_EXPECT(doc["fruit"].Contains("apple"));
    HE_EXPECT(doc["fruit"]["apple"].IsTable());
    HE_EXPECT(doc["fruit"]["apple"].Contains("color"));
    HE_EXPECT(doc["fruit"]["apple"]["color"].IsString());
    HE_EXPECT_EQ(doc["fruit"]["apple"]["color"].String(), "red");
    HE_EXPECT(doc["fruit"]["apple"].Contains("taste"));
    HE_EXPECT(doc["fruit"]["apple"]["taste"].IsTable());
    HE_EXPECT(doc["fruit"]["apple"]["taste"].Contains("sweet"));
    HE_EXPECT(doc["fruit"]["apple"]["taste"]["sweet"].IsBool());
    HE_EXPECT_EQ(doc["fruit"]["apple"]["taste"]["sweet"].Bool(), true);
    HE_EXPECT(doc["fruit"]["apple"].Contains("texture"));
    HE_EXPECT(doc["fruit"]["apple"]["texture"].IsTable());
    HE_EXPECT(doc["fruit"]["apple"]["texture"].Contains("smooth"));
    HE_EXPECT(doc["fruit"]["apple"]["texture"]["smooth"].IsBool());
    HE_EXPECT_EQ(doc["fruit"]["apple"]["texture"]["smooth"].Bool(), true);

    HE_EXPECT(doc.Contains("inline"));
    HE_EXPECT(doc["inline"].IsTable());
    HE_EXPECT(doc["inline"].Contains("table"));
    HE_EXPECT(doc["inline"]["table"].IsTable());
    HE_EXPECT(doc["inline"]["table"].Contains("name"));
    HE_EXPECT(doc["inline"]["table"]["name"].IsTable());
    HE_EXPECT(doc["inline"]["table"]["name"].Contains("first"));
    HE_EXPECT(doc["inline"]["table"]["name"]["first"].IsString());
    HE_EXPECT_EQ(doc["inline"]["table"]["name"]["first"].String(), "Tom");
    HE_EXPECT(doc["inline"]["table"]["name"].Contains("last"));
    HE_EXPECT(doc["inline"]["table"]["name"]["last"].IsString());
    HE_EXPECT_EQ(doc["inline"]["table"]["name"]["last"].String(), "Preston-Werner");
    HE_EXPECT(doc["inline"]["table"].Contains("point"));
    HE_EXPECT(doc["inline"]["table"]["point"].IsTable());
    HE_EXPECT(doc["inline"]["table"]["point"].Contains("x"));
    HE_EXPECT(doc["inline"]["table"]["point"]["x"].IsUint());
    HE_EXPECT_EQ(doc["inline"]["table"]["point"]["x"].Uint(), 1);
    HE_EXPECT(doc["inline"]["table"]["point"].Contains("y"));
    HE_EXPECT(doc["inline"]["table"]["point"]["y"].IsUint());
    HE_EXPECT_EQ(doc["inline"]["table"]["point"]["y"].Uint(), 2);
    HE_EXPECT(doc["inline"]["table"].Contains("contact"));
    HE_EXPECT(doc["inline"]["table"]["contact"].IsTable());
    HE_EXPECT(doc["inline"]["table"]["contact"].Contains("personal"));
    HE_EXPECT(doc["inline"]["table"]["contact"]["personal"].IsTable());
    HE_EXPECT(doc["inline"]["table"]["contact"]["personal"].Contains("name"));
    HE_EXPECT_EQ(doc["inline"]["table"]["contact"]["personal"]["name"].String(), "Donald Duck");
    HE_EXPECT(doc["inline"]["table"]["contact"]["personal"]["name"].IsString());
    HE_EXPECT(doc["inline"]["table"]["contact"]["personal"].Contains("email"));
    HE_EXPECT(doc["inline"]["table"]["contact"]["personal"]["email"].IsString());
    HE_EXPECT_EQ(doc["inline"]["table"]["contact"]["personal"]["email"].String(), "donald@duckburg.com");
    HE_EXPECT(doc["inline"]["table"]["contact"].Contains("work"));
    HE_EXPECT(doc["inline"]["table"]["contact"]["work"].IsTable());
    HE_EXPECT(doc["inline"]["table"]["contact"]["work"].Contains("name"));
    HE_EXPECT_EQ(doc["inline"]["table"]["contact"]["work"]["name"].String(), "Coin cleaner");
    HE_EXPECT(doc["inline"]["table"]["contact"]["work"]["name"].IsString());
    HE_EXPECT(doc["inline"]["table"]["contact"]["work"].Contains("email"));
    HE_EXPECT(doc["inline"]["table"]["contact"]["work"]["email"].IsString());
    HE_EXPECT_EQ(doc["inline"]["table"]["contact"]["work"]["email"].String(), "donald@ScroogeCorp.com");

    HE_EXPECT(doc.Contains("product"));
    HE_EXPECT(doc["product"].IsArray());
    HE_EXPECT_EQ(doc["product"].Array().Size(), 3);
    HE_EXPECT(doc["product"][0].IsTable());
    HE_EXPECT(doc["product"][0].Contains("name"));
    HE_EXPECT(doc["product"][0]["name"].IsString());
    HE_EXPECT_EQ(doc["product"][0]["name"].String(), "Hammer");
    HE_EXPECT(doc["product"][0].Contains("sku"));
    HE_EXPECT(doc["product"][0]["sku"].IsUint());
    HE_EXPECT_EQ(doc["product"][0]["sku"].Uint(), 738594937);
    HE_EXPECT(doc["product"][1].IsTable());
    HE_EXPECT(doc["product"][1].Table().IsEmpty());
    HE_EXPECT(doc["product"][2].IsTable());
    HE_EXPECT(doc["product"][2].Contains("name"));
    HE_EXPECT(doc["product"][2]["name"].IsString());
    HE_EXPECT_EQ(doc["product"][2]["name"].String(), "Nail");
    HE_EXPECT(doc["product"][2].Contains("sku"));
    HE_EXPECT(doc["product"][2]["sku"].IsUint());
    HE_EXPECT_EQ(doc["product"][2]["sku"].Uint(), 284758393);
    HE_EXPECT(doc["product"][2].Contains("color"));
    HE_EXPECT(doc["product"][2]["color"].IsString());
    HE_EXPECT_EQ(doc["product"][2]["color"].String(), "gray");
    HE_EXPECT(doc["product"][2].Contains("points"));
    HE_EXPECT(doc["product"][2]["points"].IsArray());
    HE_EXPECT_EQ(doc["product"][2]["points"].Array().Size(), 3);
    HE_EXPECT(doc["product"][2]["points"][0].IsTable());
    HE_EXPECT(doc["product"][2]["points"][0].Contains("x"));
    HE_EXPECT(doc["product"][2]["points"][0]["x"].IsUint());
    HE_EXPECT_EQ(doc["product"][2]["points"][0]["x"].Uint(), 1);
    HE_EXPECT(doc["product"][2]["points"][0].Contains("y"));
    HE_EXPECT(doc["product"][2]["points"][0]["y"].IsUint());
    HE_EXPECT_EQ(doc["product"][2]["points"][0]["y"].Uint(), 2);
    HE_EXPECT(doc["product"][2]["points"][0].Contains("z"));
    HE_EXPECT(doc["product"][2]["points"][0]["z"].IsUint());
    HE_EXPECT_EQ(doc["product"][2]["points"][0]["z"].Uint(), 3);
    HE_EXPECT(doc["product"][2]["points"][1].IsTable());
    HE_EXPECT(doc["product"][2]["points"][1].Contains("x"));
    HE_EXPECT(doc["product"][2]["points"][1]["x"].IsUint());
    HE_EXPECT_EQ(doc["product"][2]["points"][1]["x"].Uint(), 7);
    HE_EXPECT(doc["product"][2]["points"][1].Contains("y"));
    HE_EXPECT(doc["product"][2]["points"][1]["y"].IsUint());
    HE_EXPECT_EQ(doc["product"][2]["points"][1]["y"].Uint(), 8);
    HE_EXPECT(doc["product"][2]["points"][1].Contains("z"));
    HE_EXPECT(doc["product"][2]["points"][1]["z"].IsUint());
    HE_EXPECT_EQ(doc["product"][2]["points"][1]["z"].Uint(), 9);
    HE_EXPECT(doc["product"][2]["points"][2].IsTable());
    HE_EXPECT(doc["product"][2]["points"][2].Contains("x"));
    HE_EXPECT(doc["product"][2]["points"][2]["x"].IsUint());
    HE_EXPECT_EQ(doc["product"][2]["points"][2]["x"].Uint(), 2);
    HE_EXPECT(doc["product"][2]["points"][2].Contains("y"));
    HE_EXPECT(doc["product"][2]["points"][2]["y"].IsUint());
    HE_EXPECT_EQ(doc["product"][2]["points"][2]["y"].Uint(), 4);
    HE_EXPECT(doc["product"][2]["points"][2].Contains("z"));
    HE_EXPECT(doc["product"][2]["points"][2]["z"].IsUint());
    HE_EXPECT_EQ(doc["product"][2]["points"][2]["z"].Uint(), 8);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, toml_document, Write)
{
    TomlDocument doc;

    doc["a"] = true;
    doc["b"] = 50;

    TomlValue::TableType& c = doc["c"].SetTable();
    c["d"] = "yes";

    TomlValue::ArrayType& e = doc["e"].SetArray();
    TomlValue::TableType& e0 = e.EmplaceBack().SetTable();
    e0["f"] = true;
    TomlValue::TableType& e1 = e.EmplaceBack().SetTable();
    e1["f"] = false;

    String toml = doc.ToString();
    HE_EXPECT_EQ(toml, "a = true\nb = 50\n[c]\nd = \"yes\"\n[[e]]\nf = true\n[[e]]\nf = false");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, toml_document, Write_Complex)
{
    // TODO
}
