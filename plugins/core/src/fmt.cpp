// Copyright Chad Engler

// This file is based on the implementation in {fmt}
// https://fmt.dev/
// https://github.com/fmtlib/fmt
//
// License:
//
// Copyright (c) 2012 - present, Victor Zverovich
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
// BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// --- Optional exception to the license ---
//
// As an exception, if, as a result of your compiling your source code, portions of this Software
// are embedded into a machine-executable object form of such source code, you may redistribute
// such embedded portions in such object form without including the above copyright and permission
// notices.

#include "he/core/fmt.h"

#include "he/core/assert.h"
#include "he/core/compiler.h"
#include "he/core/concepts.h"
#include "he/core/config.h"
#include "he/core/cpu.h"
#include "he/core/limits.h"
#include "he/core/string.h"
#include "he/core/string_ops.h"
#include "he/core/utils.h"
#include "he/core/wstr.h"

#include "dragonbox/dragonbox.h"

#include <cmath>
#include <cstdio>
#include <exception>

#if HE_COMPILER_MSVC
    extern "C" unsigned char _BitScanReverse(unsigned long* Index, unsigned long Mask);
    #pragma intrinsic(_BitScanReverse)

    #if HE_CPU_64_BIT
        extern "C" unsigned char _BitScanReverse64(unsigned long* Index, unsigned __int64 Mask);
        #pragma intrinsic(_BitScanReverse64)
    #endif

    HE_FORCE_INLINE uint32_t _heClz(uint32_t x)
    {
        unsigned long r = 0;
        _BitScanReverse(&r, x);
        return 31 ^ r;
    }

    HE_FORCE_INLINE uint32_t _heClzll(uint64_t x)
    {
        unsigned long r = 0;
    #if HE_CPU_64_BIT
        _BitScanReverse64(&r, x);
    #else
        if (_BitScanReverse(&r, static_cast<uint32_t>(x >> 32)))
            return 63 ^ (r + 32);
        _BitScanReverse(&r, static_cast<uint32_t>(x));
    #endif
        return 63 ^ r;
    }

    #define HE_CLZ(x)       _heClz(x)
    #define HE_CLZLL(x)     _heClzll(x)
#else
    #define HE_CLZ(x)       __builtin_clz(x)
    #define HE_CLZLL(x)     __builtin_clzll(x)
#endif

// Can't use the normal assert flow, because it could come back into Fmt again.
#if HE_ENABLE_ASSERTIONS
    #define HE_FMT_ASSERT(expr, msg) (HE_LIKELY(expr) ? void(0) : FmtError(msg))
#else
    #define HE_FMT_ASSERT(expr, msg)
#endif

namespace he
{
    // An optimization by Kendall Willets from https://bit.ly/3uOIQrB.
    // This increments the upper 32 bits (log10(T) - 1) when >= T is added.
    #define HE_INC(T) (((sizeof(#T) - 1ull) << 32) - T)
    static constexpr uint64_t DigitCountLookup[] =
    {
        HE_INC(0),          HE_INC(0),          HE_INC(0),          // 8
        HE_INC(10),         HE_INC(10),         HE_INC(10),         // 64
        HE_INC(100),        HE_INC(100),        HE_INC(100),        // 512
        HE_INC(1000),       HE_INC(1000),       HE_INC(1000),       // 4096
        HE_INC(10000),      HE_INC(10000),      HE_INC(10000),      // 32k
        HE_INC(100000),     HE_INC(100000),     HE_INC(100000),     // 256k
        HE_INC(1000000),    HE_INC(1000000),    HE_INC(1000000),    // 2048k
        HE_INC(10000000),   HE_INC(10000000),   HE_INC(10000000),   // 16M
        HE_INC(100000000),  HE_INC(100000000),  HE_INC(100000000),  // 128M
        HE_INC(1000000000), HE_INC(1000000000), HE_INC(1000000000), // 1024M
        HE_INC(1000000000), HE_INC(1000000000),                     // 4B
    };
    #undef HE_INC

    static constexpr uint8_t Bsr2Log10[] =
    {
        1,  1,  1,  2,  2,  2,  3,  3,  3,  4,  4,  4,  4,  5,  5,  5,
        6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9,  10, 10, 10,
        10, 11, 11, 11, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 15, 15,
        15, 16, 16, 16, 16, 17, 17, 17, 18, 18, 18, 19, 19, 19, 19, 20,
    };

    #define HE_POWERS_OF_10(factor)                                 \
        (factor)*10, (factor)*100, (factor)*1000, (factor)*10000,   \
        (factor)*100000, (factor)*1000000, (factor)*10000000,       \
        (factor)*100000000, (factor)*1000000000
    static constexpr const uint64_t ZeroOrPowersOf10[] =
    {
        0, 0, HE_POWERS_OF_10(1u), HE_POWERS_OF_10(1000000000ull), 10000000000000000000ull,
    };
    #undef HE_POWERS_OF_10

    // Converts value in the range [0, 100) to a string.
    constexpr const char* Digits2(uint64_t value)
    {
        return &"0001020304050607080910111213141516171819"
            "2021222324252627282930313233343536373839"
            "4041424344454647484950515253545556575859"
            "6061626364656667686970717273747576777879"
            "8081828384858687888990919293949596979899"[value * 2];
    };

    HE_FORCE_INLINE uint32_t CountDigits(uint64_t x)
    {
        const uint8_t t = Bsr2Log10[HE_CLZLL(x | 1) ^ 63];
        return t - (x < ZeroOrPowersOf10[t]);
    }

    HE_FORCE_INLINE uint32_t CountDigits(uint32_t x)
    {
        const uint64_t inc = DigitCountLookup[HE_CLZ(x | 1) ^ 31];
        return static_cast<uint32_t>((x + inc) >> 32);
    }

    template <uint32_t Bits, typename T>
    inline uint32_t CountDigits(T x)
    {
        if constexpr (Limits<T>::ValueBits == 32)
            return (HE_CLZ(static_cast<uint32_t>(x) | 1) ^ 31) / Bits + 1;

        uint32_t count = 0;
        do
        {
            ++count;
        } while ((x >>= Bits) != 0);
        return count;
    }

    // A public domain branchless UTF-8 decoder by Christopher Wellons:
    // https://github.com/skeeto/branchless-utf8
    /* Decode the next character, c, from s, reporting errors in e.
     *
     * Since this is a branchless decoder, four bytes will be read from the
     * buffer regardless of the actual length of the next character. This
     * means the buffer _must_ have at least three bytes of zero padding
     * following the end of the data stream.
     *
     * Errors are reported in e, which will be non-zero if the parsed
     * character was somehow invalid: invalid byte sequence, non-canonical
     * encoding, or a surrogate half.
     *
     * The function returns a pointer to the next character. When an error
     * occurs, this pointer will be a guess that depends on the particular
     * error, but it will always advance at least one byte.
     */
    static const char* Utf8Decode(const char* s, uint32_t* c, int* e)
    {
        constexpr const int Masks[] = { 0x00, 0x7f, 0x1f, 0x0f, 0x07 };
        constexpr const uint32_t Mins[] = { 4194304, 0, 128, 2048, 65536 };
        constexpr const int ShiftC[] = { 0, 18, 12, 6, 0 };
        constexpr const int ShiftE[] = { 0, 6, 4, 2, 0 };

        uint32_t len = FmtGetCodePointLength(*s);
        // Compute the pointer to the next character early so that the next
        // iteration can start working on the next character. Neither Clang
        // nor GCC figure out this reordering on their own.
        const char* next = s + len + !len;

        using uchar = unsigned char;

        // Assume a four-byte character and load four bytes. Unused bits are
        // shifted out.
        *c = uint32_t(uchar(s[0]) & Masks[len]) << 18;
        *c |= uint32_t(uchar(s[1]) & 0x3f) << 12;
        *c |= uint32_t(uchar(s[2]) & 0x3f) << 6;
        *c |= uint32_t(uchar(s[3]) & 0x3f) << 0;
        *c >>= ShiftC[len];

        // Accumulate the various error conditions.
        *e = (*c < Mins[len]) << 6;       // non-canonical encoding
        *e |= ((*c >> 11) == 0x1b) << 7;  // surrogate half?
        *e |= (*c > 0x10FFFF) << 8;       // out of range?
        *e |= (uchar(s[1]) & 0xc0) >> 2;
        *e |= (uchar(s[2]) & 0xc0) >> 4;
        *e |= uchar(s[3]) >> 6;
        *e ^= 0x2a;  // top two bits of each tail byte correct?
        *e >>= ShiftE[len];

        return next;
    }

    static constexpr uint32_t InvalidCodePoint = ~0u;

    template <typename F>
    static void ForEachCodePoint(StringView str, F&& itr)
    {
        const auto decode = [itr](const char* buf, const char* ptr)
        {
            uint32_t cp = 0;
            int32_t error = 0;
            const char* end = Utf8Decode(buf, &cp, &error);
            const bool result = itr(error ? InvalidCodePoint : cp, StringView{ ptr, error ? 1 : static_cast<uint32_t>(end - buf) });
            return result ? (error ? buf + 1 : end) : nullptr;
        };

        constexpr uint32_t BlockSize = 4;// utf8_decode always reads blocks of 4 chars.
        const char* p = str.Data();
        if (str.Size() >= BlockSize)
        {
            for (const char* end = p + str.Size() - BlockSize + 1; p < end;)
            {
                p = decode(p, p);
                if (!p)
                    return;
            }
        }

        if (uint32_t charsLeftCount = static_cast<uint32_t>(str.Data() + str.Size() - p))
        {
            char buf[2 * BlockSize - 1]{};
            MemCopy(buf, p, charsLeftCount);
            const char* bufPtr = buf;
            do
            {
                const char* end = decode(bufPtr, p);
                if (!end)
                    return;
                p += end - bufPtr;
                bufPtr = end;
            } while (static_cast<uint32_t>(bufPtr - buf) < charsLeftCount);
        }
    }

    static uint32_t GetWidth(StringView str)
    {
        uint32_t count = 0;
        ForEachCodePoint(str, [&](uint32_t cp, StringView) -> bool
        {
            count += static_cast<uint32_t>(
                1 +
                (cp >= 0x1100 &&
                    (cp <= 0x115f ||  // Hangul Jamo init. consonants
                    cp == 0x2329 ||  // LEFT-POINTING ANGLE BRACKET
                    cp == 0x232a ||  // RIGHT-POINTING ANGLE BRACKET
                    // CJK ... Yi except IDEOGRAPHIC HALF FILL SPACE:
                    (cp >= 0x2e80 && cp <= 0xa4cf && cp != 0x303f) ||
                    (cp >= 0xac00 && cp <= 0xd7a3) ||    // Hangul Syllables
                    (cp >= 0xf900 && cp <= 0xfaff) ||    // CJK Compatibility Ideographs
                    (cp >= 0xfe10 && cp <= 0xfe19) ||    // Vertical Forms
                    (cp >= 0xfe30 && cp <= 0xfe6f) ||    // CJK Compatibility Forms
                    (cp >= 0xff00 && cp <= 0xff60) ||    // Fullwidth Forms
                    (cp >= 0xffe0 && cp <= 0xffe6) ||    // Fullwidth Forms
                    (cp >= 0x20000 && cp <= 0x2fffd) ||  // CJK
                    (cp >= 0x30000 && cp <= 0x3fffd) ||
                    // Miscellaneous Symbols and Pictographs + Emoticons:
                    (cp >= 0x1f300 && cp <= 0x1f64f) ||
                    // Supplemental Symbols and Pictographs:
                    (cp >= 0x1f900 && cp <= 0x1f9ff))));
            return true;
        });
        return count;
    }

    static uint32_t GetCodePointIndex(StringView str, uint32_t len)
    {
        const char* data = str.Data();
        uint32_t count = 0;
        for (uint32_t i = 0, size = str.Size(); i != size; ++i)
        {
            if ((data[i] & 0xc0) != 0x80 && ++count > len)
                return i;
        }
        return str.Size();
    }

    static char* Fill(char* it, uint32_t len, const FmtSpecFill& fill)
    {
        const uint32_t size = fill.Size();
        if (size == 1)
        {
            MemSet(it, fill[0], len);
            return it + len;
        }

        const char* data = fill.Data();
        for (uint32_t i = 0; i < len; ++i)
        {
            MemCopy(it, data, size);
            it += size;
        }

        return it;
    }

    struct FormatDecimalResult
    {
        char* begin;
        char* end;
    };

    template <UnsignedIntegral T>
    static FormatDecimalResult FormatDecimal(char* it, T value, uint32_t len)
    {
        HE_FMT_ASSERT(len >= CountDigits(value), "Invalid digit count");

        it += len;
        char* end = it;
        while (value >= 100)
        {
            // Integer division is slow so do it for a group of two digits instead
            // of for every digit. The idea comes from the talk by Alexandrescu
            // "Three Optimization Tips for C++". See speed-test for a comparison.
            it -= 2;
            MemCopy(it, Digits2(value % 100), 2);
            value /= 100;
        }

        if (value < 10)
        {
            *--it = static_cast<char>('0' + value);
            return { it, end };
        }

        it -= 2;
        MemCopy(it, Digits2(value), 2);
        return { it, end };
    }

    template <uint32_t Bits, UnsignedIntegral T>
    static char* FormatUint(char* it, T value, uint32_t len, bool upper = false)
    {
        static constexpr char UpperDigits[] = "0123456789ABCDEF";
        static constexpr char LowerDigits[] = "0123456789abcdef";

        it += len;
        char* end = it;

        do
        {
            const char* digits = upper ? UpperDigits : LowerDigits;
            const uint32_t digit = static_cast<uint32_t>(value & ((1 << Bits) - 1));
            *--it = static_cast<char>(Bits < 4 ? static_cast<char>('0' + digit) : digits[digit]);
        } while ((value >>= Bits) != 0);

        return end;
    }

    template <SignedIntegral T> constexpr bool IsNegative(T value) { return value < 0; }
    template <UnsignedIntegral T> constexpr bool IsNegative(T) { return false; }

    inline void PrefixAppend(uint32_t& prefix, uint32_t value)
    {
        prefix |= prefix != 0 ? value << 8 : value;
        prefix += (1u + (value > 0xff ? 1 : 0)) << 24;
    }

    // Writes the output of `writer`, padded according to format specifications in `spec`.
    // size: output size in code units.
    // width: output display width in (terminal) column positions.
    template <FmtSpecAlign Align = FmtSpecAlign::Left, typename F>
    static void WritePadded(String& out, const FmtSpec& spec, uint32_t size, uint32_t width, F&& writer)
    {
        static_assert(Align == FmtSpecAlign::Left || Align == FmtSpecAlign::Right);

        static constexpr uint32_t LeftShifts[] = { 0x1f, 0x1f, 0x00, 0x01, 0x00 };
        static constexpr uint32_t RightShifts[] = { 0x00, 0x1f, 0x00, 0x01, 0x00 };
        static constexpr const uint32_t* Shifts = Align == FmtSpecAlign::Left ? LeftShifts : RightShifts;

        const uint32_t shiftIndex = static_cast<uint32_t>(spec.align);
        const uint32_t padding = spec.width > width ? spec.width - width : 0;
        const uint32_t leftPadding = padding >> Shifts[shiftIndex];
        const uint32_t rightPadding = padding - leftPadding;

        char* it = FmtResize(out, size + (padding * spec.fill.Size()));
        if (leftPadding != 0)
            it = Fill(it, leftPadding, spec.fill);

        it = writer(it);

        if (rightPadding != 0)
            it = Fill(it, rightPadding, spec.fill);
    }

    template <FmtSpecAlign Align = FmtSpecAlign::Left, typename F>
    static void WritePadded(String& out, const FmtSpec& spec, uint32_t size, F&& writer)
    {
        WritePadded<Align>(out, spec, size, size, writer);
    }

    static void WriteChar(String& out, char value, const FmtSpecInt& spec)
    {
        WritePadded(out, spec, 1, [=](char* it)
        {
            *it++ = value;
            return it;
        });
    }

    // Writes an integer in the format
    //   <left-padding><prefix><numeric-padding><digits><right-padding>
    // where <digits> are written by `digitWriter(it)`.
    // prefix contains chars in three lower bytes and the size in the fourth byte.
    template <typename F>
    static void WriteInt(String& out, uint32_t digitCount, uint32_t prefix, const FmtSpecInt& spec, F&& digitWriter)
    {
        if (spec.width == 0 && spec.precision == -1)
        {
            char* it = FmtResize(out, digitCount + (prefix >> 24));
            if (prefix)
            {
                for (uint32_t p = prefix & 0xffffff; p != 0; p >>= 8)
                {
                    *it++ = static_cast<char>(p & 0xff);
                }
            }
            digitWriter(it);
            return;
        }

        uint32_t len = digitCount + (prefix >> 24);
        uint32_t padding = 0;
        if (spec.align == FmtSpecAlign::Numeric)
        {
            if (spec.width > len)
            {
                padding = spec.width - len;
                len = spec.width;
            }
        }
        else if (spec.precision > static_cast<int32_t>(digitCount))
        {
            len = spec.precision + (prefix >> 24);
            padding = spec.precision - digitCount;
        }

        WritePadded<FmtSpecAlign::Right>(out, spec, len, [=](char* it)
        {
            for (uint32_t p = prefix & 0xffffff; p != 0; p >>= 8)
            {
                *it++ = static_cast<char>(p & 0xff);
            }
            MemSet(it, '0', padding);
            it += padding;
            return digitWriter(it);
        });
    }

    template <Integral T>
    static void WriteInt(String& out, T value, const FmtSpecInt& spec)
    {
        uint32_t prefix = 0;
        uint64_t absValue = static_cast<uint64_t>(value);
        if (IsNegative(value))
        {
            prefix = 0x01000000u | '-';
            absValue = 0 - absValue;
        }
        else
        {
            static constexpr uint32_t Prefixes[] = { 0, 0, 0x1000000u | '+', 0x1000000u | ' ' };
            prefix = Prefixes[AsUnderlyingType(spec.sign)];
        }

        switch (spec.type)
        {
            case FmtSpecIntType::None:
            case FmtSpecIntType::Decimal:
            {
                const uint32_t digitCount = CountDigits(absValue);
                return WriteInt(out, digitCount, prefix, spec, [=](char* it)
                {
                    return FormatDecimal(it, absValue, digitCount).end;
                });
            }
            case FmtSpecIntType::Char:
            {
                return WriteChar(out, static_cast<char>(absValue), spec);
            }
            case FmtSpecIntType::Octal:
            {
                const uint32_t digitCount = CountDigits<3>(absValue);
                // Octal prefix '0' is counted as a digit, so only add it if precision
                // is not greater than the number of digits.
                if (spec.alt && spec.precision <= static_cast<int32_t>(digitCount) && absValue != 0)
                    PrefixAppend(prefix, '0');

                return WriteInt(out, digitCount, prefix, spec, [=](char* it)
                {
                    return FormatUint<3>(it, absValue, digitCount);
                });
            }
            case FmtSpecIntType::HexLower:
            case FmtSpecIntType::HexUpper:
            {
                const bool upper = spec.type == FmtSpecIntType::HexUpper;
                if (spec.alt)
                    PrefixAppend(prefix, static_cast<uint32_t>(upper ? 'X' : 'x') << 8 | '0');
                const uint32_t digitCount = CountDigits<4>(absValue);
                return WriteInt(out, digitCount, prefix, spec, [=](char* it)
                {
                    return FormatUint<4>(it, absValue, digitCount, upper);
                });
            }
            case FmtSpecIntType::BinLower:
            case FmtSpecIntType::BinUpper:
            {
                const bool upper = spec.type == FmtSpecIntType::BinUpper;
                if (spec.alt)
                    PrefixAppend(prefix, static_cast<uint32_t>(upper ? 'B' : 'b') << 8 | '0');
                const uint32_t digitCount = CountDigits<1>(absValue);
                return WriteInt(out, digitCount, prefix, spec, [=](char* it)
                {
                    return FormatUint<1>(it, absValue, digitCount);
                });
            }
        }

        FmtError("Unknown integer format spec");
    }

    static bool ShouldUseExponentFormat(int32_t outputExp, const FmtSpecFloat& spec)
    {
        if (spec.type == FmtSpecFloatType::ExponentLower || spec.type == FmtSpecFloatType::ExponentUpper)
            return true;

        if (spec.type != FmtSpecFloatType::None && spec.type != FmtSpecFloatType::GeneralLower && spec.type != FmtSpecFloatType::GeneralUpper)
            return false;

        constexpr int32_t ExpLowerBound = -4;
        constexpr int32_t ExpUpperBound = 16;

        return outputExp < ExpLowerBound || outputExp >= (spec.precision > 0 ? spec.precision : ExpUpperBound);
    }

    static bool ShouldShowPoint(const FmtSpecFloat& spec)
    {
        if (spec.alt)
            return true;

        switch (spec.type)
        {
            case FmtSpecFloatType::FixedLower:
            case FmtSpecFloatType::FixedUpper:
            case FmtSpecFloatType::ExponentLower:
            case FmtSpecFloatType::ExponentUpper:
                return spec.precision != 0;

            default:
                return false;
        }
    }

    static bool IsUpper(const FmtSpecFloat& spec)
    {
        switch (spec.type)
        {
            case FmtSpecFloatType::FixedUpper:
            case FmtSpecFloatType::ExponentUpper:
            case FmtSpecFloatType::GeneralUpper:
            case FmtSpecFloatType::HexUpper:
                return true;

            default:
                return false;
        }
    }

    static char SignToChar(FmtSpecSign sign)
    {
        return "\0-+ "[static_cast<uint32_t>(sign)];
    }

    static char* WriteSignificand(char* it, const char* significand, uint32_t significandSize)
    {
        MemCopy(it, significand, significandSize);
        return it + significandSize;
    }

    template <UnsignedIntegral T>
    static char* WriteSignificand(char* it, T significand, uint32_t significandSize)
    {
        return FormatDecimal(it, significand, significandSize).end;
    }

    template <typename T>
    static char* WriteSignificand(char* it, T significand, uint32_t significandSize, int32_t exponent)
    {
        it = WriteSignificand(it, significand, significandSize);
        MemSet(it, '0', exponent);
        return it + exponent;
    }

    template <UnsignedIntegral T>
    static char* WriteSignificand(char* it, T significand, uint32_t significandSize, int32_t integralSize, char decimalPoint)
    {
        if (!decimalPoint)
            return FormatDecimal(it, significand, significandSize).end;

        it += significandSize + 1;
        char* end = it;
        const int32_t floatingSize = static_cast<int32_t>(significandSize) - integralSize;
        for (int32_t i = floatingSize / 2; i > 0; --i)
        {
            it -= 2;
            MemCopy(it, Digits2(significand % 100), 2);
            significand /= 100;
        }

        if (floatingSize % 2 != 0)
        {
            *--it = static_cast<char>('0' + (significand % 10));
            significand /= 10;
        }

        *--it = decimalPoint;
        FormatDecimal(it - integralSize, significand, integralSize);
        return end;
    }

    static char* WriteSignificand(char* it, const char* significand, uint32_t significandSize, int32_t integralSize, char decimalPoint)
    {
        MemCopy(it, significand, integralSize);
        it += integralSize;

        if (!decimalPoint)
            return it;

        *it++ = decimalPoint;
        const uint32_t len = significandSize - integralSize;
        MemCopy(it, significand + integralSize, len);
        return it + len;
    }

    static void WriteNonFinite(String& out, bool isnan, const FmtSpecFloat& spec)
    {
        constexpr uint32_t strSize = 3;
        const bool isUpper = IsUpper(spec);
        const char* str = isnan ? (isUpper ? "NAN" : "nan") : (isUpper ? "INF" : "inf");
        const uint32_t size = strSize + (spec.sign != FmtSpecSign::None ? 1 : 0);

        FmtSpecFloat writeSpec = spec;
        if (spec.fill.Size() == 1 && spec.fill[0] == '0')
            writeSpec.fill[0] = ' ';

        WritePadded(out, writeSpec, size, [=](char* it)
        {
            if (spec.sign != FmtSpecSign::None)
                *it++ = SignToChar(spec.sign);

            MemCopy(it, str, strSize);
            return it + strSize;
        });
    }

    static char* WriteExponent(char* it, int32_t exp)
    {
        HE_FMT_ASSERT(exp > -10000 && exp < 10000, "Exponent out of range");
        if (exp < 0)
        {
            *it++ = '-';
            exp = -exp;
        }
        else
        {
            *it++ = '+';
        }

        if (exp >= 100)
        {
            const char* top = Digits2(exp / 100);
            if (exp >= 1000)
                *it++ = top[0];
            *it++ = top[1];
            exp %= 100;
        }

        const char* d = Digits2(exp);
        *it++ = d[0];
        *it++ = d[1];
        return it;
    }

    static void WriteFloat_SNPrintf(String& out, double value, const FmtSpecFloat& spec)
    {
        if (spec.sign != FmtSpecSign::None)
            out.Append(SignToChar(spec.sign));

        char format[8];
        char* fp = format;
        *fp++ = '%';
        if (ShouldShowPoint(spec))
            *fp++ = '#';
        if (spec.precision >= 0)
        {
            *fp++ = '.';
            *fp++ = '*';
        }
        switch (spec.type)
        {
            case FmtSpecFloatType::None: *fp++ = 'g'; break;
            case FmtSpecFloatType::HexLower: *fp++ = 'a'; break;
            case FmtSpecFloatType::HexUpper: *fp++ = 'A'; break;
            case FmtSpecFloatType::ExponentLower: *fp++ = 'e'; break;
            case FmtSpecFloatType::ExponentUpper: *fp++ = 'E'; break;
            case FmtSpecFloatType::FixedLower: *fp++ = 'f'; break;
            case FmtSpecFloatType::FixedUpper: *fp++ = 'F'; break;
            case FmtSpecFloatType::GeneralLower: *fp++ = 'g'; break;
            case FmtSpecFloatType::GeneralUpper: *fp++ = 'G'; break;
        }
        *fp++ = '\0';

        const int requiredSize = spec.precision >= 0
            ? std::snprintf(nullptr, 0, format, spec.precision, value)
            : std::snprintf(nullptr, 0, format, value);

        HE_FMT_ASSERT(requiredSize > 0, "Required size must be greater than zero.");

        // The buffer sizes may look a bit weird here at first glance. However, this is correct.
        // We are using a property of String where any size you set it to will always have one
        // additional character for the null terminator.
        //
        // Because of that, and the fact that `snprintf` returns the number of characters it will
        // write excluding the null terminator, we resize only to the number of characters.
        // Technically, snprintf then writes beyond our resized size; but worst case that is into
        // the extra null character slot String allocated for us. We then return from the writer
        // function as if we had only written the non-null characters.
        WritePadded<FmtSpecAlign::Right>(out, spec, requiredSize, [=](char* it)
        {
            [[maybe_unused]] const int result = spec.precision >= 0
                ? std::snprintf(it, requiredSize + 1, format, spec.precision, value)
                : std::snprintf(it, requiredSize + 1, format, value);
            HE_FMT_ASSERT(result == requiredSize, "Formatting must succeed here.");
            return it + requiredSize;
        });
    }

    static void WriteFloat(String& out, StringView significandView, int32_t exponent, const FmtSpecFloat& spec)
    {
        const char* significand = significandView.Data();
        const uint32_t significandSize = significandView.Size();
        const int32_t outputExp = exponent + significandSize - 1;
        const bool showPoint = ShouldShowPoint(spec);

        char decimalPoint = '.';
        uint32_t size = significandSize + (spec.sign != FmtSpecSign::None ? 1 : 0);

        if (ShouldUseExponentFormat(outputExp, spec))
        {
            int32_t zeroesCount = 0;
            if (showPoint)
            {
                zeroesCount = spec.precision - significandSize;
                if (zeroesCount < 0)
                    zeroesCount = 0;
                size += zeroesCount;
            }
            else if (significandSize == 1)
            {
                decimalPoint = '\0';
            }

            const int32_t absOutputExp = outputExp >= 0 ? outputExp : -outputExp;
            int32_t expDigits = 2;
            if (absOutputExp >= 100)
                expDigits = absOutputExp >= 1000 ? 4 : 3;

            size += (decimalPoint ? 1 : 0) + 2 + expDigits;
            const char expChar = IsUpper(spec) ? 'E' : 'e';

            const auto write = [&](char* it)
            {
                if (spec.sign != FmtSpecSign::None)
                    *it++ = SignToChar(spec.sign);

                it = WriteSignificand(it, significand, significandSize, 1, decimalPoint);

                if (zeroesCount > 0)
                {
                    MemSet(it, '0', zeroesCount);
                    it += zeroesCount;
                }

                *it++ = expChar;
                return WriteExponent(it, outputExp);
            };

            if (spec.width > 0)
            {
                WritePadded<FmtSpecAlign::Right>(out, spec, size, write);
            }
            else
            {
                write(FmtResize(out, size));
            }
            return;
        }

        const int32_t exp = exponent + significandSize;
        if (exponent >= 0)
        {
            size += exponent;
            int32_t zeroesCount = spec.precision - exp;
            // TODO: fuzz, check if zeroesCount > 5000
            if (showPoint)
            {
                ++size;
                if (zeroesCount <= 0 && spec.type != FmtSpecFloatType::FixedLower && spec.type != FmtSpecFloatType::FixedUpper)
                    zeroesCount = 1;
                if (zeroesCount > 0)
                    size += zeroesCount;
            }

            // TODO: grouping

            WritePadded<FmtSpecAlign::Right>(out, spec, size, [&](char* it)
            {
                if (spec.sign != FmtSpecSign::None)
                    *it++ = SignToChar(spec.sign);

                it = WriteSignificand(it, significand, significandSize, exponent);
                if (!showPoint)
                    return it;

                *it++ = decimalPoint;

                if (zeroesCount > 0)
                {
                    MemSet(it, '0', zeroesCount);
                    it += zeroesCount;
                }

                return it;
            });
            return;
        }
        else if (exp > 0)
        {
            const int32_t zeroesCount = showPoint ? spec.precision - significandSize : 0;
            size += 1 + (zeroesCount > 0 ? zeroesCount : 0);
            // TODO: grouping
            WritePadded<FmtSpecAlign::Right>(out, spec, size, [&](char* it)
            {
                if (spec.sign != FmtSpecSign::None)
                    *it++ = SignToChar(spec.sign);

                it = WriteSignificand(it, significand, significandSize, exp, decimalPoint);

                if (zeroesCount > 0)
                {
                    MemSet(it, '0', zeroesCount);
                    it += zeroesCount;
                }

                return it;
            });
            return;
        }

        int32_t zeroesCount = -exp;
        if (significandSize == 0 && spec.precision >= 0 && spec.precision < zeroesCount)
            zeroesCount = spec.precision;

        const bool pointy = zeroesCount != 0 || significandSize != 0 || showPoint;
        size += 1 + (pointy ? 1 : 0) + zeroesCount;
        WritePadded<FmtSpecAlign::Right>(out, spec, size, [&](char* it)
        {
            if (spec.sign != FmtSpecSign::None)
                *it++ = SignToChar(spec.sign);

            *it++ = '0';
            if (!pointy)
                return it;

            *it++ = decimalPoint;

            if (zeroesCount > 0)
            {
                MemSet(it, '0', zeroesCount);
                it += zeroesCount;
            }

            return WriteSignificand(it, significand, significandSize);
        });
    }

    static int32_t FormatFloat_Dragonbox(String& out, double value, int32_t precision, bool isFloat, const FmtSpecFloat& spec)
    {
        if (value <= 0)
        {
            if (precision <= 0 || (spec.type != FmtSpecFloatType::FixedLower && spec.type != FmtSpecFloatType::FixedUpper))
            {
                out.Append('0');
                return 0;
            }

            char* it = FmtResize(out, precision);
            MemSet(it, '0', precision);
            return -precision;
        }

        if (isFloat)
        {
            const auto dec = jkj::dragonbox::to_decimal(static_cast<float>(value));
            WriteInt(out, dec.significand, {});
            return dec.exponent;
        }

        const auto dec = jkj::dragonbox::to_decimal(value);
        WriteInt(out, dec.significand, {});
        return dec.exponent;
    }

    static void WriteFloat(String& out, double value, bool isFloat, const FmtSpecFloat& spec_)
    {
        FmtSpecFloat spec = spec_;
        if (std::signbit(value))
        {
            spec.sign = FmtSpecSign::Minus;
            value = -value;
        }
        else if (spec.sign == FmtSpecSign::Minus)
        {
            spec.sign = FmtSpecSign::None;
        }

        if (!std::isfinite(value))
            return WriteNonFinite(out, std::isnan(value), spec);

        if (spec.align == FmtSpecAlign::Numeric && spec.sign != FmtSpecSign::None)
        {
            out.Append(SignToChar(spec.sign));
            spec.sign = FmtSpecSign::None;
            if (spec.width != 0)
                --spec.width;
        }

        if (spec.type == FmtSpecFloatType::HexLower || spec.type == FmtSpecFloatType::HexUpper)
        {
            WriteFloat_SNPrintf(out, value, spec);
            return;
        }

        int32_t precision = spec.precision >= 0 || spec.type == FmtSpecFloatType::None ? spec.precision : 6;

        if (spec.type == FmtSpecFloatType::ExponentLower || spec.type == FmtSpecFloatType::ExponentUpper)
        {
            if (precision == Limits<int32_t>::Max)
                FmtError("Float precision is too large.");
            else
                ++precision;
        }
        else if (precision == 0 && spec.type != FmtSpecFloatType::FixedLower && spec.type != FmtSpecFloatType::FixedUpper)
        {
            precision = 1;
        }

        // We have precision requirements, fall back to snprintf
        if (precision >= 0)
        {
            WriteFloat_SNPrintf(out, value, spec);
            return;
        }

        // Very good chance this won't allocate. Almost all floats write out in < 30 characters.
        String buffer;
        const int32_t exponent = FormatFloat_Dragonbox(buffer, value, precision, isFloat, spec);
        spec.precision = precision;
        WriteFloat(out, buffer, exponent, spec);
    }

    [[noreturn]] void FmtError(const char* msg)
    {
        std::fprintf(stderr, "Fmt error: %s", msg);
        std::terminate();
    }

    void Formatter<bool>::Format(String& out, bool value) const
    {
        if (spec.type != FmtSpecIntType::None)
        {
            WriteInt(out, value ? 1 : 0, spec);
        }
        else
        {
            const StringView str = value ? "true" : "false";
            WritePadded<FmtSpecAlign::Left>(out, spec, str.Size(), [&](char* it)
            {
                MemCopy(it, str.Data(), str.Size());
                return it + str.Size();
            });
        }
    }

    void Formatter<char>::Format(String& out, char value) const
    {
        if (spec.type == FmtSpecIntType::None || spec.type == FmtSpecIntType::Char)
        {
            WriteChar(out, value, spec);
        }
        else
        {
            WriteInt(out, static_cast<int32_t>(value), spec);
        }
    }

    void Formatter<wchar_t>::Format(String& out, wchar_t value) const
    {
        if (spec.type == FmtSpecIntType::None || spec.type == FmtSpecIntType::Char)
        {
            wchar_t wstr[]{ value, L'\0' };
            char str[4]{};
            const uint32_t len = WCToMBStr(str, 4, wstr);

            WritePadded(out, spec, len, [=](char* it)
            {
                for (uint32_t i = 0; i < len; ++i)
                {
                    *it++ = str[i];
                }
                return it;
            });
        }
        else if constexpr (IsSigned<wchar_t>)
        {
            WriteInt(out, static_cast<int32_t>(value), spec);
        }
        else
        {
            WriteInt(out, static_cast<uint32_t>(value), spec);
        }
    }

    void Formatter<char8_t>::Format(String& out, char8_t value) const
    {
        WriteInt(out, static_cast<uint32_t>(value), spec);
    }

    void Formatter<char16_t>::Format(String& out, char16_t value) const
    {
        WriteInt(out, static_cast<uint32_t>(value), spec);
    }

    void Formatter<char32_t>::Format(String& out, char32_t value) const
    {
        WriteInt(out, static_cast<uint32_t>(value), spec);
    }

    void Formatter<signed char>::Format(String& out, signed char value) const
    {
        WriteInt(out, static_cast<int32_t>(value), spec);
    }

    void Formatter<short>::Format(String& out, short value) const
    {
        WriteInt(out, static_cast<int32_t>(value), spec);
    }

    void Formatter<int>::Format(String& out, int value) const
    {
        WriteInt(out, static_cast<int32_t>(value), spec);
    }

    void Formatter<long>::Format(String& out, long value) const
    {
        if constexpr (sizeof(long) == 4)
            WriteInt(out, static_cast<int32_t>(value), spec);
        else
            WriteInt(out, static_cast<int64_t>(value), spec);
    }

    void Formatter<long long>::Format(String& out, long long value) const
    {
        WriteInt(out, static_cast<int64_t>(value), spec);
    }

    void Formatter<unsigned char>::Format(String& out, unsigned char value) const
    {
        WriteInt(out, static_cast<uint32_t>(value), spec);
    }

    void Formatter<unsigned short>::Format(String& out, unsigned short value) const
    {
        WriteInt(out, static_cast<uint32_t>(value), spec);
    }

    void Formatter<unsigned int>::Format(String& out, unsigned int value) const
    {
        WriteInt(out, static_cast<uint32_t>(value), spec);
    }

    void Formatter<unsigned long>::Format(String& out, unsigned long value) const
    {
        if constexpr (sizeof(long) == 4)
            WriteInt(out, static_cast<uint32_t>(value), spec);
        else
            WriteInt(out, static_cast<uint64_t>(value), spec);
    }

    void Formatter<unsigned long long>::Format(String& out, unsigned long long value) const
    {
        WriteInt(out, static_cast<uint64_t>(value), spec);
    }

    void Formatter<float>::Format(String& out, float value) const
    {
        WriteFloat(out, value, true, spec);
    }

    void Formatter<double>::Format(String& out, double value) const
    {
        WriteFloat(out, value, false, spec);
    }

    void Formatter<const void*>::Format(String& out, const void* value) const
    {
        const uintptr_t uvalue = BitCast<uintptr_t>(value);
        const uint32_t digitCount = CountDigits<4>(uvalue);
        const uint32_t size = digitCount + 2;

        WritePadded<FmtSpecAlign::Right>(out, spec, size, [=](char* it)
        {
            *it++ = '0';
            *it++ = 'x';
            return FormatUint<4>(it, uvalue, digitCount);
        });
    }

    void Formatter<StringView>::Format(String& out, const StringView& value) const
    {
        const char* data = value.Data();
        uint32_t size = value.Size();

        if (spec.precision >= 0 && static_cast<uint32_t>(spec.precision) < size)
            size = GetCodePointIndex(value, static_cast<uint32_t>(spec.precision));

        uint32_t width = 0;
        if (spec.width != 0)
        {
            width = GetWidth({ data, size });
        }

        WritePadded(out, spec, size, width, [=](char* it)
        {
            MemCopy(it, data, size);
            return it + size;
        });
    }

    void Formatter<const char*>::Format(String& out, const char* value) const
    {
        if (HE_VERIFY(value, HE_MSG("Null pointer passed to string formatter")))
        {
            Formatter<StringView> f;
            f.spec = spec;
            f.Format(out, { value, StrLen(value) });
        }
    }

    void VFormatTo(String& out, StringView fmt, FmtArgs args, Pfn_FmtErrorHandler errorHandler)
    {
        // Special handling for the common case of formatting a single argument.
        // In this case we skip the visitor and just handle the one arg directly.
        if (fmt.Size() == 2 && fmt[0] == '{' && fmt[1] == '}')
        {
            if (args.count == 0)
            {
                errorHandler("Argument not found");
                return;
            }

            FmtParseCtx ctx({}, 0, 0, errorHandler);
            const FmtArg& arg = args.args[0];
            arg.format(ctx, out, arg.value);
            return;
        }

        FmtWriteVisitor visitor(out, fmt, args, errorHandler);
        FmtVisitString(fmt, visitor);
    }
}
