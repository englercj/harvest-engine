// Copyright Chad Engler

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define _Addr __PTRDIFF_TYPE__
#define _Int64 __INT64_TYPE__

#define _Noreturn       __attribute__((__noreturn__))
#define _Forceinline    __attribute__((always_inline)) inline
#define _Hidden         __attribute__((__visibility__("hidden")))

#ifdef __cplusplus
}
#endif
