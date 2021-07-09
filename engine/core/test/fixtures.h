// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

namespace he
{
    struct Trivial { uint32_t a; };

    struct NonTrivial
    {
        int* p;
        NonTrivial() { p = new int; }
        ~NonTrivial() { delete p; }
    };

    struct VirtualDestructor { virtual ~VirtualDestructor() {} };
}
