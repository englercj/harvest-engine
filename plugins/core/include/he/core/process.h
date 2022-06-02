// Copyright Chad Engler

#pragma once

#include "he/core/result.h"
#include "he/core/string.h"
#include "he/core/types.h"

namespace he
{
    String GetEnv(const char* name);

    // Set value to nullptr to clear it
    Result SetEnv(const char* name, const char* value);

    uint32_t GetCurrentProcessId();
    bool IsProcessRunning(uint32_t pid);
}
