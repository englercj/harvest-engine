// Copyright Chad Engler

#include "he/core/system.h"

#include "he/core/macros.h"

#if defined(HE_PLATFORM_WASM)

#include "wasm/lib_core.wasm.h"

namespace he
{
    struct SystemInfoImpl : SystemInfo
    {
        SystemInfoImpl()
        {
            // wasm has a constant page size of 64KiB
            allocationGranularity = 65536;
            pageSize = 65536;

            // Use the user agent string as the platform name
            const uint32_t len = heWASM_GetUserAgentLength();
            platform.Resize(len);
            heWASM_GetUserAgent(platform.Data(), platform.Size());

            // TODO: What should the version be? Browser or OS version? Might be nice to know both?
            version.major = 0;
            version.minor = 0;
            version.patch = 0;
            version.build = 0;
        }
    };

    const SystemInfo& GetSystemInfo()
    {
        static SystemInfoImpl s_info{};
        return s_info;
    }

    Result GetSystemName(String& outName)
    {
        outName = "WASM";
        return Result::Success;
    }

    Result GetSystemUserName(String& outName)
    {
        outName = "Unknown";
        return Result::Success;
    }

    PowerStatus GetPowerStatus()
    {
        PowerStatus status;

        heWASM_BatteryStatus batteryStatus;
        if (heWASM_GetBatteryStatus(&batteryStatus))
        {
            // See: https://www.w3.org/TR/battery-status/#idl-def-batterymanager
            bool hasNoBattery = batteryStatus.charging == true
                && batteryStatus.chargingTime == 0
                && batteryStatus.dischargingTime == Limits<double>::Infinity
                && batteryStatus.level == 1.0;

            status.onACPower.Set(batteryStatus.charging);
            status.hasBattery.Set(!hasNoBattery);

            if (batteryStatus.level >= 0.0 && batteryStatus.level <= 1.0)
                status.batteryLife.Set(static_cast<uint8_t>(batteryStatus.level * 100.0));

            if (batteryStatus.dischargingTime != Limits<double>::Infinity)
                status.batteryLifeTime.Set(FromPeriod<Seconds>(batteryStatus.dischargingTime));
        }

        return status;
    }

    void* DLOpen(const char* path)
    {
        HE_UNUSED(path);
        return nullptr;
    }

    void DLClose(void* handle)
    {
        HE_UNUSED(handle);
    }

    void* DLSymbol(void* handle, const char* symbol)
    {
        HE_UNUSED(handle, symbol);
        return nullptr;
    }
}

#endif
