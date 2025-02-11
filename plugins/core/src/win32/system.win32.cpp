// Copyright Chad Engler

#include "he/core/system.h"

#include "he/core/assert.h"
#include "he/core/wstr.h"

#if defined(HE_PLATFORM_API_WIN32)

#include <Windows.h>
#include <Lmcons.h>

namespace he
{
    using Pfn_RtlGetVersion = NTSTATUS(NTAPI*)(RTL_OSVERSIONINFOW*);

    static const char* GetWindowsName(const RTL_OSVERSIONINFOEXW& osvi)
    {
        switch (osvi.dwMajorVersion)
        {
            case 6:
            {
                switch (osvi.dwMinorVersion)
                {
                    case 0: return "Windows Vista";
                    case 1: return "Windows 7";
                    case 2: return "Windows 8";
                    case 3: return "Windows 8.1";
                    case 4: return "Windows 10 Beta"; // some early Win10 previews used "6.4"
                }
                break;
            }
            case 10:
            {
                if (osvi.dwMinorVersion == 0)
                {
                    if (osvi.dwBuildNumber < 22000)
                    {
                        return "Windows 10";
                    }

                    return "Windows 11";
                }
                break;
            }
        }

        return "Windows";
    }

    static const char* GetWindowsServerName(const RTL_OSVERSIONINFOEXW& osvi)
    {
        switch (osvi.dwMajorVersion)
        {
            case 6:
            {
                switch (osvi.dwMinorVersion)
                {
                    case 0: return "Windows Server 2008";
                    case 1: return "Windows Server 2008 R2";
                    case 2: return "Windows Server 2012";
                    case 3: return "Windows Server 2012 R2";
                }
                break;
            }
            case 10:
            {
                if (osvi.dwMinorVersion == 0)
                {
                    if (osvi.dwBuildNumber <= 14393)
                    {
                        return "Windows Server 2016";
                    }

                    if (osvi.dwBuildNumber <= 17763)
                    {
                        return "Windows Server 2019";
                    }

                    if (osvi.dwBuildNumber <= 19042)
                    {
                        return "Windows Server 20H2";
                    }

                    return "Windows Server 2022";
                }
                break;
            }
        }

        return "Windows Server";
    }

    struct SystemInfoImpl : SystemInfo
    {
        SystemInfoImpl()
        {
            SYSTEM_INFO sysInfo{};
            ::GetSystemInfo(&sysInfo);
            allocationGranularity = sysInfo.dwAllocationGranularity;
            pageSize = sysInfo.dwPageSize;

            RTL_OSVERSIONINFOEXW osvi{};
            osvi.dwOSVersionInfoSize = sizeof(osvi);

            HMODULE ntdll = ::GetModuleHandleW(L"ntdll.dll");
            Pfn_RtlGetVersion pfnRtlGetVersion = reinterpret_cast<Pfn_RtlGetVersion>(::GetProcAddress(ntdll, "RtlGetVersion"));

            if (pfnRtlGetVersion)
            {
                pfnRtlGetVersion(reinterpret_cast<OSVERSIONINFOW*>(&osvi));
            }
            else
            {
                #pragma warning(suppress:4996)
                ::GetVersionExW(reinterpret_cast<OSVERSIONINFOW*>(&osvi));
            }

            HE_ASSERT(osvi.dwPlatformId == VER_PLATFORM_WIN32_NT);

            version.major = osvi.dwMajorVersion;
            version.minor = osvi.dwMinorVersion;
            version.patch = osvi.wServicePackMajor;
            version.build = osvi.dwBuildNumber;

            const bool isWindowsServer = osvi.wProductType != VER_NT_WORKSTATION;
            platform = isWindowsServer ? GetWindowsServerName(osvi) : GetWindowsName(osvi);
        }
    };

    const SystemInfo& GetSystemInfo()
    {
        static SystemInfoImpl s_info{};
        return s_info;
    }

    Result GetSystemName(String& outName)
    {
        wchar_t nameBuf[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD nameBufLen = HE_LENGTH_OF(nameBuf);

        if (!::GetComputerNameW(nameBuf, &nameBufLen))
        {
            return Result::FromLastError();
        }

        WCToMBStr(outName, nameBuf);
        return Result::Success;
    }

    Result GetSystemUserName(String& outName)
    {
        wchar_t nameBuf[UNLEN + 1];
        DWORD nameBufLen = HE_LENGTH_OF(nameBuf);

        if (!::GetUserNameW(nameBuf, &nameBufLen))
        {
            return Result::FromLastError();
        }

        WCToMBStr(outName, nameBuf);
        return Result::Success;
    }

    PowerStatus GetPowerStatus()
    {
        PowerStatus status{};

        SYSTEM_POWER_STATUS pwrStatus{};
        if (::GetSystemPowerStatus(&pwrStatus))
        {
            if (pwrStatus.ACLineStatus != AC_LINE_UNKNOWN)
            {
                status.onACPower = pwrStatus.ACLineStatus == AC_LINE_ONLINE;
            }

            if (pwrStatus.BatteryFlag != BATTERY_FLAG_UNKNOWN)
            {
                status.hasBattery = pwrStatus.BatteryFlag != BATTERY_FLAG_NO_BATTERY;
            }

            if (pwrStatus.BatteryLifePercent != BATTERY_FLAG_UNKNOWN)
            {
                status.batteryLife = pwrStatus.BatteryLifePercent;
            }

            if (pwrStatus.BatteryLifeTime != BATTERY_LIFE_UNKNOWN)
            {
                status.batteryLifeTime = FromPeriod<Seconds>(pwrStatus.BatteryLifeTime);
            }
        }

        return status;
    }

    void* DLOpen(const char* path)
    {
        return static_cast<void*>(::LoadLibraryW(HE_TO_WCSTR(path)));
    }

    void DLClose(void* handle)
    {
        ::FreeLibrary(static_cast<HMODULE>(handle));
    }

    void* DLSymbol(void* handle, const char* symbol)
    {
        return static_cast<void*>(::GetProcAddress(static_cast<HMODULE>(handle), symbol));
    }
}

#endif
