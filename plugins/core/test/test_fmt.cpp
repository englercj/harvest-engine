// Copyright Chad Engler

#include "he/core/fmt.h"

#include "he/core/string.h"
#include "he/core/string_fmt.h"
#include "he/core/string_ops.h"
#include "he/core/test.h"
#include "he/core/vector.h"

#include <cstdio>
#include <cmath>

using namespace he;

// ------------------------------------------------------------------------------------------------
static const char* s_testFmtErrorMsg = nullptr;

static void TestFmtStringErrorHandler(const char* msg)
{
    if (!s_testFmtErrorMsg)
    {
        s_testFmtErrorMsg = msg;
    }
}

template <typename... Args>
void TestFmtStringForError(StringView fmt, Args&&...)
{
    FmtCheckVisitor<RemoveCVRef<Args>...> checker(fmt, TestFmtStringErrorHandler);
    FmtVisitString(fmt, checker);
}

#define HE_EXPECT_FMT_ERR(fmt, msg, ...) \
    s_testFmtErrorMsg = nullptr; \
    TestFmtStringForError(fmt, __VA_ARGS__); \
    HE_EXPECT(s_testFmtErrorMsg != nullptr); \
    HE_EXPECT(StrEqual(s_testFmtErrorMsg, msg), fmt, s_testFmtErrorMsg, msg)

// ------------------------------------------------------------------------------------------------
template <typename T>
static void TestUnknownSpecTypes(const T& value, const char* types)
{
    char buf[256];
    const char* special = ".0123456789?}";
    for (int i = CHAR_MIN; i <= CHAR_MAX; ++i)
    {
        const char ch = static_cast<char>(i);
        if (StrFind(types, ch) || StrFind(special, ch) || !ch)
            continue;

        std::snprintf(buf, sizeof(buf), "{0:10%c}", ch);

        s_testFmtErrorMsg = nullptr;
        TestFmtStringForError(buf, value);
        HE_EXPECT(s_testFmtErrorMsg != nullptr);
        HE_EXPECT(StrEqual(s_testFmtErrorMsg, "Invalid type specifier") || StrEqual(s_testFmtErrorMsg, "Unknown format specifier"), s_testFmtErrorMsg, ch, buf);
    }
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, FmtParseCtx)
{
    constexpr StringView expected = "testing";
    constexpr FmtParseCtx constCtx(expected, 0);

    static_assert(constCtx.Begin() == expected.Begin());
    static_assert(constCtx.End() == expected.End());
    static_assert(constCtx.Size() == expected.Size());

    {
        FmtParseCtx ctx(expected, 2);
        HE_EXPECT_EQ(ctx.Begin(), expected.Begin());
        HE_EXPECT_EQ(ctx.End(), expected.End());
        HE_EXPECT_EQ(ctx.Size(), expected.Size());

        ctx.Advance(2);
        HE_EXPECT_EQ(ctx.Begin(), expected.Begin() + 2);
        HE_EXPECT_EQ(ctx.End(), expected.End());
        HE_EXPECT_EQ(ctx.Size(), expected.Size() - 2);

        ctx.AdvanceTo(expected.Begin() + 4);
        HE_EXPECT_EQ(ctx.Begin(), expected.Begin() + 4);
        HE_EXPECT_EQ(ctx.End(), expected.End());
        HE_EXPECT_EQ(ctx.Size(), expected.Size() - 4);
    }

    {
        static bool s_errorReported = false;
        const auto errorHandler = [](const char*) { s_errorReported = true; };

        FmtParseCtx ctx(expected, 2, 0, errorHandler);

        HE_EXPECT(!s_errorReported);
        ctx.CheckArgId(0);
        HE_EXPECT(!s_errorReported);
        ctx.CheckArgId(1);
        HE_EXPECT(!s_errorReported);
        ctx.CheckArgId(2);
        HE_EXPECT(s_errorReported);
        s_errorReported = false;
        ctx.NextArgId();
        HE_EXPECT(s_errorReported);
    }

    {
        static bool s_errorReported = false;
        const auto errorHandler = [](const char*) { s_errorReported = true; };

        FmtParseCtx ctx(expected, 2, 0, errorHandler);

        HE_EXPECT(!s_errorReported);
        ctx.NextArgId();
        HE_EXPECT(!s_errorReported);
        ctx.NextArgId();
        HE_EXPECT(!s_errorReported);
        ctx.NextArgId();
        HE_EXPECT(s_errorReported);
        s_errorReported = false;
        ctx.CheckArgId(0);
        HE_EXPECT(s_errorReported);
    }
}

// ------------------------------------------------------------------------------------------------
constexpr bool FmtParseUint_TestConst(StringView text, uint32_t expected)
{
    const char* begin = text.Begin();
    return FmtParseUint(begin, text.End()) == expected && begin == text.End();
}

HE_TEST(core, fmt, FmtParseUint)
{
    static_assert(FmtParseUint_TestConst("10000000000", Limits<uint32_t>::Max));
    static_assert(FmtParseUint_TestConst("2147483649", Limits<uint32_t>::Max));
    static_assert(FmtParseUint_TestConst("10", 10u));
    static_assert(FmtParseUint_TestConst("600000", 600000u));
    static_assert(FmtParseUint_TestConst("987654321", 987654321u));
}

// ------------------------------------------------------------------------------------------------
template <typename T>
constexpr bool FmtVisitString_ArgId_TextConst([[maybe_unused]] StringView text, [[maybe_unused]] T& visitor, [[maybe_unused]] uint32_t expected)
{
    // TODO
    return true;
}

HE_TEST(core, fmt, FmtVisitString_ArgId)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
template <typename T>
constexpr bool FmtVisitString_Field_TestConst([[maybe_unused]] StringView text, [[maybe_unused]] T& visitor, [[maybe_unused]] uint32_t expected)
{
    // TODO
    return true;
}

HE_TEST(core, fmt, FmtVisitString_Field)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
template <typename T>
constexpr bool FmtVisitString_Text_TestConst([[maybe_unused]] StringView text, [[maybe_unused]] T& visitor, [[maybe_unused]] uint32_t expected)
{
    // TODO
    return true;
}

HE_TEST(core, fmt, FmtVisitString_Text)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
template <typename T>
constexpr bool FmtVisitString_Short_TestConst([[maybe_unused]] StringView text, [[maybe_unused]] T& visitor, [[maybe_unused]] uint32_t expected)
{
    // TODO
    return true;
}

HE_TEST(core, fmt, FmtVisitString_Short)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
template <typename T>
constexpr bool FmtVisitString_Long_TestConst([[maybe_unused]] StringView text, [[maybe_unused]] T& visitor, [[maybe_unused]] uint32_t expected)
{
    // TODO
    return true;
}

HE_TEST(core, fmt, FmtVisitString_Long)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
template <typename T>
constexpr bool FmtVisitString_TestConst([[maybe_unused]] StringView text, [[maybe_unused]] T& visitor, [[maybe_unused]] uint32_t expected)
{
    // TODO
    return true;
}

HE_TEST(core, fmt, FmtVisitString)
{
    // TODO
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, auto_indexing)
{
    HE_EXPECT_EQ(Format("{}{}{}", 'a', 'b', 'c'), "abc");
    HE_EXPECT_EQ(Format("{:.2}", 1.2345), "1.2");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, manual_indexing)
{
    HE_EXPECT_EQ(Format("{2}{1}{0}", 'a', 'b', 'c'), "cba");
    HE_EXPECT_EQ(Format("{0:.2}", 1.2345), "1.2");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, escaped_braces)
{
    HE_EXPECT_EQ(Format("{{"), "{");
    HE_EXPECT_EQ(Format("before {{"), "before {");
    HE_EXPECT_EQ(Format("{{ after"), "{ after");
    HE_EXPECT_EQ(Format("before {{ after"), "before { after");
    HE_EXPECT_EQ(Format("}}"), "}");
    HE_EXPECT_EQ(Format("before }}"), "before }");
    HE_EXPECT_EQ(Format("}} after"), "} after");
    HE_EXPECT_EQ(Format("before }} after"), "before } after");
    HE_EXPECT_EQ(Format("{{}}"), "{}");
    HE_EXPECT_EQ(Format("{{{}}}", 5), "{5}");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, different_positions)
{
    HE_EXPECT_EQ(Format("{0}", 42), "42");
    HE_EXPECT_EQ(Format("before {0}", 42), "before 42");
    HE_EXPECT_EQ(Format("{0} after", 42), "42 after");
    HE_EXPECT_EQ(Format("before {0} after", 42), "before 42 after");
    HE_EXPECT_EQ(Format("{0} = {1}", "answer", 42), "answer = 42");
    HE_EXPECT_EQ(Format("{1} is the {0}", "answer", 42), "42 is the answer");
    HE_EXPECT_EQ(Format("{0}{1}{0}", "abra", "cad"), "abracadabra");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, empty_spec)
{
    HE_EXPECT_EQ(Format("{0:}", 42), "42");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, left_align)
{
    HE_EXPECT_EQ(Format("{0:<4}", 42), "42  ");
    HE_EXPECT_EQ(Format("{0:<4o}", 042), "42  ");
    HE_EXPECT_EQ(Format("{0:<4x}", 0x42), "42  ");
    HE_EXPECT_EQ(Format("{0:<5}", -42), "-42  ");
    HE_EXPECT_EQ(Format("{0:<5}", 42u), "42   ");
    HE_EXPECT_EQ(Format("{0:<5}", -42l), "-42  ");
    HE_EXPECT_EQ(Format("{0:<5}", 42ul), "42   ");
    HE_EXPECT_EQ(Format("{0:<5}", -42ll), "-42  ");
    HE_EXPECT_EQ(Format("{0:<5}", 42ull), "42   ");
    HE_EXPECT_EQ(Format("{0:<5}", -42.0), "-42  ");
    HE_EXPECT_EQ(Format("{0:<5}", -42.0l), "-42  ");
    HE_EXPECT_EQ(Format("{0:<5}", 'c'), "c    ");
    HE_EXPECT_EQ(Format("{0:<5}", "abc"), "abc  ");
    HE_EXPECT_EQ(Format("{0:<8}", reinterpret_cast<void*>(0xface)), "0xface  ");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, right_align)
{
    HE_EXPECT_EQ(Format("{0:>4}", 42), "  42");
    HE_EXPECT_EQ(Format("{0:>4o}", 042), "  42");
    HE_EXPECT_EQ(Format("{0:>4x}", 0x42), "  42");
    HE_EXPECT_EQ(Format("{0:>5}", -42), "  -42");
    HE_EXPECT_EQ(Format("{0:>5}", 42u), "   42");
    HE_EXPECT_EQ(Format("{0:>5}", -42l), "  -42");
    HE_EXPECT_EQ(Format("{0:>5}", 42ul), "   42");
    HE_EXPECT_EQ(Format("{0:>5}", -42ll), "  -42");
    HE_EXPECT_EQ(Format("{0:>5}", 42ull), "   42");
    HE_EXPECT_EQ(Format("{0:>5}", -42.0), "  -42");
    HE_EXPECT_EQ(Format("{0:>5}", -42.0l), "  -42");
    HE_EXPECT_EQ(Format("{0:>5}", 'c'), "    c");
    HE_EXPECT_EQ(Format("{0:>5}", "abc"), "  abc");
    HE_EXPECT_EQ(Format("{0:>8}", reinterpret_cast<void*>(0xface)), "  0xface");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, center_align)
{
    HE_EXPECT_EQ(Format("{0:^5}", 42), " 42  ");
    HE_EXPECT_EQ(Format("{0:^5o}", 042), " 42  ");
    HE_EXPECT_EQ(Format("{0:^5x}", 0x42), " 42  ");
    HE_EXPECT_EQ(Format("{0:^5}", -42), " -42 ");
    HE_EXPECT_EQ(Format("{0:^5}", 42u), " 42  ");
    HE_EXPECT_EQ(Format("{0:^5}", -42l), " -42 ");
    HE_EXPECT_EQ(Format("{0:^5}", 42ul), " 42  ");
    HE_EXPECT_EQ(Format("{0:^5}", -42ll), " -42 ");
    HE_EXPECT_EQ(Format("{0:^5}", 42ull), " 42  ");
    HE_EXPECT_EQ(Format("{0:^5}", -42.0), " -42 ");
    HE_EXPECT_EQ(Format("{0:^5}", -42.0l), " -42 ");
    HE_EXPECT_EQ(Format("{0:^5}", 'c'), "  c  ");
    HE_EXPECT_EQ(Format("{0:^6}", "abc"), " abc  ");
    HE_EXPECT_EQ(Format("{0:^8}", reinterpret_cast<void*>(0xface)), " 0xface ");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, fill)
{
    static_assert('\x1f' == 0x1f);
    HE_EXPECT_EQ(Format("{0:*>4}", 42), "**42");
    HE_EXPECT_EQ(Format("{0:*>5}", -42), "**-42");
    HE_EXPECT_EQ(Format("{0:*>5}", 42u), "***42");
    HE_EXPECT_EQ(Format("{0:*>5}", -42l), "**-42");
    HE_EXPECT_EQ(Format("{0:*>5}", 42ul), "***42");
    HE_EXPECT_EQ(Format("{0:*>5}", -42ll), "**-42");
    HE_EXPECT_EQ(Format("{0:*>5}", 42ull), "***42");
    HE_EXPECT_EQ(Format("{0:*>5}", -42.0), "**-42");
    HE_EXPECT_EQ(Format("{0:*>5}", -42.0l), "**-42");
    HE_EXPECT_EQ(Format("{0:*<5}", 'c'), "c****");
    HE_EXPECT_EQ(Format("{0:*<5}", "abc"), "abc**");
    HE_EXPECT_EQ(Format("{0:*>8}", reinterpret_cast<void*>(0xface)), "**0xface");
    HE_EXPECT_EQ(Format("{:}=", "foo"), "foo=");
    HE_EXPECT_EQ(Format("{0:ж>4}", 42), "жж42");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, plus_sign)
{
    HE_EXPECT_EQ(Format("{0:+}", 42), "+42");
    HE_EXPECT_EQ(Format("{0:+}", -42), "-42");
    HE_EXPECT_EQ(Format("{0:+}", 42), "+42");
    HE_EXPECT_EQ(Format("{0:+}", 42l), "+42");
    HE_EXPECT_EQ(Format("{0:+}", 42ll), "+42");
    HE_EXPECT_EQ(Format("{0:+}", 42.0), "+42");
    HE_EXPECT_EQ(Format("{0:+}", 42.0l), "+42");

    HE_EXPECT_FMT_ERR("{0:+", "Invalid format specifier for char", 'a');
    HE_EXPECT_FMT_ERR("{0:+}", "Invalid format specifier for char", 'a');
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, minus_sign)
{
    HE_EXPECT_EQ(Format("{0:-}", 42), "42");
    HE_EXPECT_EQ(Format("{0:-}", -42), "-42");
    HE_EXPECT_EQ(Format("{0:-}", 42), "42");
    HE_EXPECT_EQ(Format("{0:-}", 42l), "42");
    HE_EXPECT_EQ(Format("{0:-}", 42ll), "42");
    HE_EXPECT_EQ(Format("{0:-}", 42.0), "42");
    HE_EXPECT_EQ(Format("{0:-}", 42.0l), "42");

    HE_EXPECT_FMT_ERR("{0:-", "Invalid format specifier for char", 'a');
    HE_EXPECT_FMT_ERR("{0:-}", "Invalid format specifier for char", 'a');
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, space_sign)
{
    HE_EXPECT_EQ(Format("{0: }", 42), " 42");
    HE_EXPECT_EQ(Format("{0: }", -42), "-42");
    HE_EXPECT_EQ(Format("{0: }", 42), " 42");
    HE_EXPECT_EQ(Format("{0: }", 42l), " 42");
    HE_EXPECT_EQ(Format("{0: }", 42ll), " 42");
    HE_EXPECT_EQ(Format("{0: }", 42.0), " 42");
    HE_EXPECT_EQ(Format("{0: }", 42.0l), " 42");

    HE_EXPECT_FMT_ERR("{0: ", "Invalid format specifier for char", 'a');
    HE_EXPECT_FMT_ERR("{0: }", "Invalid format specifier for char", 'a');
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, alt_flag)
{
    HE_EXPECT_EQ(Format("{0:#}", 42), "42");
    HE_EXPECT_EQ(Format("{0:#}", -42), "-42");
    HE_EXPECT_EQ(Format("{0:#b}", 42), "0b101010");
    HE_EXPECT_EQ(Format("{0:#B}", 42), "0B101010");
    HE_EXPECT_EQ(Format("{0:#b}", -42), "-0b101010");
    HE_EXPECT_EQ(Format("{0:#x}", 0x42), "0x42");
    HE_EXPECT_EQ(Format("{0:#X}", 0x42), "0X42");
    HE_EXPECT_EQ(Format("{0:#x}", -0x42), "-0x42");
    HE_EXPECT_EQ(Format("{0:#o}", 0), "0");
    HE_EXPECT_EQ(Format("{0:#o}", 042), "042");
    HE_EXPECT_EQ(Format("{0:#o}", -042), "-042");
    HE_EXPECT_EQ(Format("{0:#}", 42u), "42");
    HE_EXPECT_EQ(Format("{0:#x}", 0x42u), "0x42");
    HE_EXPECT_EQ(Format("{0:#o}", 042u), "042");

    HE_EXPECT_EQ(Format("{0:#}", -42l), "-42");
    HE_EXPECT_EQ(Format("{0:#x}", 0x42l), "0x42");
    HE_EXPECT_EQ(Format("{0:#x}", -0x42l), "-0x42");
    HE_EXPECT_EQ(Format("{0:#o}", 042l), "042");
    HE_EXPECT_EQ(Format("{0:#o}", -042l), "-042");
    HE_EXPECT_EQ(Format("{0:#}", 42ul), "42");
    HE_EXPECT_EQ(Format("{0:#x}", 0x42ul), "0x42");
    HE_EXPECT_EQ(Format("{0:#o}", 042ul), "042");

    HE_EXPECT_EQ(Format("{0:#}", -42ll), "-42");
    HE_EXPECT_EQ(Format("{0:#x}", 0x42ll), "0x42");
    HE_EXPECT_EQ(Format("{0:#x}", -0x42ll), "-0x42");
    HE_EXPECT_EQ(Format("{0:#o}", 042ll), "042");
    HE_EXPECT_EQ(Format("{0:#o}", -042ll), "-042");
    HE_EXPECT_EQ(Format("{0:#}", 42ull), "42");
    HE_EXPECT_EQ(Format("{0:#x}", 0x42ull), "0x42");
    HE_EXPECT_EQ(Format("{0:#o}", 042ull), "042");

    HE_EXPECT_EQ(Format("{0:#}", -42.0), "-42.0");
    HE_EXPECT_EQ(Format("{0:#}", -42.0l), "-42.0");
    HE_EXPECT_EQ(Format("{:#.0e}", 42.0), "4.e+01");
    HE_EXPECT_EQ(Format("{:#.0f}", 0.01), "0.");
    HE_EXPECT_EQ(Format("{:#.2g}", 0.5), "0.50");
    HE_EXPECT_EQ(Format("{:#.0f}", 0.5), "0.");

    HE_EXPECT_FMT_ERR("{0:#", "Invalid format specifier for char", 'a');
    HE_EXPECT_FMT_ERR("{0:#}", "Invalid format specifier for char", 'a');
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, zero_flag)
{
    HE_EXPECT_EQ(Format("{0:0}", 42), "42");
    HE_EXPECT_EQ(Format("{0:05}", -42), "-0042");
    HE_EXPECT_EQ(Format("{0:05}", 42u), "00042");
    HE_EXPECT_EQ(Format("{0:05}", -42l), "-0042");
    HE_EXPECT_EQ(Format("{0:05}", 42ul), "00042");
    HE_EXPECT_EQ(Format("{0:05}", -42ll), "-0042");
    HE_EXPECT_EQ(Format("{0:05}", 42ull), "00042");
    HE_EXPECT_EQ(Format("{0:07}", -42.0), "-000042");
    HE_EXPECT_EQ(Format("{0:07}", -42.0l), "-000042");

    HE_EXPECT_FMT_ERR("{0:0", "Invalid format specifier for char", 'a');
    HE_EXPECT_FMT_ERR("{0:05}", "Invalid format specifier for char", 'a');
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, width)
{
    HE_EXPECT_EQ(Format("{0:4}", -42), " -42");
    HE_EXPECT_EQ(Format("{0:5}", 42u), "   42");
    HE_EXPECT_EQ(Format("{0:6}", -42l), "   -42");
    HE_EXPECT_EQ(Format("{0:7}", 42ul), "     42");
    HE_EXPECT_EQ(Format("{0:6}", -42ll), "   -42");
    HE_EXPECT_EQ(Format("{0:7}", 42ull), "     42");
    HE_EXPECT_EQ(Format("{0:8}", -1.23), "   -1.23");
    HE_EXPECT_EQ(Format("{0:9}", -1.23l), "    -1.23");
    HE_EXPECT_EQ(Format("{0:10}", reinterpret_cast<void*>(0xcafe)), "    0xcafe");
    HE_EXPECT_EQ(Format("{0:11}", 'x'), "x          ");
    HE_EXPECT_EQ(Format("{0:12}", "str"), "str         ");
    HE_EXPECT_EQ(Format("{:*^6}", "🤡"), "**🤡**");
    HE_EXPECT_EQ(Format("{:*^8}", "你好"), "**你好**");
    HE_EXPECT_EQ(Format("{:#6}", 42.0), "  42.0");
    HE_EXPECT_EQ(Format("{:6c}", static_cast<int>('x')), "x     ");
    HE_EXPECT_EQ(Format("{:>06.0f}", 0.00884311), "000000");

    HE_EXPECT_FMT_ERR("{1:0}", "Argument not found", 0);
    HE_EXPECT_FMT_ERR("{0:{2147483648", "Argument not found");
    HE_EXPECT_FMT_ERR("{0:2147483648}", "Width number is too big", 0);
    HE_EXPECT_FMT_ERR("{0:{", "Invalid type specifier", 0);
    HE_EXPECT_FMT_ERR("{0:{}", "Invalid type specifier", 0);
    HE_EXPECT_FMT_ERR("{0:{?}}", "Invalid type specifier", 0);
    HE_EXPECT_FMT_ERR("{0:{0:}}", "Invalid type specifier", 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, precision)
{
    HE_EXPECT_EQ(Format("{0:.2}", 1.2345), "1.2");
    HE_EXPECT_EQ(Format("{0:.2}", 1.2345l), "1.2");
    HE_EXPECT_EQ(Format("{:.2}", 1.234e56), "1.2e+56");
    HE_EXPECT_EQ(Format("{0:.3}", 1.1), "1.1");
    HE_EXPECT_EQ(Format("{:.0e}", 1.0), "1.e+00");
    HE_EXPECT_EQ(Format("{:9.1e}", 0.0), "  0.0e+00");
    HE_EXPECT_EQ(Format("{:.7f}", 0.0000000000000071054273576010018587L), "0.0000000");

    HE_EXPECT_EQ(Format("{:#.0f}", 123.0), "123.");
    HE_EXPECT_EQ(Format("{:.02f}", 1.234), "1.23");
    HE_EXPECT_EQ(Format("{:.1g}", 0.001), "0.001");
    HE_EXPECT_EQ(Format("{}", 1019666432.0f), "1019666400");
    HE_EXPECT_EQ(Format("{:.0e}", 9.5), "1.e+01");
    HE_EXPECT_EQ(Format("{:.1e}", 1e-34), "1.0e-34");

    HE_EXPECT_EQ(Format("{:.494}", 4.9406564584124654e-324),
        "4.9406564584124654417656879286822137236505980261432476442558568250067550"
        "727020875186529983636163599237979656469544571773092665671035593979639877"
        "479601078187812630071319031140452784581716784898210368871863605699873072"
        "305000638740915356498438731247339727316961514003171538539807412623856559"
        "117102665855668676818703956031062493194527159149245532930545654440112748"
        "012970999954193198940908041656332452475714786901472678015935523861155013"
        "480352649347201937902681071074917033322268447533357208324319361e-324");
    HE_EXPECT_EQ(Format("{:.1074f}", 1.1125369292536e-308),
        "0.0000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000111253692925360019747947051741965785554081512200979"
        "355021686109411883779182127659725163430929750364498219730822952552570601"
        "152163505899912777129583674906301179059298598412303893909188340988729019"
        "014361467448914817838555156840459458527907308695109202499990850735085304"
        "478476991912072201449236975063640913461919914396877093174125167509869762"
        "482369631100360266123742648159508919592746619553246586039571522788247697"
        "156360766271842991667238355464496455107749716934387136380536472531224398"
        "559833794807213172371254492216255558078524900147957309382830827524104234"
        "530961756787819847850302379672357738807808384667004752163416921762619527"
        "462847642037420991432005657440259928195996762610375541867198059294212446"
        "81962777939941034720757232455434770912461317493580281734466552734375");
    HE_EXPECT_EQ(Format("{:.838A}", -2.14001164e+38),
        "-0X1.41FE3FFE71C9E000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000P+127");

    if constexpr (Limits<long double>::Digits == 64)
    {
        HE_EXPECT_EQ(Format("{:.0}", Limits<long double>::MinPos), "3e-4932");
        HE_EXPECT_EQ(Format("{:0g}", Limits<long double>::DenormMin), "3.6452e-4951");
    }

    HE_EXPECT_EQ(Format("{:#.0f}", 123.0), "123.");
    HE_EXPECT_EQ(Format("{:.02f}", 1.234), "1.23");
    HE_EXPECT_EQ(Format("{:.1g}", 0.001), "0.001");
    HE_EXPECT_EQ(Format("{}", 1019666432.0f), "1019666400");
    HE_EXPECT_EQ(Format("{:.0e}", 9.5), "1.e+01");
    HE_EXPECT_EQ(Format("{:.1e}", 1e-34), "1.0e-34");

    HE_EXPECT_EQ(Format("{:.0}", Limits<double>::MinPos), "2e-308");
    HE_EXPECT_EQ(Format("{:0g}", Limits<double>::DenormMin), "4.94066e-324");

    HE_EXPECT_EQ(Format("{0:.2}", "str"), "st");
    HE_EXPECT_EQ(Format("{0:.5}", "вожыкі"), "вожык");

    HE_EXPECT_FMT_ERR("{0:{.2147483648", "Argument not found");
    HE_EXPECT_FMT_ERR("{0:.2147483648}", "Precision number is too big", 0);
    HE_EXPECT_FMT_ERR("{0:.", "Missing precision specifier", 0);
    HE_EXPECT_FMT_ERR("{0:.}", "Missing precision specifier", 0);
    HE_EXPECT_FMT_ERR("{:.x}", "Missing precision specifier", 5);
    HE_EXPECT_FMT_ERR("{:.{0x}}", "Missing precision specifier", 5);
    HE_EXPECT_FMT_ERR("{:.{-}}", "Missing precision specifier", 5);
    HE_EXPECT_FMT_ERR("{0:.2", "Precision not allowed for integral types", 0);
    HE_EXPECT_FMT_ERR("{0:.2}", "Precision not allowed for integral types", 42);
    HE_EXPECT_FMT_ERR("{0:.2}", "Precision not allowed for integral types", 42u);
    HE_EXPECT_FMT_ERR("{0:.2}", "Precision not allowed for integral types", 42l);
    HE_EXPECT_FMT_ERR("{0:.2}", "Precision not allowed for integral types", 42ul);
    HE_EXPECT_FMT_ERR("{0:.2}", "Precision not allowed for integral types", 42ll);
    HE_EXPECT_FMT_ERR("{0:.2}", "Precision not allowed for integral types", 42ull);
    HE_EXPECT_FMT_ERR("{0:3.0}", "Precision not allowed for integral types", 'a');
    HE_EXPECT_FMT_ERR("{0:.2}", "Precision not allowed for pointer types", reinterpret_cast<void*>(0xcafe));
    HE_EXPECT_FMT_ERR("{0:.2f}", "Precision not allowed for pointer types", reinterpret_cast<void*>(0xcafe));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, bools)
{
    HE_EXPECT_EQ(Format("{}", true), "true");
    HE_EXPECT_EQ(Format("{}", false), "false");
    HE_EXPECT_EQ(Format("{:d}", true), "1");
    HE_EXPECT_EQ(Format("{:5}", true), "true ");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, chars)
{
    HE_EXPECT_EQ(Format("{0}", 'a'), "a");
    HE_EXPECT_EQ(Format("{0:c}", 'z'), "z");

    HE_EXPECT_EQ(Format("{0:d}", 'x'), Format("{0:d}", static_cast<int>('x')));
    HE_EXPECT_EQ(Format("{0:c}", 'x'), Format("{0:c}", static_cast<int>('x')));
    HE_EXPECT_EQ(Format("{0:o}", 'x'), Format("{0:o}", static_cast<int>('x')));
    HE_EXPECT_EQ(Format("{0:x}", 'x'), Format("{0:x}", static_cast<int>('x')));
    HE_EXPECT_EQ(Format("{0:X}", 'x'), Format("{0:X}", static_cast<int>('x')));
    HE_EXPECT_EQ(Format("{0:b}", 'x'), Format("{0:b}", static_cast<int>('x')));
    HE_EXPECT_EQ(Format("{0:B}", 'x'), Format("{0:B}", static_cast<int>('x')));

    HE_EXPECT_EQ(Format("{:02X}", 'x'), Format("{:02X}", static_cast<int>('x')));

    HE_EXPECT_EQ(Format("{}", '\n'), "\n");

    // TODO: volatile doesn't work in MakeArg right now.
    //volatile char c = 'x';
    //HE_EXPECT_EQ(Format("{}", c), "x");

    HE_EXPECT_EQ(Format("{}", static_cast<unsigned char>(42)), "42");
    HE_EXPECT_EQ(Format("{}", static_cast<uint8_t>(42)), "42");

    TestUnknownSpecTypes<char>('a', "dcoxXbB");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, ints)
{
    HE_EXPECT_EQ(Format("{0:d}", static_cast<short>(42)), "42");
    HE_EXPECT_EQ(Format("{0:d}", static_cast<unsigned short>(42)), "42");
    HE_EXPECT_EQ(Format("{:c}", static_cast<int>(32)), " ");
    HE_EXPECT_EQ(Format("{:c}", static_cast<int>('x')), "x");

    // Binary
    HE_EXPECT_EQ(Format("{0:b}", 0), "0");
    HE_EXPECT_EQ(Format("{0:b}", 42), "101010");
    HE_EXPECT_EQ(Format("{0:b}", 42u), "101010");
    HE_EXPECT_EQ(Format("{0:b}", -42), "-101010");
    HE_EXPECT_EQ(Format("{0:b}", 12345), "11000000111001");
    HE_EXPECT_EQ(Format("{0:b}", 0x12345678), "10010001101000101011001111000");
    HE_EXPECT_EQ(Format("{0:b}", 0x90ABCDEF), "10010000101010111100110111101111");
    HE_EXPECT_EQ(Format("{0:b}", Limits<uint32_t>::Max), "11111111111111111111111111111111");

    // Decimal
    HE_EXPECT_EQ(Format("{0}", 0), "0");
    HE_EXPECT_EQ(Format("{0}", 42), "42");
    HE_EXPECT_EQ(Format("{0:d}", 42), "42");
    HE_EXPECT_EQ(Format("{0}", 42u), "42");
    HE_EXPECT_EQ(Format("{0}", -42), "-42");
    HE_EXPECT_EQ(Format("{0}", 12345), "12345");
    HE_EXPECT_EQ(Format("{0}", 67890), "67890");
    HE_EXPECT_EQ(Format("{0}", Limits<int32_t>::Min), "-2147483648");
    HE_EXPECT_EQ(Format("{0}", Limits<int32_t>::Max), "2147483647");
    HE_EXPECT_EQ(Format("{0}", Limits<int64_t>::Min), "-9223372036854775808");
    HE_EXPECT_EQ(Format("{0}", Limits<int64_t>::Max), "9223372036854775807");
    HE_EXPECT_EQ(Format("{0}", Limits<uint32_t>::Max), "4294967295");
    HE_EXPECT_EQ(Format("{0}", Limits<uint64_t>::Max), "18446744073709551615");

    // Hex
    HE_EXPECT_EQ(Format("{0:x}", 0), "0");
    HE_EXPECT_EQ(Format("{0:x}", 0x42), "42");
    HE_EXPECT_EQ(Format("{0:x}", 0x42u), "42");
    HE_EXPECT_EQ(Format("{0:x}", -0x42), "-42");
    HE_EXPECT_EQ(Format("{0:x}", 0x12345678), "12345678");
    HE_EXPECT_EQ(Format("{0:x}", 0x90abcdef), "90abcdef");
    HE_EXPECT_EQ(Format("{0:X}", 0x12345678), "12345678");
    HE_EXPECT_EQ(Format("{0:X}", 0x90ABCDEF), "90ABCDEF");
    HE_EXPECT_EQ(Format("{0:x}", Limits<int32_t>::Min), "-80000000");
    HE_EXPECT_EQ(Format("{0:x}", Limits<int32_t>::Max), "7fffffff");
    HE_EXPECT_EQ(Format("{0:x}", Limits<int64_t>::Min), "-8000000000000000");
    HE_EXPECT_EQ(Format("{0:x}", Limits<int64_t>::Max), "7fffffffffffffff");
    HE_EXPECT_EQ(Format("{0:x}", Limits<uint32_t>::Max), "ffffffff");
    HE_EXPECT_EQ(Format("{0:x}", Limits<uint64_t>::Max), "ffffffffffffffff");

    // Octal
    HE_EXPECT_EQ(Format("{0:o}", 0), "0");
    HE_EXPECT_EQ(Format("{0:o}", 042), "42");
    HE_EXPECT_EQ(Format("{0:o}", 042u), "42");
    HE_EXPECT_EQ(Format("{0:o}", -042), "-42");
    HE_EXPECT_EQ(Format("{0:o}", 012345670), "12345670");
    HE_EXPECT_EQ(Format("{0:o}", Limits<int32_t>::Min), "-20000000000");
    HE_EXPECT_EQ(Format("{0:o}", Limits<int32_t>::Max), "17777777777");
    HE_EXPECT_EQ(Format("{0:o}", Limits<int64_t>::Min), "-1000000000000000000000");
    HE_EXPECT_EQ(Format("{0:o}", Limits<int64_t>::Max), "777777777777777777777");
    HE_EXPECT_EQ(Format("{0:o}", Limits<uint32_t>::Max), "37777777777");
    HE_EXPECT_EQ(Format("{0:o}", Limits<uint64_t>::Max), "1777777777777777777777");

    TestUnknownSpecTypes<int>(42, "dcoxXbB");
    HE_EXPECT_FMT_ERR("{0:v", "Invalid type specifier", 42);
}

// ------------------------------------------------------------------------------------------------
template <typename T>
static void TestNanAndInf()
{
    const T nan = Limits<T>::NaN;
    HE_EXPECT(std::signbit(-nan), "Checking that the compiler handles negative NaN correctly.");
    HE_EXPECT_EQ(Format("{}", nan), "nan");
    HE_EXPECT_EQ(Format("{:+}", nan), "+nan");
    HE_EXPECT_EQ(Format("{:+06}", nan), "  +nan");
    HE_EXPECT_EQ(Format("{:<+06}", nan), "+nan  ");
    HE_EXPECT_EQ(Format("{:^+06}", nan), " +nan ");
    HE_EXPECT_EQ(Format("{:>+06}", nan), "  +nan");
    HE_EXPECT_EQ(Format("{}", -nan), "-nan");
    HE_EXPECT_EQ(Format("{:+06}", -nan), "  -nan");
    HE_EXPECT_EQ(Format("{: }", nan), " nan");
    HE_EXPECT_EQ(Format("{:F}", nan), "NAN");
    HE_EXPECT_EQ(Format("{:<7}", nan), "nan    ");
    HE_EXPECT_EQ(Format("{:^7}", nan), "  nan  ");
    HE_EXPECT_EQ(Format("{:>7}", nan), "    nan");

    const T snan = Limits<T>::SignalingNaN;
    HE_EXPECT(std::signbit(-snan), "Checking that the compiler handles negative NaN correctly.");
    HE_EXPECT_EQ(Format("{}", snan), "nan");
    HE_EXPECT_EQ(Format("{:+}", snan), "+nan");
    HE_EXPECT_EQ(Format("{:+06}", snan), "  +nan");
    HE_EXPECT_EQ(Format("{:<+06}", snan), "+nan  ");
    HE_EXPECT_EQ(Format("{:^+06}", snan), " +nan ");
    HE_EXPECT_EQ(Format("{:>+06}", snan), "  +nan");
    HE_EXPECT_EQ(Format("{}", -snan), "-nan");
    HE_EXPECT_EQ(Format("{:+06}", -snan), "  -nan");
    HE_EXPECT_EQ(Format("{: }", snan), " nan");
    HE_EXPECT_EQ(Format("{:F}", snan), "NAN");
    HE_EXPECT_EQ(Format("{:<7}", snan), "nan    ");
    HE_EXPECT_EQ(Format("{:^7}", snan), "  nan  ");
    HE_EXPECT_EQ(Format("{:>7}", snan), "    nan");

    const T inf = Limits<T>::Infinity;
    HE_EXPECT_EQ(Format("{}", inf), "inf");
    HE_EXPECT_EQ(Format("{:+}", inf), "+inf");
    HE_EXPECT_EQ(Format("{}", -inf), "-inf");
    HE_EXPECT_EQ(Format("{:+06}", inf), "  +inf");
    HE_EXPECT_EQ(Format("{:+06}", -inf), "  -inf");
    HE_EXPECT_EQ(Format("{:<+06}", inf), "+inf  ");
    HE_EXPECT_EQ(Format("{:^+06}", inf), " +inf ");
    HE_EXPECT_EQ(Format("{:>+06}", inf), "  +inf");
    HE_EXPECT_EQ(Format("{: }", inf), " inf");
    HE_EXPECT_EQ(Format("{:F}", inf), "INF");
    HE_EXPECT_EQ(Format("{:<7}", inf), "inf    ");
    HE_EXPECT_EQ(Format("{:^7}", inf), "  inf  ");
    HE_EXPECT_EQ(Format("{:>7}", inf), "    inf");
}

HE_TEST(core, fmt, floats)
{
    HE_EXPECT_EQ(Format("{}", 0.0f), "0");
    HE_EXPECT_EQ(Format("{0:f}", 392.5f), "392.500000");
    HE_EXPECT_EQ(Format("{}", 0.0), "0");
    HE_EXPECT_EQ(Format("{:}", 0.0), "0");
    HE_EXPECT_EQ(Format("{:f}", 0.0), "0.000000");
    HE_EXPECT_EQ(Format("{:g}", 0.0), "0");
    HE_EXPECT_EQ(Format("{:}", 392.65), "392.65");
    HE_EXPECT_EQ(Format("{:g}", 392.65), "392.65");
    HE_EXPECT_EQ(Format("{:G}", 392.65), "392.65");
    HE_EXPECT_EQ(Format("{:g}", 4.9014e6), "4.9014e+06");
    HE_EXPECT_EQ(Format("{:f}", 392.65), "392.650000");
    HE_EXPECT_EQ(Format("{:F}", 392.65), "392.650000");
    // TODO: Locale handling for separators
    //HE_EXPECT_EQ(Format("{:L}", 42.0), "42.0");
    HE_EXPECT_EQ(Format("{:24a}", 4.2), "    0x1.0cccccccccccdp+2");
    HE_EXPECT_EQ(Format("{:<24a}", 4.2), "0x1.0cccccccccccdp+2    ");
    HE_EXPECT_EQ(Format("{0:e}", 392.65), "3.926500e+02");
    HE_EXPECT_EQ(Format("{0:E}", 392.65), "3.926500E+02");
    HE_EXPECT_EQ(Format("{0:+010.4g}", 392.65), "+0000392.6");

    double xd = 0x1.ffffffffffp+2;
    HE_EXPECT_EQ(Format("{:.10a}", xd), "0x1.ffffffffffp+2");
    HE_EXPECT_EQ(Format("{:.9a}", xd), "0x2.000000000p+2");

    if constexpr (Limits<long double>::Digits == 64)
    {
        auto ld = 0xf.ffffffffffp-3l;
        HE_EXPECT_EQ(Format("{:a}", ld), "0xf.ffffffffffp-3");
        HE_EXPECT_EQ(Format("{:.10a}", ld), "0xf.ffffffffffp-3");
        HE_EXPECT_EQ(Format("{:.9a}", ld), "0x1.000000000p+1");
    }

    HE_EXPECT_EQ(Format("{:a}", Limits<double>::MinPos), "0x1p-1022");
    HE_EXPECT_EQ(Format("{:#a}", Limits<double>::MinPos), "0x1.p-1022");

    HE_EXPECT_EQ(Format("{:a}", Limits<double>::Min), "-0x1.fffffffffffffp+1023");
    HE_EXPECT_EQ(Format("{:a}", Limits<double>::Max), "0x1.fffffffffffffp+1023");
    HE_EXPECT_EQ(Format("{:a}", Limits<double>::DenormMin), "0x0.0000000000001p-1022");

    if constexpr (Limits<long double>::Digits == 64)
    {
        HE_EXPECT_EQ(Format("{:a}", Limits<long double>::MinPos), "0x8p-16385");
        HE_EXPECT_EQ(Format("{:a}", Limits<long double>::Min), "-0xf.fffffffffffffffp+16380");
        HE_EXPECT_EQ(Format("{:a}", Limits<long double>::Max), "0xf.fffffffffffffffp+16380");
        HE_EXPECT_EQ(Format("{:a}", Limits<long double>::DenormMin), "0x0.000000000000001p-16382");
    }

    HE_EXPECT_EQ(Format("{:.10a}", 4.2), "0x1.0ccccccccdp+2");

    HE_EXPECT_EQ(Format("{:a}", -42.0), "-0x1.5p+5");
    HE_EXPECT_EQ(Format("{:A}", -42.0), "-0X1.5P+5");
    HE_EXPECT_EQ(Format("{:f}", 9223372036854775807.0), "9223372036854775808.000000");

    HE_EXPECT_EQ(Format("{}", 1e-4), "0.0001");
    HE_EXPECT_EQ(Format("{}", 1e-5), "1e-05");
    HE_EXPECT_EQ(Format("{}", 1e15), "1000000000000000");
    HE_EXPECT_EQ(Format("{}", 1e16), "1e+16");
    HE_EXPECT_EQ(Format("{}", 9.999e-5), "9.999e-05");
    HE_EXPECT_EQ(Format("{}", 1e10), "10000000000");
    HE_EXPECT_EQ(Format("{}", 1e11), "100000000000");
    HE_EXPECT_EQ(Format("{}", 1234e7), "12340000000");
    HE_EXPECT_EQ(Format("{}", 1234e-2), "12.34");
    HE_EXPECT_EQ(Format("{}", 1234e-6), "0.001234");
    HE_EXPECT_EQ(Format("{}", 0.1f), "0.1");
    HE_EXPECT_EQ(Format("{}", double(0.1f)), "0.10000000149011612");
    HE_EXPECT_EQ(Format("{}", 1.35631564e-19f), "1.3563156e-19");

    HE_EXPECT_EQ(Format("{0:}", 0.0l), "0");
    HE_EXPECT_EQ(Format("{0:f}", 0.0l), "0.000000");
    HE_EXPECT_EQ(Format("{:.1f}", 0.000000001l), "0.0");
    HE_EXPECT_EQ(Format("{:.2f}", 0.099l), "0.10");
    HE_EXPECT_EQ(Format("{0:}", 392.65l), "392.65");
    HE_EXPECT_EQ(Format("{0:g}", 392.65l), "392.65");
    HE_EXPECT_EQ(Format("{0:G}", 392.65l), "392.65");
    HE_EXPECT_EQ(Format("{0:f}", 392.65l), "392.650000");
    HE_EXPECT_EQ(Format("{0:F}", 392.65l), "392.650000");

    //char buffer[256];
    //std::snprintf(buffer, sizeof(buffer), "%Le", 392.65l);
    //HE_EXPECT_EQ(Format("{0:e}", 392.65l), buffer);
    HE_EXPECT_EQ(Format("{0:+010.4g}", 392.64l), "+0000392.6");

    auto ld = 3.31l;
    if constexpr (Limits<long double>::Format == FloatFormat::DoubleDouble128)
    {
        //std::snprintf(buffer, sizeof(buffer), "%a", static_cast<double>(ld));
        //HE_EXPECT_EQ(Format("{:a}", ld), buffer);
    }
    else if constexpr (Limits<long double>::Digits == 64)
    {
        HE_EXPECT_EQ(Format("{:a}", ld), "0xd.3d70a3d70a3d70ap-2");
    }

    TestNanAndInf<float>();
    TestNanAndInf<double>();
    TestNanAndInf<long double>();

    TestUnknownSpecTypes<float>(1.2f, "aAeEfFgG");
    TestUnknownSpecTypes<double>(1.2, "aAeEfFgG");
    TestUnknownSpecTypes<long double>(1.2, "aAeEfFgG");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, pointers)
{
    HE_EXPECT_EQ(Format("{0}", static_cast<void*>(nullptr)), "0x0");
    HE_EXPECT_EQ(Format("{0}", reinterpret_cast<void*>(0x1234)), "0x1234");
    HE_EXPECT_EQ(Format("{0}", reinterpret_cast<void*>(0x1234)), "0x1234");
#if HE_CPU_64_BIT
    HE_EXPECT_EQ(Format("{0}", reinterpret_cast<void*>(~uintptr_t())), "0xffffffffffffffff");
#else
    HE_EXPECT_EQ(Format("{0}", reinterpret_cast<void*>(~uintptr_t())), "0xffffffff");
#endif
    HE_EXPECT_EQ(Format("{}", FmtPtr(reinterpret_cast<int*>(0x1234))), "0x1234");
    HE_EXPECT_EQ(Format("{}", BitCast<void*>(&TestFmtStringForError<int>)), Format("{}", FmtPtr(TestFmtStringForError<int>)));
    HE_EXPECT_EQ(Format("{}", nullptr), "0x0");

    TestUnknownSpecTypes<void*>(reinterpret_cast<void*>(0x1234), "p");
}

// ------------------------------------------------------------------------------------------------
namespace he
{
    enum class Color { Red, Green, Blue };

    template <>
    const char* EnumTraits<Color>::ToString(Color x) noexcept
    {
        switch (x)
        {
            case Color::Red: return "Red";
            case Color::Green: return "Green";
            case Color::Blue: return "Blue";
        }
        return "<unknown>";
    }
}
HE_TEST(core, fmt, enums)
{
    HE_EXPECT_EQ(Format("{}", EnumToValue(Color::Red)), "0");
    HE_EXPECT_EQ(Format("{}", EnumToValue(Color::Blue)), "2");

    HE_EXPECT_EQ(Format("{}", Color::Red), "Red");
    HE_EXPECT_EQ(Format("{}", Color::Blue), "Blue");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, strings)
{
    StringView sv = "SV Test";
    String s = "S Test";
    const char* cs = "CS Test";
    char ca[] = "CA Test";

    HE_EXPECT_EQ(Format("{}", sv), "SV Test");
    HE_EXPECT_EQ(Format("{}", s), "S Test");
    HE_EXPECT_EQ(Format("{}", cs), "CS Test");
    HE_EXPECT_EQ(Format("{}", ca), "CA Test");

    HE_EXPECT_VERIFY({
        // this will verify the pointer is non-null
        HE_EXPECT_EQ(Format("{}", static_cast<const char*>(nullptr)), "");
    });

    HE_EXPECT_EQ(Format("{:*^8}", "test"), "**test**");

    TestUnknownSpecTypes<const char*>("test", "sp");
    HE_EXPECT_FMT_ERR("{:x}", "Unknown format specifier", "test");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, join)
{
    int v1[3] = { 1, 2, 3 };
    auto v2 = Vector<float>();
    v2.PushBack(1.2f);
    v2.PushBack(3.4f);
    void* v3[2] = { &v1[0], &v1[1] };

    HE_EXPECT_EQ(Format("({})", FmtJoin(v1, v1 + 3, ", ")), "(1, 2, 3)");
    HE_EXPECT_EQ(Format("({})", FmtJoin(v1, v1 + 1, ", ")), "(1)");
    HE_EXPECT_EQ(Format("({})", FmtJoin(v1, v1, ", ")), "()");
    HE_EXPECT_EQ(Format("({:03})", FmtJoin(v1, v1 + 3, ", ")), "(001, 002, 003)");
    HE_EXPECT_EQ(Format("({:+06.2f})", FmtJoin(v2.Begin(), v2.End(), ", ")), "(+01.20, +03.40)");

    HE_EXPECT_EQ(Format("{}, {}", v3[0], v3[1]), Format("{}", FmtJoin(v3, v3 + 2, ", ")));

    HE_EXPECT_EQ(Format("({})", FmtJoin(v1, ", ")), "(1, 2, 3)");
    HE_EXPECT_EQ(Format("({:+06.2f})", FmtJoin(v2, ", ")), "(+01.20, +03.40)");

    Vector<Color> v4;
    v4.PushBack(Color::Red);
    v4.PushBack(Color::Green);
    v4.PushBack(Color::Red);
    HE_EXPECT_EQ(Format("{}", FmtJoin(v4, " ")), "Red Green Red");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, Format)
{
    HE_EXPECT_EQ(Format("{0}, {1}, {2}", 'a', 'b', 'c'), "a, b, c");
    HE_EXPECT_EQ(Format("{}, {}, {}", 'a', 'b', 'c'), "a, b, c");
    HE_EXPECT_EQ(Format("{2}, {1}, {0}", 'a', 'b', 'c'), "c, b, a");
    HE_EXPECT_EQ(Format("{0}{1}{0}", "abra", "cad"), "abracadabra");

    HE_EXPECT_EQ(Format("{:<30}", "left aligned"), "left aligned                  ");
    HE_EXPECT_EQ(Format("{:>30}", "right aligned"), "                 right aligned");
    HE_EXPECT_EQ(Format("{:^30}", "centered"), "           centered           ");
    HE_EXPECT_EQ(Format("{:*^30}", "centered"), "***********centered***********");

    HE_EXPECT_EQ(Format("{:+f}; {:+f}", 3.14, -3.14), "+3.140000; -3.140000");
    HE_EXPECT_EQ(Format("{: f}; {: f}", 3.14, -3.14), " 3.140000; -3.140000");
    HE_EXPECT_EQ(Format("{:-f}; {:-f}", 3.14, -3.14), "3.140000; -3.140000");

    HE_EXPECT_EQ(Format("int: {0:d};  hex: {0:x};  oct: {0:o}", 42), "int: 42;  hex: 2a;  oct: 52");
    HE_EXPECT_EQ(Format("int: {0:d};  hex: {0:#x};  oct: {0:#o}", 42), "int: 42;  hex: 0x2a;  oct: 052");

    HE_EXPECT_EQ(Format("{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 'a', 'b', 'c', 'd', 'e', 'f', 'g'), "0123456789abcdefg");

    HE_EXPECT_FMT_ERR("The answer is {:d}", "Unknown format specifier", "forty-two");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, FormatTo)
{
    String buf;
    FormatTo(buf, "{0}, {1}, {2}", 'a', 'b', 'c');
    HE_EXPECT_EQ(buf, "a, b, c");

    FormatTo(buf, "{}, {}, {}", 'a', 'b', 'c');
    HE_EXPECT_EQ(buf, "a, b, ca, b, c");

    FormatTo(buf, "{2}, {1}, {0}", 'a', 'b', 'c');
    HE_EXPECT_EQ(buf, "a, b, ca, b, cc, b, a");

    FormatTo(buf, "{0}{1}{0}", "abra", "cad");
    HE_EXPECT_EQ(buf, "a, b, ca, b, cc, b, aabracadabra");

    FormatTo(buf, "{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}{}", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 'a', 'b', 'c', 'd', 'e', 'f', 'g');
    HE_EXPECT_EQ(buf, "a, b, ca, b, cc, b, aabracadabra0123456789abcdefg");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, fmt, errors)
{
    HE_EXPECT_FMT_ERR("{0}{}", "Cannot switch from manual to automatic argument indexing", 'a', 'b');
    HE_EXPECT_FMT_ERR("{0}:.{}", "Cannot switch from manual to automatic argument indexing", 'a', 'b');

    HE_EXPECT_FMT_ERR("{}{0}", "Cannot switch from automatic to manual argument indexing", 'a', 'b');
    HE_EXPECT_FMT_ERR("{}:.{0}", "Cannot switch from automatic to manual argument indexing", 'a', 'b');

    HE_EXPECT_FMT_ERR("{}", "Argument not found");
    HE_EXPECT_FMT_ERR("{0}", "Argument not found");
    HE_EXPECT_FMT_ERR("{1}", "Argument not found", 5);
    HE_EXPECT_FMT_ERR("{2147483647}", "Argument not found");
    HE_EXPECT_FMT_ERR("{4294967295}", "Argument not found");
    HE_EXPECT_FMT_ERR("{:x}", "Argument not found");

    HE_EXPECT_FMT_ERR("{:+}", "Invalid format specifier for char", 'a');

    HE_EXPECT_FMT_ERR("{:s}", "Invalid type specifier", 5);
    HE_EXPECT_FMT_ERR("{:s}", "Invalid type specifier", 'a');
    HE_EXPECT_FMT_ERR("{:s}", "Invalid type specifier", 5.0);
    HE_EXPECT_FMT_ERR("{0:s", "Invalid type specifier", 5);
    HE_EXPECT_FMT_ERR("{:\x80\x80\x80\x80\x80>}", "Invalid type specifier", 0);

    HE_EXPECT_FMT_ERR("{:d}", "Unknown format specifier", "test");
    HE_EXPECT_FMT_ERR("{:s}", "Unknown format specifier", reinterpret_cast<void*>(0x5));

    HE_EXPECT_FMT_ERR("{:{<}", "Invalid fill character '{'", 5);
    HE_EXPECT_FMT_ERR("{0:{<5}", "Invalid fill character '{'", 5);
    HE_EXPECT_FMT_ERR("{0:{<5}}", "Invalid fill character '{'", 5);

    HE_EXPECT_FMT_ERR("{:10000000000}", "Width number is too big", 5);
    HE_EXPECT_FMT_ERR("{:.10000000000}", "Precision number is too big", 5);

    HE_EXPECT_FMT_ERR("}", "Unmatched '}' in format string");

    HE_EXPECT_FMT_ERR("{0{}", "Invalid format string");
    HE_EXPECT_FMT_ERR("{?}", "Invalid format string");
    HE_EXPECT_FMT_ERR("{0", "Invalid format string");
    HE_EXPECT_FMT_ERR("{00}", "Invalid format string", 1);
    HE_EXPECT_FMT_ERR("{2147483647", "Invalid format string");
    HE_EXPECT_FMT_ERR("{4294967295", "Invalid format string");
    HE_EXPECT_FMT_ERR("{name}", "Invalid format string");
    HE_EXPECT_FMT_ERR("{n0}", "Invalid format string");
    HE_EXPECT_FMT_ERR("{0n}", "Invalid format string");
    HE_EXPECT_FMT_ERR("{", "Invalid format string");
    HE_EXPECT_FMT_ERR("{0x}", "Invalid format string");
    HE_EXPECT_FMT_ERR("{-}", "Invalid format string");

    // dynamic formatters not supported
    HE_EXPECT_FMT_ERR("{:{}}", "Invalid type specifier", 5);
    HE_EXPECT_FMT_ERR("{:{0x}}", "Invalid type specifier", 5);
    HE_EXPECT_FMT_ERR("{:{-}}", "Invalid type specifier", 5);
}
