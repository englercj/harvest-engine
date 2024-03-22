// Copyright Chad Engler

#pragma once

#include "_alltypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#undef WEOF
#define WEOF 0xffffffffU

struct tm;

typedef struct __mbstate_t { unsigned __opaque1, __opaque2; } mbstate_t;

wchar_t* wcschr(const wchar_t*, wchar_t);
wchar_t* wcsrchr(const wchar_t*, wchar_t);

wchar_t* wcspbrk(const wchar_t*, const wchar_t*);

wchar_t* wcsstr(const wchar_t*, const wchar_t*);

wchar_t* wmemchr(const wchar_t*, wchar_t, size_t);

#ifdef __cplusplus
}
#endif
