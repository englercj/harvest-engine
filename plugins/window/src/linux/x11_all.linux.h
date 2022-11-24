// Copyright Chad Engler

#pragma once

#if defined(HE_PLATFORM_LINUX)

#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput2.h>

// The None define collides with some enums we use
#if defined(None)
    #undef None
#endif

#define X11_None 0L

// The Bool define messes with some enums we have in key_value.h
#if defined(Bool)
    #undef Bool
#endif

#define X11_Bool int

// The Success define messes with some static constants in Result
#if defined(Success)
    #undef Success
#endif

#define X11_Success 0

// These symbols used None and need to be redefines
#if defined(RevertToNone)
    #undef RevertToNone
    #define RevertToNone (int)X11_None
#endif

#endif
