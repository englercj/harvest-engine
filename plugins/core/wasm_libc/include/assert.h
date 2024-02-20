// Copyright Chad Engler

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#undef assert

#ifdef NDEBUG
    #define	assert(x) (void)0
#else
    #define assert(x) ((void)((x) || (__assert_fail(#x, __FILE__, __LINE__, __func__), 0)))
#endif


__attribute__((__noreturn__)) void __assert_fail(const char* expr, const char* file, int line, const char* func);

#ifdef __cplusplus
}
#endif
