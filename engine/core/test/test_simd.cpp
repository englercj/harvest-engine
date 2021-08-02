// Copyright Chad Engler

#include "he/core/simd.h"

#include "he/core/test.h"

#include <iostream>

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, simd, Report)
{
    std::cout << "    HE_ENABLE_SIMD = " << HE_ENABLE_SIMD << std::endl;
    std::cout << "    HE_SIMD_AVX = " << HE_SIMD_AVX << std::endl;
    std::cout << "    HE_SIMD_AVX2 = " << HE_SIMD_AVX2 << std::endl;
    std::cout << "    HE_SIMD_NEON = " << HE_SIMD_NEON << std::endl;
    std::cout << "    HE_SIMD_SSE2 = " << HE_SIMD_SSE2 << std::endl;
    std::cout << "    HE_SIMD_SSE3 = " << HE_SIMD_SSE3 << std::endl;
    std::cout << "    HE_SIMD_SSE4_1 = " << HE_SIMD_SSE4_1 << std::endl;
    std::cout << "    HE_SIMD_SSE4_2 = " << HE_SIMD_SSE4_2 << std::endl;
    std::cout << "    HE_SIMD_FMA3 = " << HE_SIMD_FMA3 << std::endl;
}
