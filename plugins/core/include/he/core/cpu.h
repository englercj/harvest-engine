// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/types.h"

/// Set to one (1) when the target is any ARM platform, and zero (0) otherwise.
#define HE_CPU_ARM                  0

/// Set to one (1) when the target is 32-bit ARM platforms, and zero (0) otherwise.
#define HE_CPU_ARM_32               0

/// Set to one (1) when the target is 64-bit ARM platforms, and zero (0) otherwise.
#define HE_CPU_ARM_64               0

/// Set to one (1) when the target is any WASM platform, and zero (0) otherwise.
#define HE_CPU_WASM                 0

/// Set to one (1) when the target is 32-bit WASM platforms, and zero (0) otherwise.
#define HE_CPU_WASM_32              0

/// Set to one (1) when the target is 64-bit WASM platforms, and zero (0) otherwise.
#define HE_CPU_WASM_64              0

/// Set to one (1) when the target is any x86 platform, and zero (0) otherwise.
#define HE_CPU_X86                  0

/// Set to one (1) when the target is 32-bit x86 platforms, and zero (0) otherwise.
#define HE_CPU_X86_32               0

/// Set to one (1) when the target is 64-bit x86 platforms, and zero (0) otherwise.
#define HE_CPU_X86_64               0

/// Set to one (1) when the target is any 64-bit platform, and zero (0) otherwise.
#define HE_CPU_64_BIT               0

#if defined(__BIG_ENDIAN__) || defined(__ARMEB__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    #error "Big endian systems are not supported by the Harvest engine."
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

/// \def HE_SPIN_WAIT_PAUSE
/// Causes a cpu with dynamic execution to pause the next instruction for an
/// implementation-specific amount of time. This is useful in spin-wait loops.
#if HE_COMPILER_MSVC
    #if HE_CPU_ARM
        extern "C" void __yield(void);
        #pragma intrinsic(__yield)
        #define HE_SPIN_WAIT_PAUSE()    __yield()
    #elif HE_CPU_WASM
        #error "Wasm not supported via MSVC yet"
    #elif HE_CPU_X86
        extern "C" void _mm_pause(void);
        #pragma intrinsic(_mm_pause)
        #define HE_SPIN_WAIT_PAUSE()    _mm_pause()
    #endif
#else
    #if HE_CPU_ARM
        #define HE_SPIN_WAIT_PAUSE()    asm volatile("yield;" ::: "memory")
    #elif HE_CPU_WASM
        #define HE_SPIN_WAIT_PAUSE()    _mm_pause()
    #elif HE_CPU_X86
        #define HE_SPIN_WAIT_PAUSE()    asm volatile("pause;" ::: "memory")
    #endif
#endif

namespace he
{
    /// Normalized vendor identifier for CPU detection.
    enum class CpuVendorId : uint32_t
    {
        Unknown,    ///< Unknown vendor ID

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
        Ampere,     ///< Ampere Computing
        Arm,        ///< Arm Limited
        Broadcom,   ///< Broadcom Corporation
        Cavium,     ///< Cavium Inc.
        DEC,        ///< Digital Equipment Corporation
        Fujitsu,    ///< Fujitsu Ltd.
        Infineon,   ///< Infineon Technologies AG
        Motorola,   ///< Motorola or Freescale Semiconductor Inc.
        Nvidia,     ///< NVIDIA Corporation
        AMCC,       ///< Applied Micro Circuits Corporation
        Qualcomm,   ///< Qualcomm Inc.
        Marvell,    ///< Marvell International Ltd.
    };

    /// Information about the CPU vendor and available features.
    struct CpuInfo
    {
        /// The normalized vendor identifier that was detected. If the vendor was not recognized
        /// the value is \ref CpuVendorId::Unknown.
        CpuVendorId vendorId{ CpuVendorId::Unknown };

        /// The name of the vendor as reported by the CPU itself.
        char vendorName[64]{};

        /// Number of physical cores the system reported for the CPU.
        uint32_t coreCount{ 1 };

        /// Number of logical cores the system reported for the CPU.
        uint32_t threadCount{ 1 };

        /// Availability of various x86 instructions
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
            bool tscInvariant : 1{ false };
        } x86;

        /// Availability of various ARM instructions
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

    /// Returns information about the CPU vendor and available features.
    /// \see CpuInfo
    ///
    /// \return The discovered CPU information.
    const CpuInfo& GetCpuInfo();
}
