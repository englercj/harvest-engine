// Copyright Chad Engler

#include "he/core/system.h"

#include "he/core/file.h"
#include "he/core/macros.h"
#include "he/core/scope_guard.h"
#include "he/core/string.h"
#include "he/core/utils.h"

#include <concepts>

#if defined(HE_PLATFORM_API_POSIX)

#include <dlfcn.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/utsname.h>

namespace he
{
    static void ReadPowerStatusBool(const char* path, PowerStatus::Value<bool>& value)
    {
        File file;
        if (!file.Open(path, FileOpenMode::ReadExisting))
            return;

        char data[2];
        uint32_t bytesRead;
        if (!file.Read(data, HE_LENGTH_OF(data), &bytesRead) || bytesRead != 2)
            return;

        value.Set(data[0] != '0');
    };

    template <std::integral T>
    static void ReadPowerStatusInt(const char* path, PowerStatus::Value<T>& value)
    {
        File file;
        if (!file.Open(path, FileOpenMode::ReadExisting))
            return;

        char data[100];
        uint32_t bytesRead;
        if (!file.Read(data, HE_LENGTH_OF(data), &bytesRead) || bytesRead < 2)
            return;

        const T v = String::ToInteger<T>(data);
        value.Set(v);
    };

    template <std::floating_point T>
    static void ReadPowerStatusFloat(const char* path, PowerStatus::Value<T>& value)
    {
        File file;
        if (!file.Open(path, FileOpenMode::ReadExisting))
            return;

        char data[100];
        uint32_t bytesRead;
        if (!file.Read(data, HE_LENGTH_OF(data), &bytesRead) || bytesRead < 2)
            return;

        const T v = String::ToFloat<T>(data);
        value.Set(v);
    };

    struct SystemInfoImpl : SystemInfo
    {
        SystemInfoImpl()
        {
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

        ReadPowerStatusBool("/sys/class/power_supply/AC0/online", status.onACPower);
        ReadPowerStatusBool("/sys/class/power_supply/BAT0/present", status.hasBattery);
        ReadPowerStatusInt("/sys/class/power_supply/BAT0/capacity", status.batteryLife);

        // TODO: calculate battery life time
        // status.batteryLifeTime

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
