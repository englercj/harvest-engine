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

#include "fmt_dragonbox.h"
#include "fmt_private.h"

#include "he/core/assert.h"
#include "he/core/compiler.h"
#include "he/core/concepts.h"
#include "he/core/config.h"
#include "he/core/cpu.h"
#include "he/core/debugger.h"
#include "he/core/inline_allocator.h"
#include "he/core/limits.h"
#include "he/core/process.h"
#include "he/core/string.h"
#include "he/core/string_ops.h"
#include "he/core/types.h"
#include "he/core/utils.h"
#include "he/core/vector.h"
#include "he/core/wstr.h"

#include <cmath>
#include <cstdio>

namespace he
{
    class BigInt
    {
    public:
        // A BigInt is stored as an array of Bigits (big digits), with Bigit at index
        // 0 being the least significant one.
        using Bigit = uint32_t;
        using DoubleBigit = uint64_t;

        static constexpr uint32_t BigitsCapacity = 32;

    public:
        BigInt()
            : m_allocator(Allocator::GetDefault())
            , m_bigits(m_allocator)
            , m_exp(0)
        {
            m_bigits.Resize(BigitsCapacity);
        }

        explicit BigInt(uint64_t n) : BigInt() { Assign(n); }

        BigInt(const BigInt&) = delete;
        void operator=(const BigInt&) = delete;

        void Assign(const BigInt& other)
        {
            m_bigits = other.m_bigits;
            m_exp = other.m_exp;
        }

        template <typename Int> void operator=(Int n)
        {
            HE_FMT_ASSERT(n > 0, "");
            using UintType = Conditional<Limits<Int>::Bits <= 64, uint64_t, uint128_t>;
            Assign(static_cast<UintType>(n));
        }

        int32_t BigitsCount() const
        {
            return static_cast<int32_t>(m_bigits.Size()) + m_exp;
        }

        HE_NO_INLINE constexpr BigInt& operator<<=(int shift)
        {
            HE_FMT_ASSERT(shift >= 0, "");
            m_exp += shift / BigitBits;
            shift %= BigitBits;

            if (shift == 0)
                return *this;

            Bigit carry = 0;
            for (uint32_t i = 0, n = m_bigits.Size(); i < n; ++i)
            {
                Bigit c = m_bigits[i] >> (BigitBits - shift);
                m_bigits[i] = (m_bigits[i] << shift) + carry;
                carry = c;
            }

            if (carry != 0)
                m_bigits.PushBack(carry);

            return *this;
        }

        template <typename Int>
        constexpr BigInt& operator*=(Int value)
        {
            HE_FMT_ASSERT(value > 0, "");
            constexpr uint32_t Bits = Limits<Int>::Bits;
            using UintType = Conditional<Bits <= 32, uint32_t, Conditional<Bits <= 64, uint64_t, uint128_t>>;
            Multiply(static_cast<UintType>(value));
            return *this;
        }

        friend int32_t Compare(const BigInt& lhs, const BigInt& rhs)
        {
            const int32_t lhsCount = lhs.BigitsCount();
            const int32_t rhsCount = rhs.BigitsCount();

            if (lhsCount != rhsCount)
                return lhsCount > rhsCount ? 1 : -1;

            int32_t i = static_cast<int32_t>(lhs.m_bigits.Size()) - 1;
            int32_t j = static_cast<int32_t>(rhs.m_bigits.Size()) - 1;
            int32_t end = i - j;

            if (end < 0)
                end = 0;

            for (; i >= end; --i, --j)
            {
                const Bigit lhsBigit = lhs[i];
                const Bigit rhsBigit = rhs[j];
                if (lhsBigit != rhsBigit)
                    return lhsBigit > rhsBigit ? 1 : -1;
            }

            if (i != j)
                return i > j ? 1 : -1;

            return 0;
        }

        // Returns compare(lhs1 + lhs2, rhs).
        friend int32_t AddCompare(const BigInt& lhs1, const BigInt& lhs2, const BigInt& rhs)
        {
            const int32_t maxLhsBigits = Max(lhs1.BigitsCount(), lhs2.BigitsCount());
            const int32_t rhsBigitsCount = rhs.BigitsCount();

            if (maxLhsBigits + 1 < rhsBigitsCount)
                return -1;

            if (maxLhsBigits > rhsBigitsCount)
                return 1;

            const auto getBigit = [](const BigInt& n, int i) -> Bigit
            {
                return i >= n.m_exp && i < n.BigitsCount() ? n[i - n.m_exp] : 0;
            };

            DoubleBigit borrow = 0;
            const int32_t min_exp = Min(Min(lhs1.m_exp, lhs2.m_exp), rhs.m_exp);
            for (int i = rhsBigitsCount - 1; i >= min_exp; --i)
            {
                const DoubleBigit sum = static_cast<DoubleBigit>(getBigit(lhs1, i)) + getBigit(lhs2, i);
                const Bigit rhsBigit = getBigit(rhs, i);

                if (sum > rhsBigit + borrow)
                    return 1;

                borrow = rhsBigit + borrow - sum;

                if (borrow > 1)
                    return -1;

                borrow <<= BigitBits;
            }
            return borrow != 0 ? -1 : 0;
        }

        // Assigns pow(10, exp) to this BigInt.
        void AssignPow10(int32_t exp)
        {
            HE_FMT_ASSERT(exp >= 0, "");

            if (exp == 0)
            {
                *this = 1;
                return;
            }

            // Find the top bit.
            int bitmask = 1;
            while (exp >= bitmask)
            {
                bitmask <<= 1;
            }

            bitmask >>= 1;
            // pow(10, exp) = pow(5, exp) * pow(2, exp). First compute pow(5, exp) by
            // repeated squaring and multiplication.
            *this = 5;
            bitmask >>= 1;

            while (bitmask != 0)
            {
                Square();
                if ((exp & bitmask) != 0)
                    *this *= 5;
                bitmask >>= 1;
            }

            *this <<= exp;  // Multiply by pow(2, exp) by shifting.
        }

        void Square()
        {
            const int32_t bigitsCount = static_cast<int32_t>(m_bigits.Size());
            const int32_t resultBigitsCount = 2 * bigitsCount;

            InlineAllocator<sizeof(Bigit) * BigitsCapacity> mem(Allocator::GetDefault());
            Vector<Bigit> n(Move(m_bigits), mem);

            m_bigits.Resize(static_cast<uint32_t>(resultBigitsCount));

            uint128_t sum{ 0 };
            for (int32_t index = 0; index < bigitsCount; ++index)
            {
                // Compute Bigit at position index of the result by adding
                // cross-product terms n[i] * n[j] such that i + j == index.
                for (int i = 0, j = index; j >= 0; ++i, --j)
                {
                    // Most terms are multiplied twice which can be optimized in the future.
                    sum += static_cast<DoubleBigit>(n[i]) * n[j];
                }

                (*this)[index] = static_cast<Bigit>(sum);
                sum >>= Limits<Bigit>::Bits;  // Compute the carry.
            }

            // Do the same for the top half.
            for (int32_t index = bigitsCount; index < resultBigitsCount; ++index)
            {
                for (int32_t j = bigitsCount - 1, i = index - j; i < bigitsCount;)
                {
                    sum += static_cast<DoubleBigit>(n[i++]) * n[j--];
                }

                (*this)[index] = static_cast<Bigit>(sum);
                sum >>= Limits<Bigit>::Bits;
            }

            RemoveLeadingZeroes();
            m_exp *= 2;
        }

        // If this BigInt has a bigger exponent than other, adds trailing zero to make
        // exponents equal. This simplifies some operations such as subtraction.
        void Align(const BigInt& other)
        {
            const int32_t expDiff = m_exp - other.m_exp;
            if (expDiff <= 0)
                return;

            const int32_t BigitsCount = static_cast<int32_t>(m_bigits.Size());
            m_bigits.Resize(static_cast<uint32_t>(BigitsCount + expDiff));

            for (int32_t i = BigitsCount - 1, j = i + expDiff; i >= 0; --i, --j)
            {
                m_bigits[j] = m_bigits[i];
            }

            MemZero(m_bigits.Data(), static_cast<uint32_t>(expDiff) * sizeof(Bigit));
            m_exp -= expDiff;
        }

        // Divides this bignum by divisor, assigning the remainder to this and
        // returning the quotient.
        int32_t DivModAssign(const BigInt& divisor)
        {
            HE_FMT_ASSERT(this != &divisor, "");

            if (Compare(*this, divisor) < 0)
                return 0;

            HE_FMT_ASSERT(divisor.m_bigits[divisor.m_bigits.Size() - 1] != 0, "");

            Align(divisor);
            int32_t quotient = 0;
            do {
                SubtractAligned(divisor);
                ++quotient;
            } while (Compare(*this, divisor) >= 0);

            return quotient;
        }

    private:
        Bigit operator[](int index) const { return m_bigits[index]; }
        Bigit& operator[](int index) { return m_bigits[static_cast<uint32_t>(index)]; }

        static constexpr const int BigitBits = Limits<Bigit>::Bits;

        void SubtractBigits(int index, Bigit other, Bigit& borrow)
        {
            auto result = static_cast<DoubleBigit>((*this)[index]) - other - borrow;
            (*this)[index] = static_cast<Bigit>(result);
            borrow = static_cast<Bigit>(result >> (BigitBits * 2 - 1));
        }

        void RemoveLeadingZeroes()
        {
            int BigitsCount = static_cast<int>(m_bigits.Size()) - 1;
            while (BigitsCount > 0 && (*this)[BigitsCount] == 0)
            {
                --BigitsCount;
            }
            m_bigits.Resize(static_cast<uint32_t>(BigitsCount + 1));
        }

        // Computes *this -= other assuming aligned BigInts and *this >= other.
        void SubtractAligned(const BigInt& other)
        {
            HE_FMT_ASSERT(other.m_exp >= m_exp, "unaligned BigInts");
            HE_FMT_ASSERT(Compare(*this, other) >= 0, "");
            Bigit borrow = 0;
            int i = other.m_exp - m_exp;
            for (uint32_t j = 0, n = other.m_bigits.Size(); j != n; ++i, ++j)
            {
                SubtractBigits(i, other.m_bigits[j], borrow);
            }
            while (borrow > 0)
            {
                SubtractBigits(i, 0, borrow);
            }
            RemoveLeadingZeroes();
        }

        void Multiply(uint32_t value)
        {
            const DoubleBigit wide_value = value;
            Bigit carry = 0;
            for (uint32_t i = 0, n = m_bigits.Size(); i < n; ++i)
            {
                const DoubleBigit result = m_bigits[i] * wide_value + carry;
                m_bigits[i] = static_cast<Bigit>(result);
                carry = static_cast<Bigit>(result >> BigitBits);
            }
            if (carry != 0)
            {
                m_bigits.PushBack(carry);
            }
        }

        template <AnyOf<uint64_t, uint128_t> UInt>
        void Multiply(UInt value)
        {
            using HalfUint = Conditional<IsSame<UInt, uint128_t>, uint64_t, uint32_t>;
            const int shift = Limits<HalfUint>::Bits - BigitBits;
            const UInt lower = static_cast<HalfUint>(value);
            const UInt upper = value >> Limits<HalfUint>::Bits;
            UInt carry = 0;
            for (uint32_t i = 0, n = m_bigits.Size(); i < n; ++i)
            {
                const UInt result = lower * m_bigits[i] + static_cast<Bigit>(carry);
                carry = (upper * m_bigits[i] << shift) + (result >> BigitBits) + (carry >> BigitBits);
                m_bigits[i] = static_cast<Bigit>(result);
            }
            while (carry != 0)
            {
                m_bigits.PushBack(static_cast<Bigit>(carry));
                carry >>= BigitBits;
            }
        }

        template <AnyOf<uint64_t, uint128_t> UInt>
        void Assign(UInt n)
        {
            uint32_t BigitsCount = 0;
            do
            {
                m_bigits[BigitsCount++] = static_cast<Bigit>(n);
                n >>= BigitBits;
            } while (n != 0);
            m_bigits.Resize(BigitsCount);
            m_exp = 0;
        }

    private:
        InlineAllocator<sizeof(Bigit) * BigitsCapacity> m_allocator;
        Vector<Bigit> m_bigits;
        int32_t m_exp;
    };

    template <>
    struct Limits<uint128_t>
    {
        using Type = uint128_t;

        static constexpr bool IsSigned = false;
        static constexpr uint32_t Bits = 128;
        static constexpr uint32_t SignBits = IsSigned ? 1 : 0;
        static constexpr uint32_t ValueBits = static_cast<uint32_t>(Bits - SignBits);

        static constexpr uint32_t Digits = ValueBits;
        static constexpr uint32_t Digits10 = static_cast<uint32_t>(Digits * 3 / 10);

        static constexpr uint128_t Min{ 0 };
    #if HE_HAS_INT128
        static constexpr uint128_t Max{ ~uint128_t(0) };
    #else
        static constexpr uint128_t Max{ 0xffffffffffffffffull, 0xffffffffffffffffull };
    #endif
    };

    template <>
    struct Limits<BigInt>
    {
        using Type = BigInt;

        static constexpr bool IsSigned = false;
        static constexpr uint32_t Bits = 128;
        static constexpr uint32_t SignBits = IsSigned ? 1 : 0;
        static constexpr uint32_t ValueBits = static_cast<uint32_t>(Bits - SignBits);

        static constexpr uint32_t Digits = ValueBits;
        static constexpr uint32_t Digits10 = static_cast<uint32_t>(Digits * 3 / 10);
    };

    // A floating-point number f * pow(2, e) where T is an unsigned type.
    template <typename T>
    struct BasicFP
    {
        T f;
        int32_t e;

        constexpr BasicFP() : f(0), e(0) {}
        constexpr BasicFP(uint64_t value, int32_t exp) : f(value), e(exp) {}

        template <FloatingPoint F>
        constexpr BasicFP(F n) { Assign(n); }

        template <FloatingPoint F> requires(Limits<F>::Format != FloatFormat::DoubleDouble128)
        constexpr bool Assign(F value)
        {
            // This function assumes the float is in the format [sign][exponent][significand].
            using Uint = CarrierUint<F>;
            using Info = Limits<F>;

            constexpr Uint ImplicitBit = Uint(1) << Info::SignificandBits;
            constexpr Uint SignificandMask = ImplicitBit - 1;
            constexpr Uint ExponentMask = ((Uint(1) << Info::ExponentBits) - 1) << Info::SignificandBits;
            constexpr Uint ExponentBias = Info::MaxExponent - 1;

            const Uint bits = BitCast<Uint>(value);
            f = static_cast<T>(bits & SignificandMask);

            int32_t biasedExp = static_cast<int32_t>((bits & ExponentMask) >> Info::SignificandBits);

            // The predecessor is closer if n is a normalized power of 2 (f == 0)
            // other than the smallest normalized number (biased_e > 1).
            const bool isPredecessorCloser = f == 0 && biasedExp > 1;

            if (biasedExp == 0)
                biasedExp = 1; // subnormals use biased exponent 1 (min exponent)
            else if constexpr (Info::HasImplicitBit)
                f += static_cast<T>(ImplicitBit);

            e = biasedExp - ExponentBias - Info::SignificandBits;

            if constexpr (!Info::HasImplicitBit)
                ++e;

            return isPredecessorCloser;
        }

        template <FloatingPoint F> requires(Limits<F>::Format == FloatFormat::DoubleDouble128)
        constexpr bool Assign(F value)
        {
            static_assert(Limits<T>::Format != FloatFormat::DoubleDouble128, "Unsupported floating-point type");
            return Assign(static_cast<double>(value));
        }
    };

    constexpr void AdjustPrecision(int32_t& precision, int32_t exp10)
    {
        // Adjust fixed precision by exponent because it is relative to decimal point.
        HE_FMT_ASSERT(exp10 < 0 || precision < (Limits<int32_t>::Max - exp10), "Number is too big to be formatted.");
        precision += exp10;
    }

    constexpr uint32_t FractionPartRoundingThresholds(int32_t index)
    {
        // For checking rounding thresholds.
        // The kth entry is chosen to be the smallest integer such that the upper 32-bits of
        // 10^(k+1) times it is strictly bigger than 5 * 10^k.
        // It is equal to ceil(2^31 + 2^32/10^(k + 1)).
        // These are stored in a string literal because we cannot have static arrays
        // in constexpr functions and non-static ones are poorly optimized.
        return U"\x9999999a\x828f5c29\x80418938\x80068db9\x8000a7c6\x800010c7\x800001ae\x8000002b"[index];
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
        WritePadded(out, spec, 1, [&](char* it)
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

        WritePadded<FmtSpecAlign::Right>(out, spec, len, [&](char* it)
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
            prefix = Prefixes[EnumToValue(spec.sign)];
        }

        switch (spec.type)
        {
            case FmtSpecIntType::Default:
            case FmtSpecIntType::Decimal:
            {
                const uint32_t digitCount = CountDigits(absValue);
                return WriteInt(out, digitCount, prefix, spec, [&](char* it)
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

                return WriteInt(out, digitCount, prefix, spec, [&](char* it)
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
                return WriteInt(out, digitCount, prefix, spec, [&](char* it)
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
                return WriteInt(out, digitCount, prefix, spec, [&](char* it)
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

        if (spec.type != FmtSpecFloatType::Default
            && spec.type != FmtSpecFloatType::DefaultLower
            && spec.type != FmtSpecFloatType::DefaultUpper
            && spec.type != FmtSpecFloatType::GeneralLower
            && spec.type != FmtSpecFloatType::GeneralUpper)
        {
            return false;
        }

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
            case FmtSpecFloatType::DefaultUpper:
            case FmtSpecFloatType::FixedUpper:
            case FmtSpecFloatType::ExponentUpper:
            case FmtSpecFloatType::GeneralUpper:
            case FmtSpecFloatType::HexUpper:
                return true;

            default:
                return false;
        }
    }

    static bool IsDefault(const FmtSpecFloat& spec)
    {
        switch (spec.type)
        {
            case FmtSpecFloatType::Default:
            case FmtSpecFloatType::DefaultLower:
            case FmtSpecFloatType::DefaultUpper:
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
    static char* WriteSignificand(char* it, T significand, uint32_t significandSize, uint32_t integralSize, char decimalPoint)
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

    static char* WriteSignificand(char* it, const char* significand, uint32_t significandSize, uint32_t integralSize, char decimalPoint)
    {
        MemCopy(it, significand, integralSize);
        it += integralSize;

        if (!decimalPoint)
            return it;

        *it++ = decimalPoint;

        if (significandSize > integralSize)
        {
            const uint32_t len = significandSize - integralSize;
            MemCopy(it, significand + integralSize, len);
            return it + len;
        }

        return it;
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

        WritePadded(out, writeSpec, size, [&](char* it)
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

    template <typename T> requires(Limits<T>::Format != FloatFormat::DoubleDouble128)
    static void WriteFloat_Hex(String& out, T value, const FmtSpecFloat& spec)
    {
        using Info = Limits<T>;
        using Uint = CarrierUint<T>;

        HE_FMT_ASSERT(std::isfinite(value), "WriteFloat_Hex got a non-finite value.");

        // TODO: Audit now that I support long double

        BasicFP<Uint> fp(value);
        fp.e += Info::SignificandBits;

        if constexpr (!Info::HasImplicitBit)
            --fp.e;

        constexpr Uint FractionBits = Info::SignificandBits + static_cast<Uint>(Info::HasImplicitBit);
        constexpr Uint DigitsCount = (FractionBits + 3) / 4;

        constexpr Uint LeadingShift = ((DigitsCount - 1) * 4);
        constexpr Uint LeadingMask = Uint(0xf) << LeadingShift;
        const uint32_t leadingDigit = static_cast<uint32_t>((fp.f & LeadingMask) >> LeadingShift);

        if (leadingDigit > 1)
            fp.e -= (32 - CountLeadingZeros(leadingDigit) - 1);

        int32_t printCount = DigitsCount - 1;
        if (spec.precision >= 0 && printCount > spec.precision)
        {
            const int32_t shift = ((printCount - spec.precision - 1) * 4);
            const Uint mask = Uint(0xf) << shift;
            const uint32_t v = static_cast<uint32_t>((fp.f & mask) >> shift);

            if (v >= 8)
            {
                const Uint inc = Uint(1) << (shift + 4);
                fp.f += inc;
                fp.f &= ~(inc - 1);
            }

            // Check for long double overflow
            if constexpr (!Info::HasImplicitBit)
            {
                constexpr Uint ImplicitBit = Uint(1) << Info::SignificandBits;
                if ((fp.f & ImplicitBit) == ImplicitBit)
                {
                    fp.f >>= 4;
                    fp.e += 4;
                }
            }

            printCount = spec.precision;
        }

        char digits[Limits<Uint>::Bits / 4];
        MemSet(digits, '0', sizeof(digits));
        FormatUint<4>(digits, fp.f, DigitsCount, IsUpper(spec));
        static_assert(DigitsCount <= sizeof(digits));

        while (printCount > 0 && digits[printCount] == '0')
            --printCount;

        const bool isUpper = IsUpper(spec);
        const bool showPoint = spec.alt || printCount > 0 || printCount < spec.precision;
        const uint32_t absExp = static_cast<uint32_t>(fp.e < 0 ? -fp.e : fp.e);
        const uint32_t absExpLen = CountDigits(absExp);
        const uint32_t pointSize = (showPoint ? 1 : 0);
        const uint32_t extraPrecision = printCount < spec.precision ? (spec.precision - printCount) : 0;
        const uint32_t signSize = spec.sign != FmtSpecSign::None ? 1 : 0;
        const uint32_t size = 5 + pointSize + printCount + extraPrecision + absExpLen + signSize; // 5 = "0xFP+"

        WritePadded<FmtSpecAlign::Right>(out, spec, size, [&](char* it)
        {
            if (spec.sign != FmtSpecSign::None)
                *it++ = SignToChar(spec.sign);

            *it++ = '0';
            *it++ = isUpper ? 'X' : 'x';
            *it++ = digits[0];
            if (showPoint)
                *it++ = '.';

            MemCopy(it, digits + 1, printCount);
            it += printCount;

            for (; printCount < spec.precision; ++printCount)
                *it++ = '0';

            *it++ = isUpper ? 'P' : 'p';
            *it++ = fp.e < 0 ? '-' : '+';
            return FormatDecimal(it, absExp, CountDigits(absExp)).end;
        });
    }

    template <typename T> requires(Limits<T>::Format == FloatFormat::DoubleDouble128)
    static void WriteFloat_Hex(String& out, T value, const FmtSpecFloat& spec)
    {
        static_assert(Limits<T>::Format != FloatFormat::DoubleDouble128, "Unsupported floating-point type");
        WriteFloat_Hex(out, static_cast<double>(value), spec);
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
                if (zeroesCount <= 0)
                {
                    // prevent "1.e" when precision is default
                    if (spec.precision < 0 && showPoint && significandSize == 1)
                        zeroesCount = 1;
                    else
                        zeroesCount = 0;
                }
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

    static void FormatFloat_Dragon4(String& out, BasicFP<uint128_t> fp, int32_t precision, int32_t& exp10, bool needsFixup, bool isPredecessorCloser, bool isFixedFormat)
    {
        BigInt numerator;           // 2 * R in (FPP)^2.
        BigInt denominator;         // 2 * S in (FPP)^2.
        // lower and upper are differences between value and corresponding boundaries.
        BigInt lower;               // (M^- in (FPP)^2).
        BigInt upperStore;          // upper's value if different from lower.
        BigInt* upper = nullptr;    // (M^+ in (FPP)^2).

        // Shift numerator and denominator by an extra bit or two (if lower boundary is closer)
        // to make lower and upper integers. This eliminates multiplication by 2 during later
        // computations.
        const int32_t shift = isPredecessorCloser ? 2 : 1;
        if (fp.e >= 0)
        {
            numerator = fp.f;
            numerator <<= fp.e + shift;
            lower = 1;
            lower <<= fp.e;
            if (isPredecessorCloser)
            {
                upperStore = 1;
                upperStore <<= fp.e + 1;
                upper = &upperStore;
            }
            denominator.AssignPow10(exp10);
            denominator <<= shift;
        }
        else if (exp10 < 0)
        {
            numerator.AssignPow10(-exp10);
            lower.Assign(numerator);
            if (isPredecessorCloser)
            {
                upperStore.Assign(numerator);
                upperStore <<= 1;
                upper = &upperStore;
            }
            numerator *= fp.f;
            numerator <<= shift;
            denominator = 1;
            denominator <<= shift - fp.e;
        }
        else
        {
            numerator = fp.f;
            numerator <<= shift;
            denominator.AssignPow10(exp10);
            denominator <<= shift - fp.e;
            lower = 1;
            if (isPredecessorCloser)
            {
                upperStore = 1ULL << 1;
                upper = &upperStore;
            }
        }

        const int32_t even = static_cast<int32_t>((fp.f & 1) == 0);

        if (!upper)
            upper = &lower;

        const bool shortest = precision < 0;
        if (needsFixup)
        {
            if (AddCompare(numerator, *upper, denominator) + even <= 0)
            {
                --exp10;
                numerator *= 10;
                if (precision < 0)
                {
                    lower *= 10;
                    if (upper != &lower)
                        *upper *= 10;
                }
            }
            if (isFixedFormat)
                AdjustPrecision(precision, exp10 + 1);
        }

        // Invariant: value == (numerator / denominator) * pow(10, exp10).
        if (shortest)
        {
            // Generate the shortest representation.
            const uint32_t len = out.Size();
            char* it = nullptr;
            precision = 0;
            for (;;)
            {
                if (precision >= static_cast<int32_t>(out.Size() - len))
                    it = FmtResize(out, 16);

                const int32_t digit = numerator.DivModAssign(denominator);
                const bool low = Compare(numerator, lower) - even < 0;  // numerator <[=] lower.
                const bool high = AddCompare(numerator, *upper, denominator) + even > 0; // numerator + upper >[=] pow10:

                *it++ = static_cast<char>('0' + digit);
                ++precision;

                if (low || high)
                {
                    if (!low)
                    {
                        ++(it[-1]);
                    }
                    else if (high)
                    {
                        int result = AddCompare(numerator, numerator, denominator);
                        // Round half to even.
                        if (result > 0 || (result == 0 && (digit % 2) != 0))
                            ++(it[-1]);
                    }
                    out.Resize(precision);
                    exp10 -= precision - 1;
                    return;
                }
                numerator *= 10;
                lower *= 10;
                if (upper != &lower) *upper *= 10;
            }
        }

        // Generate the given number of digits.
        exp10 -= precision - 1;
        if (precision <= 0)
        {
            char digit = '0';
            if (precision == 0)
            {
                denominator *= 10;
                digit = AddCompare(numerator, numerator, denominator) > 0 ? '1' : '0';
            }
            out.PushBack(digit);
            return;
        }

        out.Resize(precision);
        for (int32_t i = 0; i < precision - 1; ++i)
        {
            const int32_t digit = numerator.DivModAssign(denominator);
            out[i] = static_cast<char>('0' + digit);
            numerator *= 10;
        }

        int32_t digit = numerator.DivModAssign(denominator);
        const int32_t result = AddCompare(numerator, numerator, denominator);
        if (result > 0 || (result == 0 && (digit % 2) != 0))
        {
            if (digit == 9)
            {
                const char overflow = '0' + 10;
                out[precision - 1] = overflow;
                // Propagate the carry.
                for (int i = precision - 1; i > 0 && out[i] == overflow; --i) {
                    out[i] = '0';
                    ++(out[i - 1]);
                }
                if (out[0] == overflow) {
                    out[0] = '1';
                    if (needsFixup)
                        out.PushBack('0');
                    else
                        ++exp10;
                }
                return;
            }
            ++digit;
        }
        out[precision - 1] = static_cast<char>('0' + digit);
    }

    template <typename T>
    static int32_t FormatFloat(String& out, T value, int32_t precision, const FmtSpecFloat& spec)
    {
        HE_FMT_ASSERT(value >= 0, "FormatFloat: value should always be positive");

        if (value == 0)
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

        const bool isFixedFormat = spec.type == FmtSpecFloatType::FixedLower || spec.type == FmtSpecFloatType::FixedUpper;

        int32_t exp = 0;
        bool useDragon4 = true;
        bool dragon4Fixup = false;
        if constexpr (sizeof(T) > sizeof(double) || Limits<T>::Format == FloatFormat::DoubleDouble128)
        {
            const double invLog2Of10 = 0.3010299956639812;  // 1 / log2(10)
            const BasicFP<CarrierUint<T>> fp(value);

            // Compute exp, an approximate power of 10, such that
            //   10^(exp - 1) <= value < 10^exp or 10^exp <= value < 10^(exp + 1).
            // This is based on log10(value) == log2(value) / log2(10) and approximation
            // of log2(value) by e + num_fraction_bits idea from double-conversion.
            const double e = (fp.e + CountDigits<1>(fp.f) - 1) * invLog2Of10 - 1e-10;
            exp = static_cast<int>(e);
            if (e > exp) ++exp;  // Compute ceil.
            dragon4Fixup = true;
        }
        else if (precision < 0)
        {
            // With no precision requirements we use dragonbox to output the shortest format.
            if constexpr (IsSame<T, float>)
            {
                const auto dec = dragonbox::ToDecimal(static_cast<float>(value));
                WriteInt(out, dec.significand, {});
                return dec.exponent;
            }
            else
            {
                const auto dec = dragonbox::ToDecimal(static_cast<double>(value));
                WriteInt(out, dec.significand, {});
                return dec.exponent;
            }
        }
        else
        {
            using Info = Limits<double>;
            using Uint = CarrierUint<double>;

            const double dvalue = static_cast<double>(value);

            constexpr Uint ImplicitBit = Uint(1) << Info::SignificandBits;
            constexpr Uint SignificandMask = ImplicitBit - 1;
            constexpr Uint ExponentMask = ((Uint(1) << Info::ExponentBits) - 1) << Info::SignificandBits;

            const Uint bits = BitCast<Uint>(dvalue);
            Uint significand = bits & SignificandMask;
            int32_t exponent = static_cast<int32_t>((bits & ExponentMask) >> Info::SignificandBits);

            if (exponent != 0)
            {
                exponent -= (Info::MaxExponent - 1) + Info::SignificandBits;
                significand |= ImplicitBit;
                significand <<= 1;
            }
            else
            {
                HE_FMT_ASSERT(significand != 0, "Zero significand must be handled before this point");
                uint32_t shift = CountLeadingZeros(significand);
                HE_FMT_ASSERT((shift >= Limits<Uint>::Bits - Info::SignificandBits), "Significand must be normalized");
                shift -= Limits<Uint>::Bits - Info::SignificandBits - 2;
                exponent = (Info::MinExponent - static_cast<int32_t>(Info::SignificandBits)) - shift;
                significand <<= shift;
            }

            // constexpr uint32_t FractionBits = Info::SignificandBits + 1;
            // constexpr uint32_t DigitsCount = (FractionBits + 3) / 4;

            // Compute the first several nonzero decimal significand digits.
            // We call the number we get the first segment.
            static_assert(IsSame<Info, Limits<double>>, "Kappa needs to be updated if Info isn't for double.");
            constexpr int32_t Kappa = 2; // float = 1, double = 2
            const int32_t k = Kappa - dragonbox::FloorLog10Pow2(exponent);
            const int32_t beta = exponent + dragonbox::FloorLog2Pow10(k);
            exp = -k;

            uint64_t firstSegment = 0;
            bool hasMoreSegments = false;
            int32_t digitsInFirstSegment = 0;
            {
                static_assert(IsSame<Info, Limits<double>>, "CacheType needs to be updated if Info isn't for double.");
                using CacheType = dragonbox::CacheAccessor<double>;

                const uint128_t r = dragonbox::UMul192Upper128(significand << beta, CacheType::GetCachedPower(k));
                firstSegment = HE_UINT128_HIGH64(r);
                hasMoreSegments = HE_UINT128_LOW64(r) != 0;

                // The first segment can have 18 ~ 19 digits.
                if (firstSegment >= 1000000000000000000ull)
                {
                    digitsInFirstSegment = 19;
                }
                else
                {
                    // When it is of 18-digits, we align it to 19-digits by adding a bogus
                    // zero at the end.
                    digitsInFirstSegment = 18;
                    firstSegment *= 10;
                }
            }

            // Compute the actual number of decimal digits to print.
            if (isFixedFormat)
            {
                const int32_t exp10 = exp + digitsInFirstSegment;
                AdjustPrecision(precision, exp10);
            }

            if (digitsInFirstSegment > precision)
            {
                useDragon4 = false;

                if (precision <= 0)
                {
                    exp += digitsInFirstSegment;
                    if (precision < 0)
                    {
                        // all we have are leading zeros, so output nothing.
                        // return exp;
                        out.Resize(0);
                    }
                    else
                    {
                        if ((firstSegment | static_cast<uint64_t>(hasMoreSegments)) > 5000000000000000000ull)
                        {
                            out.PushBack('1');
                        }
                        else
                        {
                            out.PushBack('0');
                        }
                    }
                }
                else
                {
                    exp += digitsInFirstSegment - precision;

                    // When precision > 0, we divide the first segment into three
                    // subsegments, each with 9, 9, and 0 ~ 1 digits so that each fits
                    // in 32-bits which usually allows faster calculation than in
                    // 64-bits. Since some compiler (e.g. MSVC) doesn't know how to optimize
                    // division-by-constant for large 64-bit divisors, we do it here
                    // manually. The magic number 7922816251426433760 below is equal to
                    // ceil(2^(64+32) / 10^10).
                    const uint64_t result = dragonbox::UMul128Upper64(firstSegment, 7922816251426433760ull);
                    const uint32_t firstSubsegment = static_cast<uint32_t>(result >> 32);
                    const uint64_t secondAndThirdSubsegments = firstSegment - (firstSubsegment * 10000000000ull);

                    uint64_t prod = 0;
                    uint32_t digits = 0;
                    int32_t digitsToPrint = precision > 9 ? 9 : precision;

                    // Print a 9-digit subsegment.
                    auto printSubsegment = [&](char* buffer, uint32_t subsegment)
                    {
                        int32_t digitsPrinted = 0;

                        // Odd number of digits to be printed
                        if ((digitsToPrint & 1) != 0)
                        {
                            // Conver to 64-bit fixed-point fractional form with 1-digit integer part.
                            // The magic number 720575941 is a good enough approximation of
                            // `2^(32 + 24) / 10^8`.
                            // See: https://jk-jeon.github.io/posts/2022/12/fixed-precision-formatting/#fixed-length-case
                            prod = ((subsegment * 720575941ull) >> 24) + 1;
                            digits = static_cast<uint32_t>(prod >> 32);
                            *buffer = static_cast<char>('0' + digits);
                            ++digitsPrinted;
                        }
                        // Event number of digits to be printed
                        else
                        {
                            // Convert to 64-bit fixed-point fractional form with 2-digits integer part.
                            // The magic number 450359963 is a good enough approximation of
                            // `2^(32 + 20) / 10^7`.
                            // See: https://jk-jeon.github.io/posts/2022/12/fixed-precision-formatting/#fixed-length-case
                            prod = ((subsegment * 450359963ull) >> 20) + 1;
                            digits = static_cast<uint32_t>(prod >> 32);
                            MemCopy(buffer, Digits2(digits), 2);
                            digitsPrinted += 2;
                        }

                        // Print all digit pairs.
                        while (digitsPrinted < digitsToPrint)
                        {
                            prod = static_cast<uint32_t>(prod) * static_cast<uint64_t>(100);
                            digits = static_cast<uint32_t>(prod >> 32);
                            MemCopy(buffer + digitsPrinted, Digits2(digits), 2);
                            digitsPrinted += 2;
                        }
                    };

                    char* buf = FmtResize(out, digitsToPrint);
                    printSubsegment(buf, firstSubsegment);

                    // Perform rounding if the first subsegment is the last subsegment to print.
                    bool shouldRoundUp = false;
                    if (precision <= 9)
                    {
                        // Rounding inside the subsegment.
                        // We round-up if:
                        //  - either the fractional part is strictly larger than 1/2, or
                        //  - the fractional part is exactly 1/2 and the last digit is odd.
                        // We rely on the following observations:
                        //  - If `fractional` >= threshold, then the fractional part is
                        //    strictly larger than 1/2.
                        //  - If the MSB of `fractional` is set, then the fractional part
                        //    must be at least 1/2.
                        //  - When the MSB of `fractional` is set, either
                        //    `secondAndThirdSubsegments` being nonzero or `hasMoreSegments`
                        //    being true means there are further digits not printed, so the
                        //    fractional part is strictly larger than 1/2.
                        if (precision < 9)
                        {
                            const uint32_t fractional = static_cast<uint32_t>(prod);
                            shouldRoundUp = fractional >= FractionPartRoundingThresholds(8 - digitsToPrint)
                                || ((fractional >> 31) & ((digits & 1) | (secondAndThirdSubsegments != 0) | hasMoreSegments)) != 0;
                        }
                        // Rounding at the subsegment boundary.
                        // In this case, the fractional part is at least 1/2 if and only if
                        // `secondAndThirdSubsegments >= 5000000000ull`, and is strictly larger
                        // than 1/2 if we further have either
                        // `secondAndThirdSubsegments > 5000000000ull` or `hasMoreSegments == true`.
                        else
                        {
                            shouldRoundUp = secondAndThirdSubsegments > 5000000000ull
                                || (secondAndThirdSubsegments == 5000000000ull && ((digits & 1) != 0 || hasMoreSegments));
                        }
                    }
                    // Otherwise, print the second subsegment.
                    else
                    {
                        // Compilers are not aware of how to leverage the maximum value of
                        // `secondAndThirdSubsegments` to find out a better magic number which
                        // allows us to eliminate an additional shift. 1844674407370955162 =
                        // ceil(2^64/10) < ceil(2^64*(10^9/(10^10 - 1))).
                        const uint32_t secondSubsegment = static_cast<uint32_t>(dragonbox::UMul128Upper64(secondAndThirdSubsegments, 1844674407370955162ull));
                        const uint32_t thirdSubsegment = static_cast<uint32_t>(secondAndThirdSubsegments) - (secondSubsegment * 10);

                        digitsToPrint = precision - 9;
                        buf = FmtResize(out, digitsToPrint);
                        printSubsegment(buf, secondSubsegment);

                        // Rounding inside the subsegment.
                        if (precision < 18)
                        {
                            // The condition third_subsegment != 0 implies that the segment was
                            // of 19 digits, so in this case the third segment should be
                            // consisting of a genuine digit from the input.
                            const uint32_t fractional = static_cast<uint32_t>(prod);
                            shouldRoundUp = fractional >= FractionPartRoundingThresholds(8 - digitsToPrint)
                                || ((fractional >> 31) & ((digits & 1) | (thirdSubsegment != 0) | hasMoreSegments)) != 0;
                        }
                        // Rounding at the subsegment boundary.
                        else
                        {
                            // In this case, the segment must be of 19 digits, thus
                            // the third subsegment should be consisting of a genuine digit from
                            // the input.
                            shouldRoundUp = thirdSubsegment > 5 || (thirdSubsegment == 5 && ((digits & 1) != 0 || hasMoreSegments));
                        }
                    }

                    // Round-up if necessary.
                    if (shouldRoundUp)
                    {
                        ++buf[precision - 1];
                        for (int32_t i = precision - 1; i > 0 && buf[i] > '9'; --i)
                        {
                            buf[i] = '0';
                            ++buf[i - 1];
                        }

                        if (buf[0] > '9')
                        {
                            buf[0] = '1';
                            if (isFixedFormat)
                            {
                                buf = FmtResize(out, 1);
                                buf[0] = '0';
                                ++precision;
                            }
                            else
                            {
                                ++exp;
                            }
                        }
                    }
                }
            }
            else
            {
                // Adjust the exponent for use with Dragon4
                exp += digitsInFirstSegment - 1;
            }
        }

        if (useDragon4)
        {
            BasicFP<uint128_t> fp;
            const bool isPredecessorCloser = fp.Assign(value);

            // Limit precision to the maximum possible number of significant digits in
            // an IEEE754 double because we don't need to generate zeros.
            const int maxDoubleDigits = 767;
            if (precision > maxDoubleDigits)
                precision = maxDoubleDigits;

            FormatFloat_Dragon4(out, fp, precision, exp, dragon4Fixup, isPredecessorCloser, isFixedFormat);
        }

        if (!isFixedFormat && !ShouldShowPoint(spec))
        {
            // Remove trailing zeros.
            uint32_t digitCount = out.Size();
            while (digitCount > 0 && out[digitCount - 1] == '0')
            {
                --digitCount;
                ++exp;
            }
            out.Resize(digitCount);
        }

        return exp;
    }

    template <FloatingPoint T>
    static void WriteFloat(String& out, T value, const FmtSpecFloat& spec_)
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
            WriteFloat_Hex(out, value, spec);
            return;
        }

        int32_t precision = spec.precision >= 0 || IsDefault(spec) ? spec.precision : 6;

        if (spec.type == FmtSpecFloatType::ExponentLower || spec.type == FmtSpecFloatType::ExponentUpper)
        {
            if (precision == Limits<int32_t>::Max) [[unlikely]]
                FmtError("Float precision is too large.");
            else
                ++precision;
        }
        else if (precision == 0 && spec.type != FmtSpecFloatType::FixedLower && spec.type != FmtSpecFloatType::FixedUpper)
        {
            precision = 1;
        }

        // Very good chance this won't allocate. Almost all floats write out in < 30 characters.
        String buffer;
        const int32_t exponent = FormatFloat(buffer, value, precision, spec);
        spec.precision = precision;
        WriteFloat(out, buffer, exponent, spec);
    }

    [[noreturn]] void FmtError(const char* msg)
    {
        PrintToDebugger("FmtError: ");
        PrintToDebugger(msg);
        PrintToDebugger("\n");
        TerminateProcess();
    }

    void Formatter<bool>::Format(String& out, bool value) const
    {
        if (spec.type != FmtSpecIntType::Default)
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
        if (spec.type == FmtSpecIntType::Default || spec.type == FmtSpecIntType::Char)
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
        if (spec.type == FmtSpecIntType::Default || spec.type == FmtSpecIntType::Char)
        {
            wchar_t wstr[]{ value, L'\0' };
            char str[4]{};
            const uint32_t len = WCToMBStr(str, 4, wstr);

            WritePadded(out, spec, len, [&](char* it)
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
    #if HE_SIZEOF_LONG == 4
        WriteInt(out, static_cast<uint32_t>(value), spec);
    #else
        WriteInt(out, static_cast<uint64_t>(value), spec);
    #endif
    }

    void Formatter<unsigned long long>::Format(String& out, unsigned long long value) const
    {
        WriteInt(out, static_cast<uint64_t>(value), spec);
    }

    void Formatter<float>::Format(String& out, float value) const
    {
        WriteFloat(out, value, spec);
    }

    void Formatter<double>::Format(String& out, double value) const
    {
        WriteFloat(out, value, spec);
    }

    void Formatter<long double>::Format(String& out, long double value) const
    {
        WriteFloat(out, value, spec);
    }

    void Formatter<const void*>::Format(String& out, const void* value) const
    {
        const uintptr_t uvalue = BitCast<uintptr_t>(value);
        const uint32_t digitCount = CountDigits<4>(uvalue);
        const uint32_t size = digitCount + 2;

        WritePadded<FmtSpecAlign::Right>(out, spec, size, [&](char* it)
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

        WritePadded(out, spec, size, width, [&](char* it)
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
