// Copyright Chad Engler

#include "he/core/system.h"

#include "he/core/file.h"
#include "he/core/macros.h"
#include "he/core/scope_guard.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/utils.h"

#include <concepts>
#include <type_traits>

#if defined(HE_PLATFORM_API_POSIX)

#include <dlfcn.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/utsname.h>

namespace he
{
    static uint32_t ReadFile(char* dst, uint32_t len, const char* path)
    {
        File file;
        if (!file.Open(path, FileOpenMode::ReadExisting))
            return 0;

        uint32_t bytesRead;
        if (!file.ReadAt(dst, 0, len, &bytesRead) || bytesRead == 0)
            return 0;

        return bytesRead;
    };

    template <uint32_t N>
    static uint32_t ReadFile(char (&dst)[N], const char* path)
    {
        return ReadFile(dst, N, path);
    }

    template <typename T>
    static void GetPowerSupplyValue(const char* data, uint32_t dataLen, StringView key, PowerStatus::Value<T>& value)
    {
        const char* p = String::Find(key);
        if (!p)
            return;

        const char* dataEnd = data + dataLen;
        const char* valueStart = p + keyLen;

        if (*valueStart == '=')
            ++valueStart;

        if (valueStart >= dataEnd)
            return;

        const char* valueEnd = valueStart;
        while (valueEnd < dataEnd && *valueEnd != '\n')
            ++valueEnd;

        if constexpr (std::is_same_v<T, bool>)
        {
            value.Set(valueStart[0] != '0');
        }
        else if constexpr (std::is_integral_v<T>)
        {
            value.Set(String::ToInteger<T>(valueStart, valueEnd));
        }
        else if constexpr (std::is_floating_point_v<T>)
        {
            value.Set(String::ToFloat<T>(valueStart, valueEnd));
        }
    }

    struct SystemInfoImpl : SystemInfo
    {
        SystemInfoImpl()
        {
            // Allocation granularity can sometimes actually be smaller than a page size, but I
            // can't find any examples of where it is larger than a page.
            allocationGranularity = static_cast<uint32_t>(sysconf(_SC_PAGE_SIZE));
            pageSize = static_cast<uint32_t>(sysconf(_SC_PAGE_SIZE));

            utsname data;
            const int rc = uname(&data);
            HE_UNUSED(rc);
            HE_ASSERT(rc == 0);

            platform = data.sysname;

            const char* major = data.release;
            const char* majorEnd = String::Find(major, '.');
            version.major = String::ToInteger<uint32_t>(major, majorEnd);

            const char* minor = majorEnd + 1;
            const char* minorEnd = String::Find(minor, '.');
            version.minor = String::ToInteger<uint32_t>(minor, minorEnd);

            const char* patch = minorEnd + 1;
            version.patch = String::ToInteger<uint32_t>(patch);
        }
    };

    const SystemInfo& GetSystemInfo()
    {
        static SystemInfoImpl s_info;
        return s_info;
    }

    String GetSystemName(Allocator& allocator)
    {
        String name(allocator);
        name.Resize(String::MaxEmbedCharacters, he::DefaultInit);

        do
        {
            const int rc = gethostname(name.Data(), name.Size());

            if (rc < 0)
            {
                name.Clear();
                break;
            }

            const uint32_t len = String::Length(name.Data());

            if (len < name.Size())
            {
                // resize to properly null terminate
                name.Resize(len);
                break;
            }

            name.Resize(Max(256u, name.Size() * 2), he::DefaultInit);
        } while (true);

        return name;
    }

    String GetSystemUserName(Allocator& allocator)
    {
        String name(allocator);

        const uid_t uid = getuid();
        const passwd* pwd = getpwuid(uid);
        if (pwd)
            name = pwd->pw_name;

        return name;
    }

    PowerStatus GetPowerStatus()
    {
        PowerStatus status{};

        constexpr StringView CapacityKey = "POWER_SUPPLY_CAPACITY";
        constexpr StringView EnergyNowKey = "POWER_SUPPLY_ENERGY_NOW";
        constexpr StringView PowerNowKey = "POWER_SUPPLY_POWER_NOW";
        constexpr StringView PresentKey = "POWER_SUPPLY_PRESENT";
        constexpr StringView OnlineKey = "POWER_SUPPLY_ONLINE";

        char buf[1024];

        // Read AC adapter info
        const uint32_t acLen = ReadFile(buf, "/sys/class/power_supply/AC0/uevent");
        if (acLen > 0)
        {
            GetPowerSupplyValue(buf, acLen, OnlineKey, status.onACPower);
        }

        // Read battery info
        const uint32_t batLen = ReadFile(buf, "/sys/class/power_supply/BAT0/uevent");
        if (batLen > 0)
        {
            GetPowerSupplyValue(buf, batLen, PresentKey, status.hasBattery);
            GetPowerSupplyValue(buf, batLen, CapacityKey, status.batteryLife);

            PowerStatus::Value<double> energyNow;
            GetPowerSupplyValue(buf, batLen, EnergyNowKey, energyNow);

            PowerStatus::Value<double> powerNow;
            GetPowerSupplyValue(buf, batLen, PowerNowKey, powerNow);

            if (energyNow.valid && powerNow.valid)
            {
                status.batterLifeTime.Set(FromPeriod<Hours>(energyNow.value / powerNow.value));
            }
        }

        // If we're on AC power, but didn't find a battery file we can be confident that
        // a battery isn't present on the system.
        if (status.onACPower.valid && status.onACPower.value && !status.hasBattery.valid)
            status.hasBattery.Set(false);

        return status;
    }

    void* DLOpen(const char* path)
    {
        return dlopen(path, RTLD_LOCAL | RTLD_LAZY);
    }

    void DLClose(void* handle)
    {
        dlclose(handle);
    }

    void* DLSymbol(void* handle, const char* symbol)
    {
        return dlsym(handle, symbol);
    }
}

#endif
