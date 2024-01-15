// Copyright Chad Engler

#include "he/core/wstr.h"

#include <cwchar>

namespace he
{
    int32_t WCStrCmp(const wchar_t* a, const wchar_t* b)
    {
        return std::wcscmp(a, b);
    }
}
