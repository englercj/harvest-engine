// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

#define HE_CPU_ARM                  0
#define HE_CPU_ARM_32               0
#define HE_CPU_ARM_64               0

#define HE_CPU_WASM                 0
#define HE_CPU_WASM_32              0
#define HE_CPU_WASM_64              0

#define HE_CPU_X86                  0
#define HE_CPU_X86_32               0
#define HE_CPU_X86_64               0

#define HE_CPU_64_BIT               0

#if defined(__BIG_ENDIAN__) || defined(__ARMEB__)
    #error "Big endian systems are not supported."
#endif

#if defined(_M_ARM) || defined(__arm__)
    #undef  HE_CPU_ARM_32
    #define HE_CPU_ARM_32           1
#elif defined(_M_ARM64) || defined(__aarch64__)
    #undef  HE_CPU_ARM_64
    #define HE_CPU_ARM_64           1
#elif defined(__wasm32)
    #undef  HE_CPU_WASM_32
    #define HE_CPU_WASM_32          1
#elif defined(__wasm64)
    #undef  HE_CPU_WASM_64
    #define HE_CPU_WASM_64          1
#elif defined(_M_IX86) || defined(__i386__)
    #undef  HE_CPU_X86_32
    #define HE_CPU_X86_32           1
#elif defined(_M_AMD64) || defined(__x86_64__)
    #undef  HE_CPU_X86_64
    #define HE_CPU_X86_64           1
#endif

#if HE_CPU_ARM_32 || HE_CPU_ARM_64
    #undef  HE_CPU_ARM
    #define HE_CPU_ARM              1
#elif HE_CPU_WASM_32 || HE_CPU_WASM_64
    #undef  HE_CPU_WASM
    #define HE_CPU_WASM             1
#elif HE_CPU_X86_32 || HE_CPU_X86_64
    #undef  HE_CPU_X86
    #define HE_CPU_X86              1
#else
    #error "Unsupported processor"
#endif

#if HE_CPU_ARM_64 || HE_CPU_WASM_64 || HE_CPU_X86_64
    #undef  HE_CPU_64_BIT
    #define HE_CPU_64_BIT           1
#endif

namespace he
{
    enum class CpuVendorId
    {
        Unknown,

        Intel,      ///< Intel Corporation

        // x86 chip vendors
        AMD,        ///< Advanced Micro Devices
        Cyrix,      ///< Cyrix Corporation
        VIA,        ///< VIA Technologies
        Transmeta,  ///< Transmeta Corporation
        NSC,        ///< National Semiconductor
        SiS,        ///< Silicon Integrated Systems
        RDC,        ///< RDC Semiconductors
        NexGen,     ///< NexGen Semiconductors
        UMC,        ///< United Microelectronics Corporation
        Hygon,      ///< Hygon (chinese AMD chip vendor)
        Zhaoxin,    ///< Shanghai Zhaoxin Semiconductor (chinese VIA chip vendor)
        Elbrus,     ///< Эльбрус (russian chip vendor for aerospace)

        // x86 hypervisor vendors
        KVM,        ///< Kernel-based Virtual Machine
        MSVM,       ///< Microsoft Hyper-V or Windows Virtual PC
        VMware,     ///< VMware hypervisor
        XenHVM,     ///< Xen - type-1 hypervisor
        Bhyve,      ///< Bhyve - type-2 hypervisor
        QEMU,       ///< QEMU - OSS hypervisor
        Parallels,  ///< Parallels - virtualization on macOS
        QNX,        ///< QNX - commercial micro-OS for embedded systems
        ACRN,       ///< ACRN - hypervisor for IoT devices

        // ARM Vendors
        Ampere,     // Ampere Computing
        Arm,        // Arm Limited
        Broadcom,   // Broadcom Corporation
        Cavium,     // Cavium Inc.
        DEC,        // Digital Equipment Corporation
        Fujitsu,    // Fujitsu Ltd.
        Infineon,   // Infineon Technologies AG
        Motorola,   // Motorola or Freescale Semiconductor Inc.
        Nvidia,     // NVIDIA Corporation
        AMCC,       // Applied Micro Circuits Corporation
        Qualcomm,   // Qualcomm Inc.
        Marvell,    // Marvell International Ltd.
    };

    struct CpuInfo
    {
        CpuVendorId vendorId{ CpuVendorId::Unknown };
        char vendorName[64]{};

        uint32_t coreCount{ 1 };
        uint32_t threadCount{ 1 };

        // x86 instruction support
        struct
        {
            bool sse : 1{ false };
            bool sse2 : 1{ false };
            bool sse3 : 1{ false };
            bool ssse3 : 1{ false };
            bool sse41 : 1{ false };
            bool sse42 : 1{ false };
            bool avx : 1{ false };
            bool avx2 : 1{ false };
            bool popcnt : 1{ false };
            bool fma3 : 1{ false };
            bool aesni : 1{ false };
            bool sha : 1{ false };
            bool rdrand : 1{ false };
            bool osxsave : 1{ false };
            bool hypervisor : 1{ false };
        } x86;

        // arm instruction support
        struct
        {
            bool neon : 1{ false };
            bool rndr : 1{ false };
            bool aes : 1{ false };
            bool sha1 : 1{ false };
            bool sha2 : 1{ false };
            bool sha3 : 1{ false };
            bool crc32 : 1{ false };
            bool atomic : 1{ false };
        } arm;
    };

    const CpuInfo& GetCpuInfo();

    /// \ignore
    void _FillPlatformCpuInfo(CpuInfo& info);
}
