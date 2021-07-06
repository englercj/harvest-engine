// Copyright Chad Engler

#pragma once

#define HE_PLATFORM_EMSCRIPTEN              0
#define HE_PLATFORM_LINUX                   0
#define HE_PLATFORM_WINDOWS                 0

// Console platforms are enabled manually by build scripts because their SDKs are non-public.

#if !defined(HE_PLATFORM_XBOX_SERIES_X)
    #define HE_PLATFORM_XBOX_SERIES_X       0
#endif

#if !defined(HE_PLATFORM_PS5)
    #define HE_PLATFORM_PS5                 0
#endif

#if !defined(HE_PLATFORM_SWITCH)
    #define HE_PLATFORM_SWITCH              0
#endif

// API families that may span multiple platforms, such as Xbox and Windows both being Win32.
// These are most useful for filtering code for shared functionality between a family of platforms.
#define HE_API_WIN32                        0
#define HE_API_POSIX                        0
#define HE_API_SCE                          0
#define HE_API_NN                           0

// API types that classify a type of platform, such as Linux and Windows both being Desktop.
// These are most useful for filtering code based on the expected platform abilities.
#define HE_API_DESKTOP                      0
#define HE_API_CONSOLE                      0
#define HE_API_WEB                          0

#if defined(__EMSCRIPTEN__)
    #undef  HE_PLATFORM_EMSCRIPTEN
    #define HE_PLATFORM_EMSCRIPTEN          1
    #undef  HE_API_POSIX
    #define HE_API_POSIX                    1
    #undef  HE_API_WEB
    #define HE_API_WEB                      1
#elif defined(__linux__)
    #undef  HE_PLATFORM_LINUX
    #define HE_PLATFORM_LINUX               1
    #undef  HE_API_POSIX
    #define HE_API_POSIX                    1
    #undef  HE_API_DESKTOP
    #define HE_API_DESKTOP                  1
#elif defined(_WIN32) || defined(_WIN64)
    #undef  HE_PLATFORM_WINDOWS
    #define HE_PLATFORM_WINDOWS             1
    #undef  HE_API_WIN32
    #define HE_API_WIN32                    1
    #undef  HE_API_DESKTOP
    #define HE_API_DESKTOP                  1
#elif HE_PLATFORM_XBOX_SERIES_X
    #undef  HE_API_WIN32
    #define HE_API_WIN32                    1
    #undef  HE_API_CONSOLE
    #define HE_API_CONSOLE                  1
#elif HE_PLATFORM_PS5
    #undef  HE_API_SCE
    #define HE_API_SCE                      1
    #undef  HE_API_CONSOLE
    #define HE_API_CONSOLE                  1
#elif HE_PLATFORM_SWITCH
    #undef  HE_API_NN
    #define HE_API_NN                       1
    #undef  HE_API_CONSOLE
    #define HE_API_CONSOLE                  1
#else
    #error "Unknown platform"
#endif
