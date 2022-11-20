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

// X11 headers define 'None' which messes with our symbols so we undefine that one
// and use X11_None instead
#if defined(None)
    #undef None
#endif

#define X11_None 0L

#endif
