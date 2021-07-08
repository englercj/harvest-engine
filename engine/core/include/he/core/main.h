// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/platform.h"
#include "he/core/types.h"

namespace he { int AppMain(int argc, char* argv[]); }

#if HE_COMPILER_MSVC && HE_API_WIN32

namespace he { int _Win32AppMain(); }

int wmain(int, wchar_t* []) { return he::_Win32AppMain(); }
int __stdcall wWinMain(struct HINSTANCE__*, struct HINSTANCE__*, wchar_t*, int) { return he::_Win32AppMain(); }

#else

int main(int argc, char* argv[]) { return he::AppMain(argc, argv); }

#endif
