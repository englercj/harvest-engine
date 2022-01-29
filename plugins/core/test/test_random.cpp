// Copyright Chad Engler

#include "he/core/random.h"

#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, random, GetSecureRandomBytes)
{
    uint8_t values[32];
    HE_EXPECT(GetSecureRandomBytes(values));

    bool somethingIsDifferent = false;
    for (uint8_t v : values)
    {
        if (v != values[0])
        {
            somethingIsDifferent = true;
            break;
        }
    }

    HE_EXPECT(somethingIsDifferent);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, random, GetHardwareRandomBytes)
{
    uint8_t values[32];
    HE_EXPECT(GetHardwareRandomBytes(values));

    bool somethingIsDifferent = false;
    for (uint8_t v : values)
    {
        if (v != values[0])
        {
            somethingIsDifferent = true;
            break;
        }
    }

    HE_EXPECT(somethingIsDifferent);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, random, GetSystemRandomBytes)
{
    uint8_t values[32];
    HE_EXPECT(GetSystemRandomBytes(values));

    bool somethingIsDifferent = false;
    for (uint8_t v : values)
    {
        if (v != values[0])
        {
            somethingIsDifferent = true;
            break;
        }
    }

    HE_EXPECT(somethingIsDifferent);
}
