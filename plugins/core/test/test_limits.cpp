// Copyright Chad Engler

#include "he/core/limits.h"

#include "he/core/test.h"

#include <cmath>
#include <limits>

using namespace he;

// ------------------------------------------------------------------------------------------------
template <typename T>
static void TestIntegralType()
{
    static_assert(IsSame<T, typename Limits<T>::Type>);
    static_assert(Limits<T>::IsSigned == std::numeric_limits<T>::is_signed);
    static_assert(Limits<T>::Bits == (sizeof(T) * 8));
    static_assert(Limits<T>::SignBits == (IsSigned<T> ? 1 : 0));
    static_assert(Limits<T>::ValueBits == (IsSame<T, bool> ? 8 : T(std::numeric_limits<T>::digits)));
    static_assert(Limits<T>::Digits == std::numeric_limits<T>::digits);
    static_assert(Limits<T>::Digits10 == std::numeric_limits<T>::digits10);
    static_assert(Limits<T>::Min == std::numeric_limits<T>::min());
    static_assert(Limits<T>::Max == std::numeric_limits<T>::max());
}

template <typename T>
static void TestFloatingPointType()
{
    static_assert(IsSame<T, typename Limits<T>::Type>);
    static_assert(Limits<T>::IsSigned == std::numeric_limits<T>::is_signed);
    //static_assert(Limits<T>::HasImplicitBit == true); // TODO
    static_assert(Limits<T>::Bits == (sizeof(T) * 8));
    static_assert(Limits<T>::Digits == std::numeric_limits<T>::digits);
    static_assert(Limits<T>::Digits10 == std::numeric_limits<T>::digits10);
    static_assert(Limits<T>::SignBits == 1);
    static_assert(Limits<T>::SignificandBits == (std::numeric_limits<T>::digits - static_cast<uint32_t>(Limits<T>::HasImplicitBit)));
    static_assert(Limits<T>::ExponentBits == Limits<T>::Bits - (Limits<T>::SignBits + Limits<T>::SignificandBits));
    static_assert(Limits<T>::MaxExponent == std::numeric_limits<T>::max_exponent);
    static_assert(Limits<T>::MinExponent == std::numeric_limits<T>::min_exponent);
    static_assert(Limits<T>::Min == std::numeric_limits<T>::lowest());
    static_assert(Limits<T>::Max == std::numeric_limits<T>::max());
    static_assert(Limits<T>::MinPos == std::numeric_limits<T>::min());
    static_assert(Limits<T>::Epsilon == std::numeric_limits<T>::epsilon());
    static_assert(Limits<T>::Infinity == std::numeric_limits<T>::infinity());
    static_assert(Limits<T>::ZeroSafe == std::numeric_limits<T>::min() * T(1000.0));

    HE_EXPECT(std::isnan(Limits<T>::NaN));
    HE_EXPECT(std::isnan(Limits<T>::SignalingNaN));

    const T nanValue = Limits<T>::NaN;
    const T stdNanValue = std::numeric_limits<T>::quiet_NaN();
    HE_EXPECT_EQ_MEM(&nanValue, &stdNanValue, sizeof(T));

    const T snanValue = Limits<T>::SignalingNaN;
    const T stdSNanValue = std::numeric_limits<T>::signaling_NaN();
    HE_EXPECT_EQ_MEM(&snanValue, &stdSNanValue, sizeof(T));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, limits, assumptions)
{
    static_assert(std::numeric_limits<float>::is_iec559
               && std::numeric_limits<float>::digits == 24
               && std::numeric_limits<float>::max_exponent == 128,
        "IEEE-754 single-precision assumed");

    static_assert(std::numeric_limits<double>::is_iec559
               && std::numeric_limits<double>::digits == 53
               && std::numeric_limits<double>::max_exponent == 1024,
        "IEEE-754 double-precision assumed");

    static_assert(std::numeric_limits<long double>::is_iec559);

    static_assert(std::numeric_limits<long double>::digits ==
        (Limits<long double>::Format == FloatFormat::Binary32 ? 24
        : Limits<long double>::Format == FloatFormat::Binary64 ? 53
        : Limits<long double>::Format == FloatFormat::Binary128 ? 113
        : Limits<long double>::Format == FloatFormat::Extended80 ? 164
        : Limits<long double>::Format == FloatFormat::DoubleDouble128 ? 106
        : 0));
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, limits, integers)
{
    TestIntegralType<bool>();
    TestIntegralType<signed char>();
    TestIntegralType<wchar_t>();
    TestIntegralType<char8_t>();
    TestIntegralType<char16_t>();
    TestIntegralType<char32_t>();

    TestIntegralType<char>();
    TestIntegralType<short>();
    TestIntegralType<int>();
    TestIntegralType<long>();
    TestIntegralType<long long>();

    TestIntegralType<unsigned char>();
    TestIntegralType<unsigned short>();
    TestIntegralType<unsigned int>();
    TestIntegralType<unsigned long>();
    TestIntegralType<unsigned long long>();
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, limits, floats)
{
    // Test our limit values against the standard library
    TestFloatingPointType<float>();
    TestFloatingPointType<double>();
     TestFloatingPointType<long double>();
}
