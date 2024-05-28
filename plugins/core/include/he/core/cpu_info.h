// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

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

        /// The name of the vendor as reported by the CPU.
        char vendorName[48]{};

        /// Number of physical cores the system reported for the CPU.
        uint32_t coreCount{ 1 };

        /// Number of logical cores the system reported for the CPU.
        uint32_t threadCount{ 1 };

        /// Size (in bytes) of a single cache line of the CPU.
        uint32_t cacheLineSize{ 64 };

        /// Availability of various x86 instructions
        struct
        {
            /// The brand of the CPU.
            char brandName[64]{};

            /// A product revision number assigned to the CPU.
            uint8_t steppingId{ 0 };

            /// Model ID of the CPU.
            uint16_t modelId{ 0 };

            /// Family ID of the CPU.
            uint16_t familyId{ 0 };

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
            bool sha1 : 1{ false };
            bool sha256 : 1{ false };
            bool sha512 : 1{ false };
            bool sm3 : 1{ false };
            bool sm4 : 1{ false };
            bool rdrand : 1{ false };
            bool osxsave : 1{ false };
            bool hypervisor : 1{ false };
            bool tscInvariant : 1{ false };
        } x86;

        /// Availability of various ARM instructions
        struct
        {
            /// Advanced SIMD (NEON) instructions are available.
            bool neon : 1{ false };

            /// AES instructions are available.
            /// AESE, AESD, AESMC, AESIMC
            bool aes : 1{ false };

            /// SHA1 instructions are available.
            /// SHA1C, SHA1P, SHA1M, SHA1H, SHA1SU0, SHA1SU1.
            bool sha1 : 1{ false };

            /// SHA2 (256) instructions are available.
            /// SHA256H, SHA256H2, SHA256SU0, SHA256SU1.
            bool sha256 : 1{ false };

            /// SHA2 (512) instructions are available.
            /// SHA512H, SHA512H2, SHA512SU0, SHA512SU1
            bool sha512 : 1{ false };

            /// SHA3 instructions are available.
            /// EOR3, RAX1, XAR, BCAX
            bool sha3 : 1{ false };

            /// CRC32 instructions are available.
            /// CRC32B, CRC32H, CRC32W, CRC32X, CRC32CB, CRC32CH, CRC32CW, CRC32CX
            bool crc32 : 1{ false };

            /// Atomic instructions are available.
            /// LDADD, LDCLR, LDEOR, LDSET, LDSMAX, LDSMIN, LDUMAX, LDUMIN, CAS, CASP, SWP
            bool atomic : 1{ false };

            /// 128-bit atomic instructions are available.
            /// LDCLRP, LDSETP, SWPP
            bool atomic128 : 1{ false };

            /// RDMA instructions are available.
            /// SQRDMLAH, SQRDMLSH
            bool rdma : 1{ false };

            /// SM3 instructions are available.
            /// SM3SS1, SM3TT1A, SM3TT1B, SM3TT2A, SM3TT2B, SM3PARTW1, SM3PARTW2
            bool sm3 : 1{ false };

            /// SM4 instructions are available.
            /// SM4E, SM4EKEY
            bool sm4 : 1{ false };

            /// Dot Product instructions are available.
            /// UDOT, SDOT
            bool dp : 1{ false };

            /// FHM instructions are available.
            /// FMLAL, FMLSL
            bool fhm : 1{ false };

            /// Random Number registers are available.
            /// RNDR, RNDRRS
            bool rndr : 1{ false };
        } arm;
    };

    /// Returns information about the CPU vendor and available features.
    /// \see CpuInfo
    ///
    /// \return The discovered CPU information.
    const CpuInfo& GetCpuInfo();
}
