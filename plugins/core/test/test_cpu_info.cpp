// Copyright Chad Engler

#include "he/core/cpu_info.h"

#include "he/core/enum_ops.h"
#include "he/core/test.h"

#include <iostream>

using namespace he;

// ------------------------------------------------------------------------------------------------
HE_TEST(core, cpu_info, Report)
{
    const CpuInfo& info = GetCpuInfo();

    std::cout << "    vendorId: '" << EnumToString(info.vendorId) << "' (" << EnumToValue(info.vendorId) << ')' << std::endl;
    std::cout << "    vendorName: '" << info.vendorName << "'" << std::endl;
    std::cout << "    core count: " << info.coreCount << std::endl;
    std::cout << "    thread count: " << info.threadCount << std::endl;
    std::cout << "    cache line size: " << info.cacheLineSize << std::endl;

    std::cout << "    x86:" << std::endl;
    std::cout << "        brandName: '" << info.x86.brandName << "'" << std::endl;
    std::cout << "        steppingId: " << uint32_t(info.x86.steppingId) << std::endl;
    std::cout << "        modelId: " << info.x86.modelId << std::endl;
    std::cout << "        familyId: " << info.x86.familyId << std::endl;
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
    std::cout << "        sha1: " << info.x86.sha1 << std::endl;
    std::cout << "        sha256: " << info.x86.sha256 << std::endl;
    std::cout << "        sha512: " << info.x86.sha512 << std::endl;
    std::cout << "        sm3: " << info.x86.sm3 << std::endl;
    std::cout << "        sm4: " << info.x86.sm4 << std::endl;
    std::cout << "        rdrand: " << info.x86.rdrand << std::endl;
    std::cout << "        osxsave: " << info.x86.osxsave << std::endl;
    std::cout << "        hypervisor: " << info.x86.hypervisor << std::endl;
    std::cout << "        tscInvariant: " << info.x86.tscInvariant << std::endl;

    std::cout << "    arm:" << std::endl;
    std::cout << "        neon: " << info.arm.neon << std::endl;
    std::cout << "        aes: " << info.arm.aes << std::endl;
    std::cout << "        sha1: " << info.arm.sha1 << std::endl;
    std::cout << "        sha256: " << info.arm.sha256 << std::endl;
    std::cout << "        sha512: " << info.arm.sha512 << std::endl;
    std::cout << "        sha3: " << info.arm.sha3 << std::endl;
    std::cout << "        crc32: " << info.arm.crc32 << std::endl;
    std::cout << "        atomic: " << info.arm.atomic << std::endl;
    std::cout << "        rdm: " << info.arm.rdm << std::endl;
    std::cout << "        sm3: " << info.arm.sm3 << std::endl;
    std::cout << "        sm4: " << info.arm.sm4 << std::endl;
    std::cout << "        dp: " << info.arm.dp << std::endl;
    std::cout << "        fhm: " << info.arm.fhm << std::endl;
    std::cout << "        rndr: " << info.arm.rndr << std::endl;
}
