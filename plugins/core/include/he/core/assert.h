// Copyright Chad Engler

#pragma once

#include "he/core/compiler.h"
#include "he/core/config.h"
#include "he/core/debug.h"
#include "he/core/error.h"
#include "he/core/log.h"

#if defined(_PREFAST_)
    // Prevent code analysis from reporting errors already checked by assertions
    #define HE_ASSERT(expr, ...) __assume(expr)
    #define HE_VERIFY(expr, ...) (!!(expr))
#elif !HE_ENABLE_ASSERTIONS
    #define HE_ASSERT(expr, ...) (void)0
    #define HE_VERIFY(expr, ...) (!!(expr))
#else
    #define HE_ASSERT_EX(type, expr, ...) \
        (HE_LIKELY(!!(expr)) || ([&](const char* funcName_) { \
            const ::he::ErrorSource ErrorSource_{ ::he::ErrorType::type, HE_LINE, HE_FILE, funcName_, #expr }; \
            const ::he::LogKV errorKvList_[]{ {"",0}, __VA_ARGS__ }; \
            return ::he::HandleError(ErrorSource_, errorKvList_ + 1, HE_LENGTH_OF(errorKvList_) - 1); \
        })(__FUNCTION__) && HE_DEBUG_BREAK())

    #define HE_ASSERT(expr, ...) (void)(HE_ASSERT_EX(Assert, expr, __VA_ARGS__))
    #define HE_VERIFY(expr, ...) (HE_ASSERT_EX(Verify, expr, __VA_ARGS__))
#endif
