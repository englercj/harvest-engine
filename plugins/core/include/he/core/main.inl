// Copyright Chad Engler

#include "he/core/compiler.h"

namespace he { int AppMain(int argc, char* argv[]); }

#if defined(HE_IMPL_PLATFORM_MAIN)

HE_IMPL_PLATFORM_MAIN()

#elif defined(HE_PLATFORM_API_WIN32)

namespace he { int _Win32AppMain(); }

int wmain(int, wchar_t* []) { return he::_Win32AppMain(); }
int __stdcall wWinMain(struct HINSTANCE__*, struct HINSTANCE__*, wchar_t*, int) { return he::_Win32AppMain(); }

#elif defined(HE_PLATFORM_WASM)

namespace he { void _SetMainThreadState(); }

int main(int argc, char* argv[])
{
    he::_SetMainThreadState();
    return he::AppMain(argc, argv);
}

#else

#include <clocale>

int main(int argc, char* argv[])
{
    std::setlocale(LC_ALL, "C.UTF-8");
    return he::AppMain(argc, argv);
}

#endif
