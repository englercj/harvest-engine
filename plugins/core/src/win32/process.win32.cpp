// Copyright Chad Engler

#include "he/core/process.h"

#include "he/core/alloca.h"
#include "he/core/allocator.h"
#include "he/core/wstr.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

namespace he
{
    Result GetEnv(const char* name, String& outValue)
    {
        const wchar_t* wideName = HE_TO_WCSTR(name);
        const uint32_t requiredLen = ::GetEnvironmentVariableW(wideName, nullptr, 0);
        if (requiredLen == 0)
        {
            outValue.Clear();
            return Result::FromLastError();
        }

        wchar_t* wideValue = HE_ALLOCA(wchar_t, requiredLen);
        const uint32_t writtenLen = ::GetEnvironmentVariableW(wideName, wideValue, requiredLen);
        if (writtenLen == 0)
        {
            outValue.Clear();
            return Result::FromLastError();
        }

        // Try and catch a case where the env var changed and got longer between
        // the call to get the size and the call to copy the value.
        if (writtenLen >= requiredLen)
        {
            outValue.Clear();
            return Win32Result(ERROR_ENVVAR_NOT_FOUND);
        }

        WCToMBStr(outValue, wideValue);
        return Result::Success;
    }

    Result SetEnv(const char* name, const char* value)
    {
        const wchar_t* wideName = HE_TO_WCSTR(name);
        const wchar_t* wideValue = value ? HE_TO_WCSTR(value) : nullptr;

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
        const DWORD r = ::WaitForSingleObject(process, 0);
        ::CloseHandle(process);
        return r == WAIT_TIMEOUT;
    }

    Result GetCurrentProcessFilename(String& out)
    {
        wchar_t wcPath[MAX_PATH];
        DWORD rc = ::GetModuleFileNameW(NULL, wcPath, MAX_PATH);

        if (rc == 0 || rc == MAX_PATH)
        {
            // TODO: Handle ERROR_INSUFFICIENT_BUFFER and increase buffer size accordingly.
            out.Clear();
            return Result::FromLastError();
        }

        WCToMBStr(out, wcPath, rc);
        return Result::Success;
    }
}

#endif
