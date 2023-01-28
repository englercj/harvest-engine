// Copyright Chad Engler

#include "he/core/system.h"
#include "he/core/system_fmt.h"

#include "he/core/fmt.h"
#include "he/core/result_fmt.h"
#include "he/core/test.h"

#include <iostream>

using namespace he;

#if defined(HE_PLATFORM_API_WIN32)
    constexpr char TestLibName[] = "Kernel32.dll";
    constexpr char TestSymbol[] = "VirtualAlloc";
#else
    constexpr char TestLibName[] = "libdl.so";
    constexpr char TestSymbol[] = "dlopen";
#endif

// ------------------------------------------------------------------------------------------------
HE_TEST(core, system, GetSystemInfo)
{
    const SystemInfo& info = GetSystemInfo();

    std::cout << "    allocationGranularity = " << info.allocationGranularity << std::endl;
    std::cout << "    pageSize = " << info.pageSize << std::endl;
    std::cout << "    platform = " << info.platform.Data() << std::endl;
    std::cout << "    version = " << info.version.major << '.' << info.version.minor << '.' << info.version.patch << '.' << info.version.build << std::endl;
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, system, SystemInfo_Fmt)
{
    const SystemInfo& info = GetSystemInfo();

    const String infoStr = Format("{}", info);
    std::cout << "    System Info = " << infoStr.Data() << std::endl;
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, system, GetSystemName)
{
    String name;
    Result r = GetSystemName(name);
    HE_EXPECT(r, r);
    HE_EXPECT(!name.IsEmpty());

    std::cout << "    System Name = " << name.Data() << std::endl;
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, system, GetSystemUserName)
{
    String name;
    Result r = GetSystemUserName(name);
    HE_EXPECT(r, r);
    HE_EXPECT(!name.IsEmpty());

    std::cout << "    User Name = " << name.Data() << std::endl;
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, system, GetPowerStatus)
{
    PowerStatus status = GetPowerStatus();

    std::cout << "    onACPower = [" << (status.onACPower.valid ? "valid" : "invalid") << "] " << status.onACPower.value << std::endl;
    std::cout << "    hasBattery = [" << (status.hasBattery.valid ? "valid" : "invalid") << "] " << status.hasBattery.value << std::endl;
    std::cout << "    batteryLife = [" << (status.batteryLife.valid ? "valid" : "invalid") << "] " << static_cast<uint32_t>(status.batteryLife.value) << std::endl;
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, system, DLOpen_DLClose)
{
    void* handle = DLOpen(TestLibName);
    HE_EXPECT_NE(handle, nullptr);

    void* symbol = DLSymbol(handle, TestSymbol);
    HE_EXPECT_NE(symbol, nullptr);

    DLClose(handle);
}

// ------------------------------------------------------------------------------------------------
HE_TEST(core, system, DynamicLib)
{
    DynamicLib lib;
    HE_EXPECT(!lib.IsOpen());

    lib.Open(TestLibName);
    HE_EXPECT(lib.IsOpen());

    void* symbol = lib.Symbol(TestSymbol);
    HE_EXPECT(lib.IsOpen());
    HE_EXPECT_NE(symbol, nullptr);

    lib.Close();
}
