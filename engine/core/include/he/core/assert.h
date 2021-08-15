// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/config.h"
#include "he/core/debug.h"
#include "he/core/error.h"

#include "fmt/format.h"

#if defined(_PREFAST_)
    // Prevent code analysis from reporting errors already checked by assertions
    #define HE_ASSERT(expr, ...) __assume(expr)
    #define HE_VERIFY(expr, ...) (!!(expr))
#elif HE_ENABLE_ASSERTIONS
    #define HE_ASSERT(expr, ...) (void)(HE_LIKELY(!!(expr)) || (he::HandleError(he::ErrorType::Assert, HE_FILE, HE_LINE, __FUNCTION__, #expr, ##__VA_ARGS__) && HE_DEBUG_BREAK()))
    #define HE_VERIFY(expr, ...) (HE_LIKELY(!!(expr)) || (he::HandleError(he::ErrorType::Verify, HE_FILE, HE_LINE, __FUNCTION__, #expr, ##__VA_ARGS__) && HE_DEBUG_BREAK()))
#else
    #define HE_ASSERT(expr, ...) (void)0
    #define HE_VERIFY(expr, ...) (!!(expr))
#endif
