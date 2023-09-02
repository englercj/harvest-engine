// Copyright Chad Engler

#include "he/core/cpu_info.h"

#include "he/core/compiler.h"
#include "he/core/cpu.h"
#include "he/core/macros.h"
#include "he/core/memory_ops.h"
#include "he/core/string_view.h"

#if HE_CPU_X86

#include "rdrand.x86.h"

#if HE_COMPILER_MSVC
    #include <intrin.h>
#else
    #include <cpuid.h>
#endif

namespace he
{
    void _FillPlatformCpuInfo(CpuInfo& info);

    struct VendorIdMapping
    {
        StringView name;
        CpuVendorId id;
    };

    static const VendorIdMapping VendorIdMap[] =
    {
        { "GenuineIntel", CpuVendorId::Intel },
        { "AMDisbetter!", CpuVendorId::AMD },
        { "AuthenticAMD", CpuVendorId::AMD },
        { "CentaurHauls", CpuVendorId::VIA },
        { "CyrixInstead", CpuVendorId::Cyrix },
        { "TransmetaCPU", CpuVendorId::Transmeta },
        { "GenuineTMx86", CpuVendorId::Transmeta },
        { "Geode by NSC", CpuVendorId::NSC },
        { "NexGenDriven", CpuVendorId::NexGen },
        { "RiseRiseRise", CpuVendorId::SiS },
        { "SiS SiS SiS ", CpuVendorId::SiS },
        { "UMC UMC UMC ", CpuVendorId::UMC },
        { "VIA VIA VIA ", CpuVendorId::VIA },
        { "Vortex86 SoC", CpuVendorId::SiS },
        { "  Shanghai  ", CpuVendorId::Zhaoxin },
        { "HygonGenuine", CpuVendorId::Hygon },
        { "E2K MACHINE", CpuVendorId::Elbrus },
        { "Genuine  RDC", CpuVendorId::RDC },

        { "bhyve bhyve ", CpuVendorId::Bhyve },
        { "KVMKVMKVMKVM", CpuVendorId::KVM },
        { " KVMKVMKVM  ", CpuVendorId::KVM },
        { "TCGTCGTCGTCG", CpuVendorId::QEMU },
        { "Microsoft Hv", CpuVendorId::MSVM },
        { " lrpepyh  vr", CpuVendorId::Parallels },
        { "VMwareVMware", CpuVendorId::VMware },
        { "XenVMMXenVMM", CpuVendorId::XenHVM },
        { "ACRNACRNACRN", CpuVendorId::ACRN },
        { " QNXQVMBSQG ", CpuVendorId::QNX },
    };

    static bool TestRdRand()
    {
        size_t values[8]{};

        // Run the instruction a few times to trigger the built-in self-test. If the instruction
        // fails the test we consider it unsupported.
        for (size_t& v : values)
        {
            if (!_RdRand(v))
                return false;
        }

        // Check if we actually are getting random-looking data. Some processors had bugs where
        // they would report functioning rdrand but not actually give valid data.
        // See: https://arstechnica.com/gadgets/2019/10/how-a-months-old-amd-microcode-bug-destroyed-my-weekend/
        for (size_t& v : values)
        {
            if (v != values[0])
                return true;
        }

        return false;
    }

    HE_FORCE_INLINE void CpuId(uint32_t leaf, uint32_t& eax, uint32_t& ebx, uint32_t& ecx, uint32_t& edx)
    {
    #if HE_COMPILER_MSVC
        int32_t regs[4];
        __cpuid(regs, leaf);
        eax = regs[0];
        ebx = regs[1];
        ecx = regs[2];
        edx = regs[3];
    #else
        __cpuid(leaf, eax, ebx, ecx, edx);
    #endif
    }

    HE_FORCE_INLINE void CpuIdEx(uint32_t leaf, uint32_t subleaf, uint32_t& eax, uint32_t& ebx, uint32_t& ecx, uint32_t& edx)
    {
    #if HE_COMPILER_MSVC
        int32_t regs[4];
        __cpuidex(regs, leaf, subleaf);
        eax = regs[0];
        ebx = regs[1];
        ecx = regs[2];
        edx = regs[3];
    #else
        __cpuid_count(leaf, subleaf, eax, ebx, ecx, edx);
    #endif
    }

    struct CpuInfoImpl : CpuInfo
    {
        CpuInfoImpl()
        {
            uint32_t eax = 0;
            uint32_t ebx = 0;
            uint32_t ecx = 0;
            uint32_t edx = 0;

            // Function Id 0 = Highest Function Parameter and Manufacturer ID
            CpuId(0, eax, ebx, ecx, edx);

            const uint32_t highestFuncId = eax;
            MemCopy(vendorName + 0, &ebx, 4);
            MemCopy(vendorName + 8, &ecx, 4);
            MemCopy(vendorName + 4, &edx, 4);

            for (uint32_t i = 0; i < HE_LENGTH_OF(VendorIdMap); ++i)
            {
                if (VendorIdMap[i].name == vendorName)
                {
                    vendorId = VendorIdMap[i].id;
                    break;
                }
            }

            // Function Id 1 = Processor Info and Feature Bits
            if (highestFuncId >= 1)
            {
                CpuId(1, eax, ebx, ecx, edx);

                x86.sse = (edx & (1 << 25)) != 0;
                x86.sse2 = (edx & (1 << 26)) != 0;

                x86.sse3 = (ecx & (1 << 0)) != 0;
                x86.ssse3 = (ecx & (1 << 9)) != 0;
                x86.fma3 = (ecx & (1 << 12)) != 0;
                x86.sse41 = (ecx & (1 << 19)) != 0;
                x86.sse42 = (ecx & (1 << 20)) != 0;
                x86.popcnt = (ecx & (1 << 23)) != 0;
                x86.aesni = (ecx & (1 << 25)) != 0;
                x86.osxsave = (ecx & (1 << 27)) != 0;
                x86.avx = (ecx & (1 << 28)) != 0;
                x86.rdrand = (ecx & (1 << 30)) != 0;
                x86.hypervisor = (ecx & (1 << 31)) != 0;

                // Check if OS has enabled extended support necessary for AVX
                if (x86.osxsave && x86.avx)
                {
                    const uint64_t mask = _xgetbv(0);
                    x86.avx = (mask & 6) == 6;
                }

                // Disable FMA3 if AVX is not available.
                if (!x86.avx)
                {
                    x86.fma3 = false;
                }

                if (x86.rdrand)
                {
                    x86.rdrand = TestRdRand();
                }
            }

            // Function Id 7 = Extended Features
            if (highestFuncId >= 7)
            {
                CpuId(7, eax, ebx, ecx, edx);

                x86.avx2 = (ebx & (1 << 5)) != 0;
                x86.sha1 = (ebx & (1 << 29)) != 0;
                x86.sha256 = x86.sha1;

                if (!x86.avx)
                    x86.avx2 = false;

                CpuIdEx(7, 1, eax, ebx, ecx, edx);
                x86.sha512 = (eax & (1 << 0)) != 0;
                x86.sm3 = (eax & (1 << 1)) != 0;
                x86.sm4 = (eax & (1 << 2)) != 0;
            }

            // Function Id 80000000h = Get Highest Extended Function Implemented
            CpuId(0x80000000, eax, ebx, ecx, edx);
            const uint32_t highestExFuncId = eax;

            // Function Id 80000007h = Advanced Power Management Information
            if (highestExFuncId >= 0x80000007)
            {
                CpuId(0x80000007, eax, ebx, ecx, edx);
                x86.tscInvariant = (edx & (1 << 8)) != 0;
            }

            _FillPlatformCpuInfo(*this);
        }
    };

    const CpuInfo& GetCpuInfo()
    {
        static CpuInfoImpl s_info;
        return s_info;
    }
}

#endif
