// Copyright Chad Engler

#include "he/core/args.h"

#include "he/core/allocator.h"
#include "he/core/string_view_fmt.h"
#include "he/core/test.h"
#include "he/core/vector.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
struct TestArgs
{
    int64_t i64{ 0 };
    int32_t i32{ 0 };
    int16_t i16{ 0 };
    int8_t i8{ 0 };

    uint64_t u64{ 0 };
    uint32_t u32{ 0 };
    uint16_t u16{ 0 };
    uint8_t u8{ 0 };

    double d{ 0.0 };
    float f{ 0.0f };

    bool b0{ false };
    bool b1{ false };

    const char* str{};
    Vector<const char*> vec{ Allocator::GetDefault() };
};

template <int32_t N>
ArgResult ParseTestArgs(Span<ArgDesc> argDescs, const char* (&argv)[N])
{
    return ParseArgs(argDescs, N, argv);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, args, boolean)
{
    TestArgs args;

    ArgDesc argDescs[]
    {
        { args.b0, 'h', "help" },
        { args.b1, 'v', "verbose" },
    };

    {
        new(&args) TestArgs();

        const char* argv[]{ "game.exe", "-h", "-v" };
        HE_EXPECT(ParseTestArgs(argDescs, argv));

        HE_EXPECT(args.b0);
        HE_EXPECT(args.b1);
    }

    {
        new(&args) TestArgs();

        const char* argv[]{ "game.exe", "--help", "--verbose" };
        HE_EXPECT(ParseTestArgs(argDescs, argv));

        HE_EXPECT(args.b0);
        HE_EXPECT(args.b1);
    }

    {
        new(&args) TestArgs();

        const char* argv[]{ "game.exe", "-hv" };
        HE_EXPECT(ParseTestArgs(argDescs, argv));

        HE_EXPECT(args.b0);
        HE_EXPECT(args.b1);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, args, integers)
{
    TestArgs args;

    ArgDesc argDescs[]
    {
        { args.i64, "i64" },
        { args.i32, "i32" },
        { args.i16, "i16" },
        { args.i8, "i8" },

        { args.u64, "u64" },
        { args.u32, "u32" },
        { args.u16, "u16" },
        { args.u8, "u8" },
    };

    const char* argv[]
    {
        "game.exe",
        "--i64", "9223372036854775807",
        "--i32", "2147483647",
        "--i16", "32767",
        "--i8", "127",
        "--u64", "1844674407370955199",
        "--u32", "4294967295",
        "--u16", "65535",
        "--u8", "255",
    };
    HE_EXPECT(ParseTestArgs(argDescs, argv));

    HE_EXPECT_EQ(args.i64, 9223372036854775807ll);
    HE_EXPECT_EQ(args.i32, 2147483647);
    HE_EXPECT_EQ(args.i16, 32767);
    HE_EXPECT_EQ(args.i8, 127);
    HE_EXPECT_EQ(args.u64, 1844674407370955199ull);
    HE_EXPECT_EQ(args.u32, 4294967295);
    HE_EXPECT_EQ(args.u16, 65535);
    HE_EXPECT_EQ(args.u8, 255);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, args, integers_short)
{
    TestArgs args;

    ArgDesc argDescs[]
    {
        { args.i64, 'i' },
        { args.u64, 'u' },
    };

    const char* argv[]{ "game.exe", "-i64", "-u32" };
    HE_EXPECT(ParseTestArgs(argDescs, argv));

    HE_EXPECT_EQ(args.i64, 64);
    HE_EXPECT_EQ(args.u64, 32);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, args, integers_negative)
{
    TestArgs args;

    ArgDesc argDescs[]
    {
        { args.i64, "i64" },
        { args.i32, "i32" },
        { args.i16, "i16" },
        { args.i8, "i8" },
    };

    const char* argv[]
    {
        "game.exe",
        "--i64", "-9223372036854775807",
        "--i32", "-2147483647",
        "--i16", "-32767",
        "--i8", "-127",
    };
    HE_EXPECT(ParseTestArgs(argDescs, argv));

    HE_EXPECT_EQ(args.i64, -9223372036854775807ll);
    HE_EXPECT_EQ(args.i32, -2147483647);
    HE_EXPECT_EQ(args.i16, -32767);
    HE_EXPECT_EQ(args.i8, -127);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, args, integers_hex)
{
    TestArgs args;

    ArgDesc argDescs[]
    {
        { args.i64, "i64" },
        { args.i32, "i32" },
        { args.i16, "i16" },
        { args.i8, "i8" },

        { args.u64, "u64" },
        { args.u32, "u32" },
        { args.u16, "u16" },
        { args.u8, "u8" },
    };

    const char* argv[]
    {
        "game.exe",
        "--i64", "0x7fffffffffffffff",
        "--i32", "0x7fffffff",
        "--i16", "0x7fff",
        "--i8", "0x7f",
        "--u64", "0xffffffffffffffff",
        "--u32", "0xffffffff",
        "--u16", "0xffff",
        "--u8", "0xff",
    };
    HE_EXPECT(ParseTestArgs(argDescs, argv));

    HE_EXPECT_EQ(args.i64, 0x7fffffffffffffff);
    HE_EXPECT_EQ(args.i32, 0x7fffffff);
    HE_EXPECT_EQ(args.i16, 0x7fff);
    HE_EXPECT_EQ(args.i8, 0x7f);
    HE_EXPECT_EQ(args.u64, 0xffffffffffffffff);
    HE_EXPECT_EQ(args.u32, 0xffffffff);
    HE_EXPECT_EQ(args.u16, 0xffff);
    HE_EXPECT_EQ(args.u8, 0xff);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, args, integers_octal)
{
    TestArgs args;

    ArgDesc argDescs[]
    {
        { args.i64, "i64" },
        { args.i32, "i32" },
        { args.i16, "i16" },
        { args.i8, "i8" },

        { args.u64, "u64" },
        { args.u32, "u32" },
        { args.u16, "u16" },
        { args.u8, "u8" },
    };

    const char* argv[]
    {
        "game.exe",
        "--i64", "0112",
        "--i32", "0112",
        "--i16", "0112",
        "--i8", "0112",
        "--u64", "0112",
        "--u32", "0112",
        "--u16", "0112",
        "--u8", "0112",
    };
    HE_EXPECT(ParseTestArgs(argDescs, argv));

    HE_EXPECT_EQ(args.i64, 0112);
    HE_EXPECT_EQ(args.i32, 0112);
    HE_EXPECT_EQ(args.i16, 0112);
    HE_EXPECT_EQ(args.i8, 0112);
    HE_EXPECT_EQ(args.u64, 0112);
    HE_EXPECT_EQ(args.u32, 0112);
    HE_EXPECT_EQ(args.u16, 0112);
    HE_EXPECT_EQ(args.u8, 0112);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, args, integers_binary)
{
    TestArgs args;

    ArgDesc argDescs[]
    {
        { args.i64, "i64" },
        { args.i32, "i32" },
        { args.i16, "i16" },
        { args.i8, "i8" },

        { args.u64, "u64" },
        { args.u32, "u32" },
        { args.u16, "u16" },
        { args.u8, "u8" },
    };

    const char* argv[]
    {
        "game.exe",
        "--i64", "0b1001010",
        "--i32", "0b1001010",
        "--i16", "0b1001010",
        "--i8", "0b1001010",
        "--u64", "0b1001010",
        "--u32", "0b1001010",
        "--u16", "0b1001010",
        "--u8", "0b1001010",
    };
    HE_EXPECT(ParseTestArgs(argDescs, argv));

    HE_EXPECT_EQ(args.i64, 0b1001010);
    HE_EXPECT_EQ(args.i32, 0b1001010);
    HE_EXPECT_EQ(args.i16, 0b1001010);
    HE_EXPECT_EQ(args.i8, 0b1001010);
    HE_EXPECT_EQ(args.u64, 0b1001010);
    HE_EXPECT_EQ(args.u32, 0b1001010);
    HE_EXPECT_EQ(args.u16, 0b1001010);
    HE_EXPECT_EQ(args.u8, 0b1001010);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, args, floating_point)
{
    TestArgs args;

    ArgDesc argDescs[]
    {
        { args.d, 'd', "double" },
        { args.f, 'f', "float" },
    };

    {
        new(&args) TestArgs();

        const char* argv[]{ "game.exe", "-d", "1234567.54321", "-f", "1234.5678" };
        HE_EXPECT(ParseTestArgs(argDescs, argv));

        HE_EXPECT_EQ(args.d, 1234567.54321);
        HE_EXPECT_EQ(args.f, 1234.5678f);
    }

    {
        new(&args) TestArgs();

        const char* argv[]{ "game.exe", "--double", "1234567.54321" };
        HE_EXPECT(ParseTestArgs(argDescs, argv));

        HE_EXPECT_EQ(args.d, 1234567.54321);
        HE_EXPECT_EQ(args.f, 0.0f);
    }

    {
        new(&args) TestArgs();

        const char* argv[]{ "game.exe", "--float", "1234.5678" };
        HE_EXPECT(ParseTestArgs(argDescs, argv));

        HE_EXPECT_EQ(args.d, 0.0);
        HE_EXPECT_EQ(args.f, 1234.5678f);
    }

    {
        new(&args) TestArgs();

        const char* argv[]{ "game.exe", "--float", "1234.5678" };
        HE_EXPECT(ParseTestArgs(argDescs, argv));

        HE_EXPECT_EQ(args.d, 0.0);
        HE_EXPECT_EQ(args.f, 1234.5678f);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, args, string)
{
    TestArgs args;

    ArgDesc argDescs[]
    {
        { args.str, 's', "point" },
    };

    {
        new(&args) TestArgs();

        const char* str = "This is a string";
        const char* argv[]{ "game.exe", "-s", str };
        HE_EXPECT(ParseTestArgs(argDescs, argv));

        HE_EXPECT_EQ(args.str, str);
    }

    {
        new(&args) TestArgs();

        const char* str = "This is a string";
        const char* argv[]{ "game.exe", "--point", str };
        HE_EXPECT(ParseTestArgs(argDescs, argv));

        HE_EXPECT_EQ(args.str, str);
    }

    {
        new(&args) TestArgs();

        const char* argv[]{ "game.exe", "-sTEST_2" };
        HE_EXPECT(ParseTestArgs(argDescs, argv));

        HE_EXPECT_EQ(args.str, argv[1] + 2);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, args, vector)
{
    TestArgs args;

    ArgDesc argDescs[]
    {
        { args.vec, 'I', "include-dir" },
    };

    {
        new(&args) TestArgs();

        const char* includeDirs[]{ "c:/", "d:/", "e:/", "f:/" };
        const char* argv[]{ "game.exe", "-I", includeDirs[0], "--include-dir", includeDirs[1], "-I", includeDirs[2], "--include-dir", includeDirs[3] };
        HE_EXPECT(ParseTestArgs(argDescs, argv));

        HE_EXPECT_EQ(args.vec.Size(), HE_LENGTH_OF(includeDirs));
        for (uint32_t i = 0; i < HE_LENGTH_OF(includeDirs); ++i)
        {
            HE_EXPECT_EQ(args.vec[i], includeDirs[i]);
        }
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, args, mixed)
{
    TestArgs args;

    ArgDesc argDescs[]
    {
        { args.i32, 'i', "i32" },
        { args.u32, 'u', "u32" },
        { args.b0, 'h', "help" },
        { args.b1, 'v', "verbose" },
        { args.str, 's', "point" },
        { args.vec, 'c', "cell" },
    };

    const char* str0 = "This is a string.";
    const char* argv[]
    {
        "game.exe",
        "-i10",
        "--cell", "123",
        "--u32", "5072",
        "--cell", "456",
        "-hv",
        "-c", "789",
        "--point", str0,
    };
    HE_EXPECT(ParseTestArgs(argDescs, argv));

    HE_EXPECT_EQ(args.i32, 10);
    HE_EXPECT_EQ(args.u32, 5072);
    HE_EXPECT(args.b0);
    HE_EXPECT(args.b1);
    HE_EXPECT_EQ(args.str, str0);
    HE_EXPECT_EQ(args.vec.Size(), 3);
    HE_EXPECT_EQ_STR(args.vec[0], "123");
    HE_EXPECT_EQ_STR(args.vec[1], "456");
    HE_EXPECT_EQ_STR(args.vec[2], "789");
}
