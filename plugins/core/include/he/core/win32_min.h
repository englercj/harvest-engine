// Copyright Chad Engler

#pragma once

#if !defined(WIN32_LEAN_AND_MEAN)
    #define WIN32_LEAN_AND_MEAN
#endif

#if !defined(NOMINMAX)
    #define NOMINMAX
#endif

// X is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
#pragma warning(push)
#pragma warning(disable : 4668)

#include <Windows.h>

#pragma warning(pop)
