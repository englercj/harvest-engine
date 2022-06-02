// Copyright Chad Engler

#include "he/core/process.h"

#include "he/core/alloca.h"
#include "he/core/allocator.h"
#include "he/core/wstr.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

namespace he
{
    constexpr uint32_t MaxStackLen = 2048;

    String GetEnv(const char* name)
    {
        const wchar_t* wideName = HE_TO_WSTR(name);
        const uint32_t requiredLen = ::GetEnvironmentVariableW(wideName, nullptr, 0);

        wchar_t* wideValue = Allocator::GetDefault().Malloc<wchar_t>(requiredLen);
        const uint32_t writtenLen = ::GetEnvironmentVariableW(wideName, wideValue, requiredLen);
        if (writtenLen == 0 || writtenLen >= requiredLen)
        {
            wideValue[0] = L'\0';
        }

        String value;
        WCToMBStr(value, wideValue);
        Allocator::GetDefault().Free(wideValue);
        return value;
    }

    Result SetEnv(const char* name, const char* value)
    {
        const wchar_t* wideName = HE_TO_WSTR(name);
        const wchar_t* wideValue = value ? HE_TO_WSTR(value) : nullptr;

        if (!::SetEnvironmentVariableW(wideName, wideValue))
            return Result::FromLastError();

        return Result::Success;
    }

    uint32_t GetCurrentProcessId()
    {
        return ::GetProcessId(::GetCurrentProcess());
    }

    bool IsProcessRunning(uint32_t pid)
    {
        const HANDLE process = ::OpenProcess(SYNCHRONIZE, FALSE, pid);
        const DWORD ret = ::WaitForSingleObject(process, 0);
        ::CloseHandle(process);
        return ret == WAIT_TIMEOUT;
    }
}

#endif
