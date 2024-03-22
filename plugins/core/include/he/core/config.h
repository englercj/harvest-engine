// Copyright Chad Engler

#pragma once

/// \def HE_CFG_DEBUG
/// Defined when the current configuration is Debug.

/// \def HE_CFG_DEVELOPMENT
/// Defined when the current configuration is Development.

/// \def HE_CFG_RELEASE
/// Defined when the current configuration is Release.

/// \def HE_PLATFORM_LINUX
/// Defined when the target platform is Linux.

/// \def HE_PLATFORM_WASM32
/// Defined when the target platform is Wasm32.

/// \def HE_PLATFORM_WASM64
/// Defined when the target platform is Wasm64.

/// \def HE_PLATFORM_WINDOWS
/// Defined when the target platform is Windows.

/// \def HE_PLATFORM_API_POSIX
/// Defined when the target platform implements Posix APIs.

/// \def HE_PLATFORM_API_WASM
/// Defined when the target platform implements WASM APIs.

/// \def HE_PLATFORM_API_WIN32
/// Defined when the target platform implements Win32 APIs.

/// \def HE_INTERNAL_BUILD
/// Controls if this build is considered Internal. By default this is enabled for non-Release
/// configurations.
#if !defined(HE_INTERNAL_BUILD)
    #if defined(HE_CFG_RELEASE)
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

/// \def HE_ENABLE_DEFAULT_ALLOCATOR
/// When true (default) defines the \ref Allocator::GetDefault function. Otherwise, the user is
/// responsible for defining that function.
#if !defined(HE_ENABLE_DEFAULT_ALLOCATOR)
    #define HE_ENABLE_DEFAULT_ALLOCATOR     1
#endif

/// \def HE_HAS_LIBC
/// When true libc is available for use. Some platforms do not have a libc implementation and
/// require a custom implementation.
#if !defined(HE_HAS_LIBC)
    #if defined(HE_PLATFORM_API_WASM)
        #define HE_HAS_LIBC             0
    #else
        #define HE_HAS_LIBC             1
    #endif
#endif
