// Copyright Chad Engler

#include "he/core/system.h"

#include "he/core/macros.h"

#if defined(HE_PLATFORM_EMSCRIPTEN)

#include <emscripten/html5.h>

namespace he
{
    struct SystemInfoImpl : SystemInfo
    {
        SystemInfoImpl()
        {
            // wasm has a constant page size of 64KiB
            allocationGranularity = 65536;
            pageSize = 65536;
            platform = "Emscripten";
            version.major = __EMSCRIPTEN_major__;
            version.minor = __EMSCRIPTEN_minor__;
            version.patch = __EMSCRIPTEN_tiny__;
        }
    };

    const SystemInfo& GetSystemInfo()
    {
        static SystemInfoImpl s_info{};
        return s_info;
    }

    Result GetSystemName(String& outName)
    {
        outName = "Emscripten";
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

        EmscriptenBatteryEvent batteryStatus;
        EMSCRIPTEN_RESULT result = emscripten_get_battery_status(&batteryStatus);

        if (result == EMSCRIPTEN_RESULT_SUCCESS)
        {
            // See: https://www.w3.org/TR/battery-status/#idl-def-batterymanager
            bool hasNoBattery = batteryStatus.charging == true
                && batteryStatus.chargingTime == 0
                && batteryStatus.dischargingTime == std::numeric_limits<double>::infinity()
                && batteryStatus.level == 1.0;

            status.onACPower.Set(batteryStatus.charging);
            status.hasBattery.Set(!hasNoBattery);

            if (batteryStatus.level >= 0.0 && batteryStatus.level <= 1.0)
                status.batteryLife.Set(static_cast<uint8_t>(batteryStatus.level * 100.0));

            if (batteryStatus.dischargingTime != std::numeric_limits<double>::infinity())
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
