// Copyright Chad Engler

#include "he/core/simd.h"

#include "he/core/test.h"

#include <iostream>

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, simd, Report)
{
    std::cout << "    HE_SIMD_AVX = " << HE_SIMD_AVX << std::endl;
    std::cout << "    HE_SIMD_AVX2 = " << HE_SIMD_AVX2 << std::endl;
    std::cout << "    HE_SIMD_NEON = " << HE_SIMD_NEON << std::endl;
    std::cout << "    HE_SIMD_SSE2 = " << HE_SIMD_SSE2 << std::endl;
    std::cout << "    HE_SIMD_SSE3 = " << HE_SIMD_SSE3 << std::endl;
    std::cout << "    HE_SIMD_SSE4_1 = " << HE_SIMD_SSE4_1 << std::endl;
    std::cout << "    HE_SIMD_SSE4_2 = " << HE_SIMD_SSE4_2 << std::endl;
    std::cout << "    HE_SIMD_FMA3 = " << HE_SIMD_FMA3 << std::endl;
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, simd, MakeSimd128)
{
    const Simd128 v0 = MakeSimd128(1, 2, 3, 4);
    const Simd128 v1 = MakeSimd128(1, 2, 3, 4);
    const Simd128 v2 = MakeSimd128(4, 3, 2, 1);

    HE_EXPECT_EQ_MEM(&v0, &v1, sizeof(v0));
    HE_EXPECT_NE_MEM(&v0, &v2, sizeof(v0));
}
