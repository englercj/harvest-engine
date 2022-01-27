// Copyright Chad Engler

#include "he/core/cpu.h"

#include "he/core/test.h"

#include <iostream>

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, cpu, Report)
{
    std::cout << "    HE_CPU_ARM = " << HE_CPU_ARM << std::endl;
    std::cout << "    HE_CPU_ARM_32 = " << HE_CPU_ARM_32 << std::endl;
    std::cout << "    HE_CPU_ARM_64 = " << HE_CPU_ARM_64 << std::endl;

    std::cout << "    HE_CPU_WASM = " << HE_CPU_WASM << std::endl;
    std::cout << "    HE_CPU_WASM_32 = " << HE_CPU_WASM_32 << std::endl;
    std::cout << "    HE_CPU_WASM_64 = " << HE_CPU_WASM_64 << std::endl;

    std::cout << "    HE_CPU_X86 = " << HE_CPU_X86 << std::endl;
    std::cout << "    HE_CPU_X86_32 = " << HE_CPU_X86_32 << std::endl;
    std::cout << "    HE_CPU_X86_64 = " << HE_CPU_X86_64 << std::endl;

    std::cout << "    HE_CPU_64_BIT = " << HE_CPU_64_BIT << std::endl;
}
