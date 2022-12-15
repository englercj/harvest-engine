// Copyright Chad Engler

#pragma once

/// \def HE_CFG_DEBUG
/// Defined when the current configuration is Debug.

/// \def HE_CFG_RELEASE
/// Defined when the current configuration is Release.

/// \def HE_CFG_SHIPPING
/// Defined when the current configuration is Shipping.

/// \def HE_PLATFORM_EMSCRIPTEN
/// Defined when the target platform is Emscripten.

/// \def HE_PLATFORM_LINUX
/// Defined when the target platform is Linux.

/// \def HE_PLATFORM_WINDOWS
/// Defined when the target platform is Windows.

/// \def HE_PLATFORM_API_POSIX
/// Defined when the target platform implements Posix APIs.

/// \def HE_PLATFORM_API_WIN32
/// Defined when the target platform implements Win32 APIs.

/// \def HE_INTERNAL_BUILD
/// Controls if this build is considered Internal. By default this is enabled for non-Shipping
/// configurations.
#if !defined(HE_INTERNAL_BUILD)
    #if defined(HE_CFG_SHIPPING)
        #define HE_INTERNAL_BUILD       0
    #else
        #define HE_INTERNAL_BUILD       1
    #endif
#endif

/// \def HE_ENABLE_ASSERTIONS
/// Controls if assertions are checked. By default these are enabled for Internal builds.
#if !defined(HE_ENABLE_ASSERTIONS)
    #define HE_ENABLE_ASSERTIONS        HE_INTERNAL_BUILD
#endif

/// \def HE_ENABLE_SIMD
/// Controls if SIMD instructions can be used. By default this is enabled.
#if !defined(HE_ENABLE_SIMD)
    #define HE_ENABLE_SIMD              1
#endif

/// \def HE_USER_DEFINED_DEFAULT_ALLOCATOR
/// Controls if the user is responsible for defining the \ref Allocator::GetDefault function.
#if !defined(HE_USER_DEFINED_DEFAULT_ALLOCATOR)
    #define HE_USER_DEFINED_DEFAULT_ALLOCATOR 0
#endif
