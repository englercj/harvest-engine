// Copyright Chad Engler

#include "he/core/cpu_info.h"

#include "he/core/compiler.h"
#include "he/core/cpu.h"
#include "he/core/string.h"

#if HE_CPU_ARM

#include "cpu_registers.arm.h"

namespace he
{
    void _FillPlatformCpuInfo(CpuInfo& info);

    struct CpuInfoImpl : CpuInfo
    {
        CpuInfoImpl()
        {
        #if HE_CPU_ARM_64 || defined(__ARM_NEON__) || defined(__ARM_NEON)
            // AArch64 includes neon
            arm.neon = true;
        #endif

            // https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/ID-AA64ISAR0-EL1--AArch64-Instruction-Set-Attribute-Register-0
            // https://developer.arm.com/documentation/ddi0595/2021-06/AArch64-Registers/MIDR-EL1--Main-ID-Register
            uint64_t aa64isar0;
            uint64_t midr;
        #if HE_COMPILER_MSVC
            aa64isar0 = static_cast<uint64_t>(_ReadStatusReg(ARM64_ID_AA64ISAR0_EL1));
            midr = static_cast<uint64_t>(_ReadStatusReg(ARM64_MIDR_EL1));
        #else
            asm volatile("mrs %0, ID_AA64ISAR0_EL1" : "=r"(aa64isar0));
            asm volatile("mrs %0, MIDR_EL1" : "=r"(midr));
        #endif

            arm.rndr = ((aa64isar0 >> 60) & 0xf) != 0;
            arm.aes = ((aa64isar0 >> 4) & 0xf) != 0;
            arm.sha1 = ((aa64isar0 >> 8) & 0xf) != 0;
            arm.sha2 = arm.sha1 && ((aa64isar0 >> 12) & 0xf) != 0;
            arm.sha3 = arm.sha1 && ((aa64isar0 >> 12) & 0xf) == 2 && (aa64isar0 >> 32) & 0xf) != 0;
            arm.crc32 = ((aa64isar0 >> 16) & 0xf) != 0;
            arm.atomic = ((aa64isar0 >> 20) & 0xf) != 0;

            const uint64_t vendor = (midr >> 24) & 0xff;
            switch (vendor)
            {
                case 0xC0:
                    String::Copy(vendorName, "Ampere Computing");
                    vendorId = CpuVendorId::Ampere;
                    break;
                case 0x41:
                    String::Copy(vendorName, "Arm Limited");
                    vendorId = CpuVendorId::ARM;
                    break;
                case 0x42:
                    String::Copy(vendorName, "Broadcom Corporation");
                    vendorId = CpuVendorId::Broadcom;
                    break;
                case 0x43:
                    String::Copy(vendorName, "Cavium Inc");
                    vendorId = CpuVendorId::Cavium;
                    break;
                case 0x44:
                    String::Copy(vendorName, "Digital Equipment Corporation");
                    vendorId = CpuVendorId::DEC;
                    break;
                case 0x46:
                    String::Copy(vendorName, "Fujitsu Ltd");
                    vendorId = CpuVendorId::Fujitsu;
                    break;
                case 0x49:
                    String::Copy(vendorName, "Infineon Technologies AG");
                    vendorId = CpuVendorId::Infineon;
                    break;
                case 0x4D:
                    String::Copy(vendorName, "Motorola or Freescale Semiconductor Inc");
                    vendorId = CpuVendorId::Motorola;
                    break;
                case 0x4E:
                    String::Copy(vendorName, "NVIDIA Corporation");
                    vendorId = CpuVendorId::NVIDIA;
                    break;
                case 0x50:
                    String::Copy(vendorName, "Applied Micro Circuits Corporation");
                    vendorId = CpuVendorId::AMCC;
                    break;
                case 0x51:
                    String::Copy(vendorName, "Qualcomm Inc");
                    vendorId = CpuVendorId::Qualcomm;
                    break;
                case 0x56:
                    String::Copy(vendorName, "Marvell International Ltd");
                    vendorId = CpuVendorId::Marvell;
                    break;
                case 0x69:
                    String::Copy(vendorName, "Intel Corporation");
                    vendorId = CpuVendorId::Intel;
                    break;
                default:
                    String::Copy(vendorName, "Unknown");
                    vendorId = CpuVendorId::Unknown;
                    break;
            }
        }
    };

    const CpuInfo& GetCpuInfo()
    {
        static CpuInfoImpl s_info;
        return s_info;
    }
}

#endif
