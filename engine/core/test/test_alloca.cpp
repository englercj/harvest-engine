// Copyright Chad Engler

#include "fixtures.h"

#include "he/core/alloca.h"

#include "he/core/test.h"

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, alloca, ALLOCA)
{
    char* c = HE_ALLOCA(char, 8);
    HE_EXPECT(c);

    Trivial* t = HE_ALLOCA(Trivial, 2);
    HE_EXPECT(t);

    // Won't compile
    //NonTrivial* nt = HE_ALLOCA(NonTrivial, 2);
    //HE_EXPECT(nt);
}
