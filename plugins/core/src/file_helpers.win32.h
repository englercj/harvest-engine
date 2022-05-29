// Copyright Chad Engler

#pragma once

#include "he/core/file.h"
#include "he/core/string.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

namespace he
{
    constexpr intptr_t Win32InvalidFd = reinterpret_cast<intptr_t>(INVALID_HANDLE_VALUE);

    HANDLE Win32FileOpen(const char* path, FileOpenMode mode, FileOpenFlag flags, DWORD extraFlags);

    Result Win32FileGetAttributes(HANDLE handle, FileAttributes& outAttributes);
    Result Win32FileGetPath(HANDLE handle, String& outPath);
}

#endif
