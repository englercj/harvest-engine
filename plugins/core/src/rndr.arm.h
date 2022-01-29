// Copyright Chad Engler

#pragma once

#include "he/core/cpu.h"
#include "he/core/compiler.h"

#if HE_CPU_ARM

namespace he
{
    HE_FORCE_INLINE bool _Rndr(size_t& outValue)
    {
        // Reads of RNDR set PSTATE.NZCV to 0b0000 on success and 0b0100 otherwise.
        bool ok;
        #if HE_COMPILER_MSVC
            outValue = static_cast<size_t>(_ReadStatusReg(ARM64_RNDR_EL0));
            __asm cset ok, ne;
        #else
            asm volatile(
                "mrs %0, RNDR_EL0\n\t"
                "cset %w1, ne"
                : "=r"(outValue), "=r"(ok) :: "cc");
        #endif
        return ok;
    }
}

#endif
