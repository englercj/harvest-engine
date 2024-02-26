// Copyright Chad Engler

#include "fmt_private.h"

#include "he/core/types.h"

namespace he
{
    // An optimization by Kendall Willets from https://bit.ly/3uOIQrB.
    // This increments the upper 32 bits (log10(T) - 1) when >= T is added.
    #define HE_INC(T) (((sizeof(#T) - 1ull) << 32) - T)
    const uint64_t DigitCountLookup[] =
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

    #define HE_POWERS_OF_10(factor)                                 \
        (factor)*10, (factor)*100, (factor)*1000, (factor)*10000,   \
        (factor)*100000, (factor)*1000000, (factor)*10000000,       \
        (factor)*100000000, (factor)*1000000000
    const uint64_t ZeroOrPowersOf10[] =
    {
        0, 0, HE_POWERS_OF_10(1u), HE_POWERS_OF_10(1000000000ull), 10000000000000000000ull,
    };
    #undef HE_POWERS_OF_10

    const uint8_t Bsr2Log10[] =
    {
        1,  1,  1,  2,  2,  2,  3,  3,  3,  4,  4,  4,  4,  5,  5,  5,
        6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9,  10, 10, 10,
        10, 11, 11, 11, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 15, 15,
        15, 16, 16, 16, 16, 17, 17, 17, 18, 18, 18, 19, 19, 19, 19, 20,
    };
}
