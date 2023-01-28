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
    static_assert(Limits<T>::Min == std::numeric_limits<T>::min());
    static_assert(Limits<T>::Max == std::numeric_limits<T>::max());
}

template <typename T>
static void TestFloatingPointType()
{
    static_assert(IsSame<T, typename Limits<T>::Type>);
    static_assert(Limits<T>::IsSigned == std::numeric_limits<T>::is_signed);
    static_assert(Limits<T>::Bits == (sizeof(T) * 8));
    static_assert(Limits<T>::SignBits == 1);
    // static_assert(Limits<T>::ExponentBits == 0);
    static_assert(Limits<T>::MantissaBits == std::numeric_limits<T>::digits);
    static_assert(Limits<T>::Min == std::numeric_limits<T>::lowest());
    static_assert(Limits<T>::Max == std::numeric_limits<T>::max());
    static_assert(Limits<T>::MinPos == std::numeric_limits<T>::min());
    static_assert(Limits<T>::Epsilon == std::numeric_limits<T>::epsilon());
    static_assert(Limits<T>::Infinity == std::numeric_limits<T>::infinity());
    HE_EXPECT(std::isnan(Limits<T>::NaN));
    HE_EXPECT(std::isnan(Limits<T>::SignalingNaN));
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
    TestFloatingPointType<float>();
    TestFloatingPointType<double>();
    TestFloatingPointType<long double>();
}
