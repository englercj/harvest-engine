// Copyright Chad Engler

#include "he/core/alloca.h"
#include "he/core/assert.h"
#include "he/core/compiler.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

#include <stdlib.h>

namespace he
{
    int AppMain(int argc, char* argv[]);

    int _Win32AppMain()
    {
        int argc = __argc;
        wchar_t** wargv = __wargv;

        char** argv = new char*[argc + 1];

        for (int i = 0; i < argc; ++i)
        {
            int len = ::WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, nullptr, 0, nullptr, nullptr);

            argv[i] = new char[len];

            int copied = ::WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, argv[i], len, nullptr, nullptr);

            HE_ASSERT(len == copied);
            HE_UNUSED(copied);
        }

        argv[argc] = nullptr;

        int ret = AppMain(argc, argv);

        for (int i = 0; i < argc; ++i)
        {
            delete[] argv[i];
        }

        delete[] argv;

        return ret;
    }
}

#endif
