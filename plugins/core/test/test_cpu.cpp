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

    const CpuInfo& info = GetCpuInfo();

    std::cout << "    vendorId: " << static_cast<int32_t>(info.vendorId) << std::endl;
    std::cout << "    vendorName: '" << info.vendorName << "'" << std::endl;
    std::cout << "    core count: " << info.coreCount << std::endl;
    std::cout << "    thread count: " << info.threadCount << std::endl;

    std::cout << "    x86:" << std::endl;
    std::cout << "        sse: " << info.x86.sse << std::endl;
    std::cout << "        sse2: " << info.x86.sse2 << std::endl;
    std::cout << "        sse3: " << info.x86.sse3 << std::endl;
    std::cout << "        ssse3: " << info.x86.ssse3 << std::endl;
    std::cout << "        sse41: " << info.x86.sse41 << std::endl;
    std::cout << "        sse42: " << info.x86.sse42 << std::endl;
    std::cout << "        avx: " << info.x86.avx << std::endl;
    std::cout << "        avx2: " << info.x86.avx2 << std::endl;
    std::cout << "        popcnt: " << info.x86.popcnt << std::endl;
    std::cout << "        fma3: " << info.x86.fma3 << std::endl;
    std::cout << "        aesni: " << info.x86.aesni << std::endl;
    std::cout << "        sha: " << info.x86.sha << std::endl;
    std::cout << "        rdrand: " << info.x86.rdrand << std::endl;
    std::cout << "        osxsave: " << info.x86.osxsave << std::endl;
    std::cout << "        hypervisor: " << info.x86.hypervisor << std::endl;

    std::cout << "    ARM:" << std::endl;
    std::cout << "        neon: " << info.arm.neon << std::endl;
    std::cout << "        rndr: " << info.arm.rndr << std::endl;
    std::cout << "        aes: " << info.arm.aes << std::endl;
    std::cout << "        sha1: " << info.arm.sha1 << std::endl;
    std::cout << "        sha2: " << info.arm.sha2 << std::endl;
    std::cout << "        sha3: " << info.arm.sha3 << std::endl;
    std::cout << "        crc32: " << info.arm.crc32 << std::endl;
    std::cout << "        atomic: " << info.arm.atomic << std::endl;
}
