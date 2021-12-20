// Copyright Chad Engler

#pragma once

#include "he/core/file.h"

#if defined(HE_PLATFORM_API_POSIX) && !defined(HE_PLATFORM_EMSCRIPTEN)

namespace he
{
    int PosixFileOpen(const char* path, FileOpenMode mode, FileOpenFlag flags, int extraFlags);
}

#endif
