// Copyright Chad Engler

#include "he/core/system.h"

#include "he/core/macros.h"

#if defined(HE_PLATFORM_API_WASM)

#include "he/core/wasm/lib_core.wasm.h"

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

        double chargingTime = 0;
        double dischargingTime = 0;
        double level = 0;
        bool charging = false;
        if (heWASM_GetBatteryStatus(&chargingTime, &dischargingTime, &level, &charging))
        {
            // See: https://www.w3.org/TR/battery-status/#idl-def-batterymanager
            bool hasNoBattery = charging == true
                && chargingTime == 0
                && dischargingTime == Limits<double>::Infinity
                && level == 1.0;

            status.onACPower = charging;
            status.hasBattery = !hasNoBattery;

            if (level >= 0.0 && level <= 1.0)
            {
                status.batteryLife = static_cast<uint8_t>(level * 100.0);
            }

            if (dischargingTime != Limits<double>::Infinity)
            {
                status.batteryLifeTime = FromPeriod<Seconds>(dischargingTime);
            }
        }

        return status;
    }

    void* DLOpen([[maybe_unused]] const char* path)
    {
        return nullptr;
    }

    void DLClose([[maybe_unused]] void* handle)
    {
    }

    void* DLSymbol([[maybe_unused]] void* handle, [[maybe_unused]] const char* symbol)
    {
        return nullptr;
    }
}

#endif
