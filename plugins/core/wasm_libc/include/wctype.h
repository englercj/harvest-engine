// Copyright Chad Engler

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__WINT_TYPE__)
    typedef __WINT_TYPE__ wint_t;
#else
    typedef unsigned wint_t;
#endif

typedef unsigned int wctype_t;

#ifdef __cplusplus
}
#endif
