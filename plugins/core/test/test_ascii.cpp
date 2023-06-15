// Copyright Chad Engler

#include "he/core/ascii.h"

#include "he/core/string_ops.h"
#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, ascii, IsWhitespace)
{
    HE_EXPECT(IsWhitespace(' '));
    HE_EXPECT(IsWhitespace('\t'));
    HE_EXPECT(IsWhitespace('\n'));
    HE_EXPECT(IsWhitespace('\v'));
    HE_EXPECT(IsWhitespace('\f'));
    HE_EXPECT(IsWhitespace('\r'));

    HE_EXPECT(!IsWhitespace('a'));
    HE_EXPECT(!IsWhitespace('f'));
    HE_EXPECT(!IsWhitespace('z'));
    HE_EXPECT(!IsWhitespace('A'));
    HE_EXPECT(!IsWhitespace('F'));
    HE_EXPECT(!IsWhitespace('Z'));
    HE_EXPECT(!IsWhitespace('0'));
    HE_EXPECT(!IsWhitespace('5'));
    HE_EXPECT(!IsWhitespace('9'));
    HE_EXPECT(!IsWhitespace('~'));
    HE_EXPECT(!IsWhitespace('['));
    HE_EXPECT(!IsWhitespace('}'));
    HE_EXPECT(!IsWhitespace('\0'));

    HE_EXPECT(IsWhitespace("    \n\t\r\n\v    \f"));
    HE_EXPECT(!IsWhitespace("    \n\t\r1\v    \f"));
    HE_EXPECT(!IsWhitespace("  A  \t1\v   xx \f"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, ascii, IsUpper)
{
    HE_EXPECT(IsUpper('A'));
    HE_EXPECT(IsUpper('F'));
    HE_EXPECT(IsUpper('L'));
    HE_EXPECT(IsUpper('Q'));
    HE_EXPECT(IsUpper('U'));
    HE_EXPECT(IsUpper('Z'));

    HE_EXPECT(!IsUpper('a'));
    HE_EXPECT(!IsUpper('f'));
    HE_EXPECT(!IsUpper('l'));
    HE_EXPECT(!IsUpper('q'));
    HE_EXPECT(!IsUpper('u'));
    HE_EXPECT(!IsUpper('z'));
    HE_EXPECT(!IsUpper('~'));
    HE_EXPECT(!IsUpper('['));
    HE_EXPECT(!IsUpper('}'));
    HE_EXPECT(!IsUpper(' '));
    HE_EXPECT(!IsUpper('\n'));
    HE_EXPECT(!IsUpper('\0'));

    HE_EXPECT(IsUpper("ABCD"));
    HE_EXPECT(!IsUpper("ABcD"));
    HE_EXPECT(!IsUpper("\nABC"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, ascii, IsLower)
{
    HE_EXPECT(IsLower('a'));
    HE_EXPECT(IsLower('f'));
    HE_EXPECT(IsLower('l'));
    HE_EXPECT(IsLower('q'));
    HE_EXPECT(IsLower('u'));
    HE_EXPECT(IsLower('z'));

    HE_EXPECT(!IsLower('A'));
    HE_EXPECT(!IsLower('F'));
    HE_EXPECT(!IsLower('L'));
    HE_EXPECT(!IsLower('Q'));
    HE_EXPECT(!IsLower('U'));
    HE_EXPECT(!IsLower('Z'));
    HE_EXPECT(!IsLower('~'));
    HE_EXPECT(!IsLower('['));
    HE_EXPECT(!IsLower('}'));
    HE_EXPECT(!IsLower(' '));
    HE_EXPECT(!IsLower('\n'));
    HE_EXPECT(!IsLower('\0'));

    HE_EXPECT(IsLower("abcd"));
    HE_EXPECT(!IsLower("abCd"));
    HE_EXPECT(!IsLower("\nabc"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, ascii, IsAlpha)
{
    HE_EXPECT(IsAlpha('a'));
    HE_EXPECT(IsAlpha('f'));
    HE_EXPECT(IsAlpha('l'));
    HE_EXPECT(IsAlpha('q'));
    HE_EXPECT(IsAlpha('u'));
    HE_EXPECT(IsAlpha('z'));

    HE_EXPECT(!IsAlpha('0'));
    HE_EXPECT(!IsAlpha('5'));
    HE_EXPECT(!IsAlpha('9'));
    HE_EXPECT(!IsAlpha('~'));
    HE_EXPECT(!IsAlpha('['));
    HE_EXPECT(!IsAlpha('}'));
    HE_EXPECT(!IsAlpha(' '));
    HE_EXPECT(!IsAlpha('\n'));
    HE_EXPECT(!IsAlpha('\0'));

    HE_EXPECT(IsAlpha("abCD"));
    HE_EXPECT(!IsAlpha("abCD1"));
    HE_EXPECT(!IsAlpha("\nabCD"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, ascii, IsNumeric)
{
    HE_EXPECT(IsNumeric('0'));
    HE_EXPECT(IsNumeric('5'));
    HE_EXPECT(IsNumeric('9'));

    HE_EXPECT(!IsNumeric('a'));
    HE_EXPECT(!IsNumeric('f'));
    HE_EXPECT(!IsNumeric('l'));
    HE_EXPECT(!IsNumeric('q'));
    HE_EXPECT(!IsNumeric('u'));
    HE_EXPECT(!IsNumeric('z'));
    HE_EXPECT(!IsNumeric('A'));
    HE_EXPECT(!IsNumeric('F'));
    HE_EXPECT(!IsNumeric('L'));
    HE_EXPECT(!IsNumeric('Q'));
    HE_EXPECT(!IsNumeric('U'));
    HE_EXPECT(!IsNumeric('Z'));
    HE_EXPECT(!IsNumeric('~'));
    HE_EXPECT(!IsNumeric('['));
    HE_EXPECT(!IsNumeric('}'));
    HE_EXPECT(!IsNumeric(' '));
    HE_EXPECT(!IsNumeric('\n'));
    HE_EXPECT(!IsNumeric('\0'));

    HE_EXPECT(IsNumeric("12345"));
    HE_EXPECT(!IsNumeric("123a45"));
    HE_EXPECT(!IsNumeric("\n12345"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, ascii, IsAlphaNum)
{
    HE_EXPECT(IsAlphaNum('0'));
    HE_EXPECT(IsAlphaNum('5'));
    HE_EXPECT(IsAlphaNum('9'));
    HE_EXPECT(IsAlphaNum('a'));
    HE_EXPECT(IsAlphaNum('f'));
    HE_EXPECT(IsAlphaNum('l'));
    HE_EXPECT(IsAlphaNum('q'));
    HE_EXPECT(IsAlphaNum('u'));
    HE_EXPECT(IsAlphaNum('z'));
    HE_EXPECT(IsAlphaNum('A'));
    HE_EXPECT(IsAlphaNum('F'));
    HE_EXPECT(IsAlphaNum('L'));
    HE_EXPECT(IsAlphaNum('Q'));
    HE_EXPECT(IsAlphaNum('U'));
    HE_EXPECT(IsAlphaNum('Z'));

    HE_EXPECT(!IsAlphaNum('~'));
    HE_EXPECT(!IsAlphaNum('['));
    HE_EXPECT(!IsAlphaNum('}'));
    HE_EXPECT(!IsAlphaNum(' '));
    HE_EXPECT(!IsAlphaNum('\n'));
    HE_EXPECT(!IsAlphaNum('\0'));

    HE_EXPECT(IsAlphaNum("AbC123"));
    HE_EXPECT(!IsAlphaNum("AbC123~"));
    HE_EXPECT(!IsAlphaNum("\nAbC123"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, ascii, IsInteger)
{
    HE_EXPECT(IsInteger("0"));
    HE_EXPECT(IsInteger("5"));
    HE_EXPECT(IsInteger("9"));
    HE_EXPECT(IsInteger("12345"));
    HE_EXPECT(IsInteger("8451856398232113568"));

    HE_EXPECT(IsInteger("-0"));
    HE_EXPECT(IsInteger("-5"));
    HE_EXPECT(IsInteger("-9"));
    HE_EXPECT(IsInteger("-12345"));
    HE_EXPECT(IsInteger("-8451856398232113568"));

    HE_EXPECT(!IsInteger("0."));
    HE_EXPECT(!IsInteger("5-"));
    HE_EXPECT(!IsInteger("9="));
    HE_EXPECT(!IsInteger("12!345"));
    HE_EXPECT(!IsInteger("84[5185]6398}{2321_13568"));
    HE_EXPECT(!IsInteger("~"));
    HE_EXPECT(!IsInteger("    "));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, ascii, IsFloat)
{
    HE_EXPECT(IsFloat("0."));
    HE_EXPECT(IsFloat("0.0"));
    HE_EXPECT(IsFloat("5.5"));
    HE_EXPECT(IsFloat("9.2"));
    HE_EXPECT(IsFloat("12345.54321"));
    HE_EXPECT(IsFloat("8451856398232113568.987654321"));

    HE_EXPECT(IsFloat("-0.0"));
    HE_EXPECT(IsFloat("-5.5"));
    HE_EXPECT(IsFloat("-9.2"));
    HE_EXPECT(IsFloat("-12345.54321"));
    HE_EXPECT(IsFloat("-8451856398232113568.987654321"));

    HE_EXPECT(!IsFloat("5-"));
    HE_EXPECT(!IsFloat("9="));
    HE_EXPECT(!IsFloat("12!345"));
    HE_EXPECT(!IsFloat("84[5185]6398}{2321_13568"));
    HE_EXPECT(!IsFloat("~"));
    HE_EXPECT(!IsFloat("    "));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, ascii, IsHex)
{
    HE_EXPECT(IsHex('a'));
    HE_EXPECT(IsHex('b'));
    HE_EXPECT(IsHex('c'));
    HE_EXPECT(IsHex('d'));
    HE_EXPECT(IsHex('e'));
    HE_EXPECT(IsHex('f'));
    HE_EXPECT(IsHex('A'));
    HE_EXPECT(IsHex('B'));
    HE_EXPECT(IsHex('C'));
    HE_EXPECT(IsHex('D'));
    HE_EXPECT(IsHex('E'));
    HE_EXPECT(IsHex('F'));
    HE_EXPECT(IsHex('0'));
    HE_EXPECT(IsHex('1'));
    HE_EXPECT(IsHex('2'));
    HE_EXPECT(IsHex('3'));
    HE_EXPECT(IsHex('4'));
    HE_EXPECT(IsHex('5'));
    HE_EXPECT(IsHex('6'));
    HE_EXPECT(IsHex('7'));
    HE_EXPECT(IsHex('8'));
    HE_EXPECT(IsHex('9'));

    HE_EXPECT(!IsHex('g'));
    HE_EXPECT(!IsHex('l'));
    HE_EXPECT(!IsHex('q'));
    HE_EXPECT(!IsHex('u'));
    HE_EXPECT(!IsHex('z'));
    HE_EXPECT(!IsHex('G'));
    HE_EXPECT(!IsHex('L'));
    HE_EXPECT(!IsHex('Q'));
    HE_EXPECT(!IsHex('U'));
    HE_EXPECT(!IsHex('Z'));
    HE_EXPECT(!IsHex('~'));
    HE_EXPECT(!IsHex('['));
    HE_EXPECT(!IsHex('}'));
    HE_EXPECT(!IsHex(' '));
    HE_EXPECT(!IsHex('\n'));
    HE_EXPECT(!IsHex('\0'));

    HE_EXPECT(IsHex("abcdefABCDEF0123456789"));
    HE_EXPECT(!IsHex("abcdefABCDEFz0123456789"));
    HE_EXPECT(!IsHex("\nabcdefABCDEF0123456789"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, ascii, IsPrint)
{
    for (char c = ' '; c <= '~'; ++c)
    {
        HE_EXPECT(IsPrint(c));
    }

    HE_EXPECT(!IsPrint('\0'));
    HE_EXPECT(!IsPrint('\t'));
    HE_EXPECT(!IsPrint('\n'));
    HE_EXPECT(!IsPrint('\r'));

    HE_EXPECT(IsPrint(" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"));
    HE_EXPECT(!IsPrint("BCDEFGHIJKLMNOPQRSTUVWXYZ\tabcdefghijklmnopqrstuvwxyz~"));
    HE_EXPECT(!IsPrint("\nBCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, ascii, ToUpper)
{
    HE_EXPECT_EQ(ToUpper('a'), 'A');
    HE_EXPECT_EQ(ToUpper('f'), 'F');
    HE_EXPECT_EQ(ToUpper('l'), 'L');
    HE_EXPECT_EQ(ToUpper('q'), 'Q');
    HE_EXPECT_EQ(ToUpper('u'), 'U');
    HE_EXPECT_EQ(ToUpper('z'), 'Z');
    HE_EXPECT_EQ(ToUpper('A'), 'A');
    HE_EXPECT_EQ(ToUpper('F'), 'F');
    HE_EXPECT_EQ(ToUpper('L'), 'L');
    HE_EXPECT_EQ(ToUpper('Q'), 'Q');
    HE_EXPECT_EQ(ToUpper('U'), 'U');
    HE_EXPECT_EQ(ToUpper('Z'), 'Z');

    constexpr char input[] = "ThIs IS A tEST!!!";
    constexpr char expected[] = "THIS IS A TEST!!!";

    char buffer[260];
    StrCopy(buffer, input);
    ToUpper(buffer);
    HE_EXPECT_EQ_STR(buffer, expected);

    buffer[0] = '\0';
    ToUpper(buffer);
    HE_EXPECT_EQ_STR(buffer, "");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, ascii, ToLower)
{
    HE_EXPECT_EQ(ToLower('a'), 'a');
    HE_EXPECT_EQ(ToLower('f'), 'f');
    HE_EXPECT_EQ(ToLower('l'), 'l');
    HE_EXPECT_EQ(ToLower('q'), 'q');
    HE_EXPECT_EQ(ToLower('u'), 'u');
    HE_EXPECT_EQ(ToLower('z'), 'z');
    HE_EXPECT_EQ(ToLower('A'), 'a');
    HE_EXPECT_EQ(ToLower('F'), 'f');
    HE_EXPECT_EQ(ToLower('L'), 'l');
    HE_EXPECT_EQ(ToLower('Q'), 'q');
    HE_EXPECT_EQ(ToLower('U'), 'u');
    HE_EXPECT_EQ(ToLower('Z'), 'z');

    constexpr char input[] = "ThIs IS A tEST!!!";
    constexpr char expected[] = "this is a test!!!";

    char buffer[260];
    StrCopy(buffer, input);
    ToLower(buffer);
    HE_EXPECT_EQ_STR(buffer, expected);

    buffer[0] = '\0';
    ToLower(buffer);
    HE_EXPECT_EQ_STR(buffer, "");
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, ascii, ToHex)
{
    HE_EXPECT_EQ(ToHex(0x0), '0');
    HE_EXPECT_EQ(ToHex(0x5), '5');
    HE_EXPECT_EQ(ToHex(0x9), '9');

    HE_EXPECT_EQ(ToHex(0xa), 'a');
    HE_EXPECT_EQ(ToHex(0xd), 'd');
    HE_EXPECT_EQ(ToHex(0xf), 'f');

    HE_EXPECT_EQ(ToHex(0xa, false), 'a');
    HE_EXPECT_EQ(ToHex(0xd, false), 'd');
    HE_EXPECT_EQ(ToHex(0xf, false), 'f');

    HE_EXPECT_EQ(ToHex(0xa, true), 'A');
    HE_EXPECT_EQ(ToHex(0xd, true), 'D');
    HE_EXPECT_EQ(ToHex(0xf, true), 'F');
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, ascii, HexToNibble)
{
    HE_EXPECT_EQ(HexToNibble('0'), 0);
    HE_EXPECT_EQ(HexToNibble('1'), 1);
    HE_EXPECT_EQ(HexToNibble('2'), 2);
    HE_EXPECT_EQ(HexToNibble('3'), 3);
    HE_EXPECT_EQ(HexToNibble('4'), 4);
    HE_EXPECT_EQ(HexToNibble('5'), 5);
    HE_EXPECT_EQ(HexToNibble('6'), 6);
    HE_EXPECT_EQ(HexToNibble('7'), 7);
    HE_EXPECT_EQ(HexToNibble('8'), 8);
    HE_EXPECT_EQ(HexToNibble('9'), 9);
    HE_EXPECT_EQ(HexToNibble('a'), 10);
    HE_EXPECT_EQ(HexToNibble('b'), 11);
    HE_EXPECT_EQ(HexToNibble('c'), 12);
    HE_EXPECT_EQ(HexToNibble('d'), 13);
    HE_EXPECT_EQ(HexToNibble('e'), 14);
    HE_EXPECT_EQ(HexToNibble('f'), 15);
    HE_EXPECT_EQ(HexToNibble('A'), 10);
    HE_EXPECT_EQ(HexToNibble('B'), 11);
    HE_EXPECT_EQ(HexToNibble('C'), 12);
    HE_EXPECT_EQ(HexToNibble('D'), 13);
    HE_EXPECT_EQ(HexToNibble('E'), 14);
    HE_EXPECT_EQ(HexToNibble('F'), 15);

    HE_EXPECT_EQ(HexToNibble('g'), 0);
    HE_EXPECT_EQ(HexToNibble('q'), 0);
    HE_EXPECT_EQ(HexToNibble('v'), 0);
    HE_EXPECT_EQ(HexToNibble('G'), 0);
    HE_EXPECT_EQ(HexToNibble('Q'), 0);
    HE_EXPECT_EQ(HexToNibble('v'), 0);
    HE_EXPECT_EQ(HexToNibble('~'), 0);
    HE_EXPECT_EQ(HexToNibble('['), 0);
    HE_EXPECT_EQ(HexToNibble('}'), 0);
    HE_EXPECT_EQ(HexToNibble(' '), 0);
    HE_EXPECT_EQ(HexToNibble('\n'), 0);
    HE_EXPECT_EQ(HexToNibble('\0'), 0);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, ascii, HexPairToByte)
{
    HE_EXPECT_EQ(HexPairToByte('0', '0'), 0);
    HE_EXPECT_EQ(HexPairToByte('0', '9'), 9);
    HE_EXPECT_EQ(HexPairToByte('0', 'd'), 13);
    HE_EXPECT_EQ(HexPairToByte('0', 'f'), 15);
    HE_EXPECT_EQ(HexPairToByte('6', 'f'), 111);
    HE_EXPECT_EQ(HexPairToByte('a', 'a'), 170);
    HE_EXPECT_EQ(HexPairToByte('d', 'f'), 223);
    HE_EXPECT_EQ(HexPairToByte('f', 'f'), 255);
}
