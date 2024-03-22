// Copyright Chad Engler

#pragma once

#include "he/core/file.h"
#include "he/core/string.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

namespace he
{
    constexpr intptr_t Win32InvalidFd = reinterpret_cast<intptr_t>(INVALID_HANDLE_VALUE);

    HANDLE Win32FileOpen(const char* path, FileAccessMode access, FileCreateMode create, FileOpenFlag flags, DWORD extraFlags);

    FileAttributeFlag Win32ParseFileAttributeFlags(DWORD dwFileAttributes);
    void Win32ParseFileAttributes(const WIN32_FILE_ATTRIBUTE_DATA& info, FileAttributes& outAttributes);
    void Win32ParseFileAttributes(const BY_HANDLE_FILE_INFORMATION& info, FileAttributes& outAttributes);

    Result Win32FileGetAttributes(HANDLE handle, FileAttributes& outAttributes);
    Result Win32FileGetPath(HANDLE handle, String& outPath);
}

#endif
