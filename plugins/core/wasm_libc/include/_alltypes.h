// Copyright Chad Engler

#pragma once

#include "_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef signed char     int8_t;
typedef short           int16_t;
typedef int             int32_t;
typedef _Int64          int64_t;
typedef _Int64          intmax_t;

typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;
typedef unsigned _Int64 uint64_t;
typedef unsigned _Int64 uintmax_t;

typedef unsigned _Addr size_t;
typedef unsigned _Addr uintptr_t;

typedef _Addr ssize_t;
typedef _Addr intptr_t;
typedef _Addr ptrdiff_t;

typedef _Int64 off_t;

#ifndef __cplusplus
typedef __WCHAR_TYPE__ wchar_t;
#endif

typedef struct _IO_FILE FILE;
typedef struct _locale_struct* locale_t;

#ifdef __cplusplus
}
#endif
