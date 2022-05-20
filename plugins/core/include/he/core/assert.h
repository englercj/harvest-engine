// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/config.h"
#include "he/core/error.h"

#if defined(_PREFAST_)
    // Prevent code analysis from reporting errors already checked by assertions
    #define HE_ASSERT(expr, ...) __assume(expr)
    #define HE_VERIFY(expr, ...) (!!(expr))
#elif !HE_ENABLE_ASSERTIONS
    #define HE_ASSERT(expr, ...) (void)0
    #define HE_VERIFY(expr, ...) (!!(expr))
#else
    #define HE_ASSERT(expr, ...) (void)HE_ERROR_IF(Assert, expr, __VA_ARGS__)
    #define HE_VERIFY(expr, ...) HE_ERROR_IF(Verify, expr, __VA_ARGS__)
#endif
