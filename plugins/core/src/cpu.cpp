// Copyright Chad Engler

#include "he/core/cpu.h"

#include "he/core/enum_ops.h"

namespace he
{
    template <>
    const char* AsString(CpuVendorId x)
    {
        switch (x)
        {
            case CpuVendorId::Unknown: break;
            case CpuVendorId::Intel: return "Intel";
            case CpuVendorId::AMD: return "AMD";
            case CpuVendorId::Cyrix: return "Cyrix";
            case CpuVendorId::VIA: return "VIA";
            case CpuVendorId::Transmeta: return "Transmeta";
            case CpuVendorId::NSC: return "NSC";
            case CpuVendorId::SiS: return "SiS";
            case CpuVendorId::RDC: return "RDC";
            case CpuVendorId::NexGen: return "NexGen";
            case CpuVendorId::UMC: return "UMC";
            case CpuVendorId::Hygon: return "Hygon";
            case CpuVendorId::Zhaoxin: return "Zhaoxin";
            case CpuVendorId::Elbrus: return "Elbrus";
            case CpuVendorId::KVM: return "KVM";
            case CpuVendorId::MSVM: return "MSVM";
            case CpuVendorId::VMware: return "VMware";
            case CpuVendorId::XenHVM: return "XenHVM";
            case CpuVendorId::Bhyve: return "Bhyve";
            case CpuVendorId::QEMU: return "QEMU";
            case CpuVendorId::Parallels: return "Parallels";
            case CpuVendorId::QNX: return "QNX";
            case CpuVendorId::ACRN: return "ACRN";
            case CpuVendorId::Ampere: return "Ampere";
            case CpuVendorId::Arm: return "Arm";
            case CpuVendorId::Broadcom: return "Broadcom";
            case CpuVendorId::Cavium: return "Cavium";
            case CpuVendorId::DEC: return "DEC";
            case CpuVendorId::Fujitsu: return "Fujitsu";
            case CpuVendorId::Infineon: return "Infineon";
            case CpuVendorId::Motorola: return "Motorola";
            case CpuVendorId::Nvidia: return "Nvidia";
            case CpuVendorId::AMCC: return "AMCC";
            case CpuVendorId::Qualcomm: return "Qualcomm";
            case CpuVendorId::Marvell: return "Marvell";
        }

        return "<unknown>";
    }

}
