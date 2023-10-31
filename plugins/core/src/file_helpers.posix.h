// Copyright Chad Engler

#pragma once

#include "he/core/file.h"
#include "he/core/string.h"

#if defined(HE_PLATFORM_API_POSIX) && !defined(HE_PLATFORM_EMSCRIPTEN)

struct stat;

namespace he
{
    int PosixFileOpen(const char* path, FileOpenMode mode, FileOpenFlag flags, int extraFlags);

    void PosixFileGatherAttributes(const stat& sb, const char* path, FileAttributes& attribs);
    Result PosixFileGetAttributes(int fd, FileAttributes& outAttributes);
    Result PosixFileGetPath(int fd, String& outPath);

    Result PosixReadLink(const char* linkPath, String& outPath);
}

#endif
