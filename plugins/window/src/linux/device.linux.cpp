// Copyright Chad Engler

#include "device.linux.h"

#include "gamepad.linux.h"
#include "view.linux.h"

#include "he/core/log.h"
#include "he/core/memory_ops.h"
#include "he/core/string_ops.h"
#include "he/math/vec2.h"
#include "he/window/application.h"
#include "he/window/event.h"
#include "he/window/key.h"

#if defined(HE_PLATFORM_LINUX)

#include <dlfcn.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "x11_all.linux.h"

namespace he::window::linux
{
    static Key TranslateKey(KeySym x11Key)
    {
        switch (x11Key)
        {
            case XK_BackSpace: return Key::Backspace;
            case XK_Return: return Key::Enter;
            case XK_Escape: return Key::Escape;
            case XK_space: return Key::Space;
            case XK_Tab: return Key::Tab;
            case XK_Pause: return Key::Pause;
            case XK_Print: return Key::PrintScreen;
            case XK_KP_Delete: return Key::NumPad_Decimal; // or XK_KP_Decimal? or XK_KP_Separator?
            case XK_KP_Multiply: return Key::NumPad_Multiply;
            case XK_KP_Add: return Key::NumPad_Add;
            case XK_KP_Subtract: return Key::NumPad_Subtract;
            case XK_KP_Divide: return Key::NumPad_Divide;
            case XK_KP_Insert:
            case XK_KP_0: return Key::NumPad_0;
            case XK_KP_End:
            case XK_KP_1: return Key::NumPad_1;
            case XK_KP_Down:
            case XK_KP_2: return Key::NumPad_2;
            case XK_KP_Page_Down: // or XK_KP_Next
            case XK_KP_3: return Key::NumPad_3;
            case XK_KP_Left:
            case XK_KP_4: return Key::NumPad_4;
            case XK_KP_Begin:
            case XK_KP_5: return Key::NumPad_5;
            case XK_KP_Right:
            case XK_KP_6: return Key::NumPad_6;
            case XK_KP_Home:
            case XK_KP_7: return Key::NumPad_7;
            case XK_KP_Up:
            case XK_KP_8: return Key::NumPad_8;
            case XK_KP_Page_Up:   // or XK_KP_Prior
            case XK_KP_9: return Key::NumPad_9;
            case XK_F1: return Key::F1;
            case XK_F2: return Key::F2;
            case XK_F3: return Key::F3;
            case XK_F4: return Key::F4;
            case XK_F5: return Key::F5;
            case XK_F6: return Key::F6;
            case XK_F7: return Key::F7;
            case XK_F8: return Key::F8;
            case XK_F9: return Key::F9;
            case XK_F10: return Key::F10;
            case XK_F11: return Key::F11;
            case XK_F12: return Key::F12;
            case XK_Home: return Key::Home;
            case XK_Left: return Key::Left;
            case XK_Up: return Key::Up;
            case XK_Right: return Key::Right;
            case XK_Down: return Key::Down;
            case XK_Page_Up: return Key::PageUp;
            case XK_Page_Down: return Key::PageDown;
            case XK_Insert: return Key::Insert;
            case XK_Delete: return Key::Delete;
            case XK_End: return Key::End;
            case XK_Alt_L:
            case XK_Alt_R: return Key::Alt;
            case XK_Control_L:
            case XK_Control_R: return Key::Control;
            case XK_Shift_L:
            case XK_Shift_R: return Key::Shift;
            case XK_Super_L:
            case XK_Super_R: return Key::Super;
            case XK_Scroll_Lock: return Key::ScrollLock;
            case XK_Num_Lock: return Key::NumLock;
            case XK_Caps_Lock: return Key::CapsLock;
            case '0': return Key::Number_0;
            case '1': return Key::Number_1;
            case '2': return Key::Number_2;
            case '3': return Key::Number_3;
            case '4': return Key::Number_4;
            case '5': return Key::Number_5;
            case '6': return Key::Number_6;
            case '7': return Key::Number_7;
            case '8': return Key::Number_8;
            case '9': return Key::Number_9;
            case 'a': return Key::A;
            case 'b': return Key::B;
            case 'c': return Key::C;
            case 'd': return Key::D;
            case 'e': return Key::E;
            case 'f': return Key::F;
            case 'g': return Key::G;
            case 'h': return Key::H;
            case 'i': return Key::I;
            case 'j': return Key::J;
            case 'k': return Key::K;
            case 'l': return Key::L;
            case 'm': return Key::M;
            case 'n': return Key::N;
            case 'o': return Key::O;
            case 'p': return Key::P;
            case 'q': return Key::Q;
            case 'r': return Key::R;
            case 's': return Key::S;
            case 't': return Key::T;
            case 'u': return Key::U;
            case 'v': return Key::V;
            case 'w': return Key::W;
            case 'x': return Key::X;
            case 'y': return Key::Y;
            case 'z': return Key::Z;
            case XK_semicolon: return Key::Semicolon;
            case XK_equal: return Key::Equals;
            case XK_comma: return Key::Comma;
            case XK_minus: return Key::Minus;
            case XK_period: return Key::Period;
            case XK_slash: return Key::Slash;
            case XK_grave: return Key::Grave;
            case XK_bracketleft: return Key::LeftBracket;
            case XK_backslash: return Key::Backslash;
            case XK_bracketright: return Key::RightBracket;
            case XK_apostrophe: return Key::Apostrophe;
        }
        return Key::None;
    }

    static const char* UriToFilePath(char* uri)
    {
        constexpr char FilePrefix[] = "file://";
        constexpr uint32_t FilePrefixLen = HE_LENGTH_OF(FilePrefix) - 1;

        if (uri[0] == '#')
            return nullptr;

        // If the uri specifies the file:// scheme then we want to verify the hostname
        if (StrEqualN(uri, FilePrefix, FilePrefixLen))
        {
            uri += FilePrefixLen;

            const char* hostEnd = StrFind(uri, '/');
            if (hostEnd)
            {
                char host[257];
                if (gethostname(host, 255) == 0)
                {
                    host[256] = '\0';
                    const uint32_t hostLen = hostEnd - uri;
                    if (!StrEqualN(uri, host, hostLen))
                        return nullptr;

                    uri = const_cast<char*>(hostEnd + 1);
                }
            }
        }
        // If a uri scheme is specified, but its not file:// then we can't handle this
        else if (StrFind(uri, ":/"))
        {
            return nullptr;
        }

        // Decode the URI in-place
        const char* end = uri + StrLen(uri);
        const char* read = uri;
        char* write = uri;

        while (read < end && write < end)
        {
            if (read[0] == '%' && read[1] && read[2])
            {
                const char digits[3]{ read[1], read[2], '\0' };
                *write++ = static_cast<char>(StrToInt<uint8_t>(digits, nullptr, 16));
                read += 3;
            }
            else
            {
                *write++ = *read++;
            }
        }

        return uri;
    }

    DeviceImpl::DeviceImpl(Allocator& allocator) noexcept
        : Device(allocator)
        , m_gamepads{ { this, 0 }, { this, 1 }, { this, 2 }, { this, 3 } }
    {}

    DeviceImpl::~DeviceImpl() noexcept
    {
        if (m_xi)
            dlclose(m_xi);

        if (m_xlib)
        {
            if (m_display)
            {
                if (m_hiddenCursor != X11_None)
                {
                    m_XFreeCursor(m_display, m_hiddenCursor);
                    m_XFreePixmap(m_display, m_hiddenCursorBitmap);
                }

                for (uint32_t i = 0; i < static_cast<uint32_t>(PointerCursor::_Count); ++i)
                {
                    if (m_cursors[i] != X11_None)
                        m_XFreeCursor(m_display, m_cursors[i]);
                }

                if (m_im)
                    m_XCloseIM(m_im);

                m_XCloseDisplay(m_display);
            }

            dlclose(m_xlib);
        }
    }

    bool DeviceImpl::Initialize()
    {
        m_xlib = dlopen("libX11.so.6", RTLD_LAZY | RTLD_LOCAL);
        if (!m_xlib)
        {
            HE_LOGF_ERROR(he_window, "Failed to load Xlib.");
            return false;
        }

        m_XChangeProperty = reinterpret_cast<Pfn_XChangeProperty>(dlsym(m_xlib, "XChangeProperty"));
        m_XCloseDisplay = reinterpret_cast<Pfn_XCloseDisplay>(dlsym(m_xlib, "XCloseDisplay"));
        m_XCloseIM = reinterpret_cast<Pfn_XCloseIM>(dlsym(m_xlib, "XCloseIM"));
        m_XConvertSelection = reinterpret_cast<Pfn_XConvertSelection>(dlsym(m_xlib, "XConvertSelection"));
        m_XCreateFontCursor = reinterpret_cast<Pfn_XCreateFontCursor>(dlsym(m_xlib, "XCreateFontCursor"));
        m_XCreateBitmapFromData = reinterpret_cast<Pfn_XCreateBitmapFromData>(dlsym(m_xlib, "XCreateBitmapFromData"));
        m_XCreateIC = reinterpret_cast<Pfn_XCreateIC>(dlsym(m_xlib, "XCreateIC"));
        m_XCreatePixmapCursor = reinterpret_cast<Pfn_XCreatePixmapCursor>(dlsym(m_xlib, "XCreatePixmapCursor"));
        m_XCreateWindow = reinterpret_cast<Pfn_XCreateWindow>(dlsym(m_xlib, "XCreateWindow"));
        m_XDefineCursor = reinterpret_cast<Pfn_XDefineCursor>(dlsym(m_xlib, "XDefineCursor"));
        m_XDeleteContext = reinterpret_cast<Pfn_XDeleteContext>(dlsym(m_xlib, "XDeleteContext"));
        m_XDestroyIC = reinterpret_cast<Pfn_XDestroyIC>(dlsym(m_xlib, "XDestroyIC"));
        m_XDestroyWindow = reinterpret_cast<Pfn_XDestroyWindow>(dlsym(m_xlib, "XDestroyWindow"));
        m_XFilterEvent = reinterpret_cast<Pfn_XFilterEvent>(dlsym(m_xlib, "XFilterEvent"));
        m_XFindContext = reinterpret_cast<Pfn_XFindContext>(dlsym(m_xlib, "XFindContext"));
        m_XFlush = reinterpret_cast<Pfn_XFlush>(dlsym(m_xlib, "XFlush"));
        m_XFree = reinterpret_cast<Pfn_XFree>(dlsym(m_xlib, "XFree"));
        m_XFreeCursor = reinterpret_cast<Pfn_XFreeCursor>(dlsym(m_xlib, "XFreeCursor"));
        m_XFreeEventData = reinterpret_cast<Pfn_XFreeEventData>(dlsym(m_xlib, "XFreeEventData"));
        m_XFreePixmap = reinterpret_cast<Pfn_XFreePixmap>(dlsym(m_xlib, "XFreePixmap"));
        m_XGetAtomName = reinterpret_cast<Pfn_XGetAtomName>(dlsym(m_xlib, "XGetAtomName"));
        m_XGetEventData = reinterpret_cast<Pfn_XGetEventData>(dlsym(m_xlib, "XGetEventData"));
        m_XGetInputFocus = reinterpret_cast<Pfn_XGetInputFocus>(dlsym(m_xlib, "XGetInputFocus"));
        m_XGetWMNormalHints = reinterpret_cast<Pfn_XGetWMNormalHints>(dlsym(m_xlib, "XGetWMNormalHints"));
        m_XGetWindowAttributes = reinterpret_cast<Pfn_XGetWindowAttributes>(dlsym(m_xlib, "XGetWindowAttributes"));
        m_XGetWindowProperty = reinterpret_cast<Pfn_XGetWindowProperty>(dlsym(m_xlib, "XGetWindowProperty"));
        m_XGrabPointer = reinterpret_cast<Pfn_XGrabPointer>(dlsym(m_xlib, "XGrabPointer"));
        m_XIconifyWindow = reinterpret_cast<Pfn_XIconifyWindow>(dlsym(m_xlib, "XIconifyWindow"));
        m_XInitThreads = reinterpret_cast<Pfn_XInitThreads>(dlsym(m_xlib, "XInitThreads"));
        m_XInternAtom = reinterpret_cast<Pfn_XInternAtom>(dlsym(m_xlib, "XInternAtom"));
        m_XLookupKeysym = reinterpret_cast<Pfn_XLookupKeysym>(dlsym(m_xlib, "XLookupKeysym"));
        m_XMapRaised = reinterpret_cast<Pfn_XMapRaised>(dlsym(m_xlib, "XMapRaised"));
        m_XMapWindow = reinterpret_cast<Pfn_XMapWindow>(dlsym(m_xlib, "XMapWindow"));
        m_XMoveResizeWindow = reinterpret_cast<Pfn_XMoveResizeWindow>(dlsym(m_xlib, "XMoveResizeWindow"));
        m_XMoveWindow = reinterpret_cast<Pfn_XMoveWindow>(dlsym(m_xlib, "XMoveWindow"));
        m_XNextEvent = reinterpret_cast<Pfn_XNextEvent>(dlsym(m_xlib, "XNextEvent"));
        m_XOpenDisplay = reinterpret_cast<Pfn_XOpenDisplay>(dlsym(m_xlib, "XOpenDisplay"));
        m_XOpenIM = reinterpret_cast<Pfn_XOpenIM>(dlsym(m_xlib, "XOpenIM"));
        m_XPeekEvent = reinterpret_cast<Pfn_XPeekEvent>(dlsym(m_xlib, "XPeekEvent"));
        m_XPending = reinterpret_cast<Pfn_XPending>(dlsym(m_xlib, "XPending"));
        m_XQueryExtension = reinterpret_cast<Pfn_XQueryExtension>(dlsym(m_xlib, "XQueryExtension"));
        m_XQueryPointer = reinterpret_cast<Pfn_XQueryPointer>(dlsym(m_xlib, "XQueryPointer"));
        m_XRaiseWindow = reinterpret_cast<Pfn_XRaiseWindow>(dlsym(m_xlib, "XRaiseWindow"));
        m_XRefreshKeyboardMapping = reinterpret_cast<Pfn_XRefreshKeyboardMapping>(dlsym(m_xlib, "XRefreshKeyboardMapping"));
        m_XResizeWindow = reinterpret_cast<Pfn_XResizeWindow>(dlsym(m_xlib, "XResizeWindow"));
        m_XSaveContext = reinterpret_cast<Pfn_XSaveContext>(dlsym(m_xlib, "XSaveContext"));
        m_XSendEvent = reinterpret_cast<Pfn_XSendEvent>(dlsym(m_xlib, "XSendEvent"));
        m_XSetInputFocus = reinterpret_cast<Pfn_XSetInputFocus>(dlsym(m_xlib, "XSetInputFocus"));
        m_XSetWMProtocols = reinterpret_cast<Pfn_XSetWMProtocols>(dlsym(m_xlib, "XSetWMProtocols"));
        m_XStoreName = reinterpret_cast<Pfn_XStoreName>(dlsym(m_xlib, "XStoreName"));
        m_XTranslateCoordinates = reinterpret_cast<Pfn_XTranslateCoordinates>(dlsym(m_xlib, "XTranslateCoordinates"));
        m_XUngrabPointer = reinterpret_cast<Pfn_XUngrabPointer>(dlsym(m_xlib, "XUngrabPointer"));
        m_XUnmapWindow = reinterpret_cast<Pfn_XUnmapWindow>(dlsym(m_xlib, "XUnmapWindow"));
        m_XUnsetICFocus = reinterpret_cast<Pfn_XUnsetICFocus>(dlsym(m_xlib, "XUnsetICFocus"));
        m_XWarpPointer = reinterpret_cast<Pfn_XWarpPointer>(dlsym(m_xlib, "XWarpPointer"));
        m_XkbSetDetectableAutoRepeat = reinterpret_cast<Pfn_XkbSetDetectableAutoRepeat>(dlsym(m_xlib, "XkbSetDetectableAutoRepeat"));
        m_XrmUniqueQuark = reinterpret_cast<Pfn_XrmUniqueQuark>(dlsym(m_xlib, "XrmUniqueQuark"));
        m_Xutf8LookupString = reinterpret_cast<Pfn_Xutf8LookupString>(dlsym(m_xlib, "Xutf8LookupString"));
        m_Xutf8SetWMProperties = reinterpret_cast<Pfn_Xutf8SetWMProperties>(dlsym(m_xlib, "Xutf8SetWMProperties"));

        // This function returns a nonzero status if initialization was successful; otherwise, it returns zero
        if (m_XInitThreads() == 0)
        {
            HE_LOGF_ERROR(he_window, "XInitThreads failed.");
            return false;
        }

        m_display = m_XOpenDisplay(nullptr);
        if (m_display == nullptr)
        {
            HE_LOGF_ERROR(he_window, "XOpenDisplay failed.");
            return false;
        }

        m_atomNetActiveWindow = m_XInternAtom(m_display, "_NET_ACTIVE_WINDOW", False);
        m_atomNetWMPing = m_XInternAtom(m_display, "_NET_WM_PING", False);
        m_atomNetWMState = m_XInternAtom(m_display, "_NET_WM_STATE", False);
        m_atomNetWMStateAbove = m_XInternAtom(m_display, "_NET_WM_STATE_ABOVE", False);
        m_atomNetWMStateMinimized = m_XInternAtom(m_display, "_NET_WM_STATE_HIDDEN", False);
        m_atomNetWMStateMaximizedHorz = m_XInternAtom(m_display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
        m_atomNetWMStateMaximizedVert = m_XInternAtom(m_display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
        m_atomNetWMStateFullscreen = m_XInternAtom(m_display, "_NET_WM_STATE_FULLSCREEN", False);
        m_atomNetWMWindowOpacity = m_XInternAtom(m_display, "_NET_WM_WINDOW_OPACITY", False);
        m_atomNetWMWindowType = m_XInternAtom(m_display, "_NET_WM_WINDOW_TYPE", False);
        m_atomNetWMWindowTypeNormal = m_XInternAtom(m_display, "_NET_WM_WINDOW_TYPE_NORMAL", False);
        m_atomMotifWMHints = m_XInternAtom(m_display, "_MOTIF_WM_HINTS", False);
        m_atomWMDeleteWindow = m_XInternAtom(m_display, "WM_DELETE_WINDOW", False);
        m_atomWMProtocols = m_XInternAtom(m_display, "WM_PROTOCOLS", False);
        m_atomWMState = m_XInternAtom(m_display, "WM_STATE", False);
        // m_atomWMTakeFocus = m_XInternAtom(m_display, "WM_TAKE_FOCUS", False);
        m_atomXdndAware = m_XInternAtom(m_display, "XdndAware", False);
        m_atomXdndEnter = m_XInternAtom(m_display, "XdndEnter", False);
        m_atomXdndLeave = m_XInternAtom(m_display, "XdndLeave", False);
        m_atomXdndPosition = m_XInternAtom(m_display, "XdndPosition", False);
        m_atomXdndStatus = m_XInternAtom(m_display, "XdndStatus", False);
        m_atomXdndActionCopy = m_XInternAtom(m_display, "XdndActionCopy", False);
        m_atomXdndActionMove = m_XInternAtom(m_display, "XdndActionMove", False);
        m_atomXdndActionLink = m_XInternAtom(m_display, "XdndActionLink", False);
        m_atomXdndDrop = m_XInternAtom(m_display, "XdndDrop", False);
        m_atomXdndFinished = m_XInternAtom(m_display, "XdndFinished", False);
        m_atomXdndSelection = m_XInternAtom(m_display, "XdndSelection", False);
        m_atomXdndTypeList = m_XInternAtom(m_display, "XdndTypeList", False);
        m_atomTextUriList = m_XInternAtom(m_display, "text/uri-list", False);

        m_root = RootWindow(m_display, DefaultScreen(m_display));
        m_context = ((XContext)m_XrmUniqueQuark()); // XUniqueContext();

        // Attempt to enable detectable auto-repeat so we can tell when X11 is simulating
        // repeated key presses and prevent it from sending virtual key up events.
        if (m_XkbSetDetectableAutoRepeat)
        {
            X11_Bool supported = False;
            X11_Bool enabled = m_XkbSetDetectableAutoRepeat(m_display, True, &supported);

            if (enabled == True && supported == True)
                m_hasDetectableAutoRepeat = true;
        }

        // Open the input method
        m_im = m_XOpenIM(m_display, nullptr, nullptr, nullptr);
        if (m_im == nullptr)
        {
            HE_LOGF_ERROR(he_window, "XOpenIM failed.");
            return false;
        }

        // Always have support for mouse
        m_deviceInfo.hasMouse = true;

        // Detect raw mouse input support
        m_xi = dlopen("libXi.so.6", RTLD_LAZY | RTLD_LOCAL);
        if (m_xi)
        {
            m_XIQueryVersion = reinterpret_cast<Pfn_XIQueryVersion>(dlsym(m_xi, "XIQueryVersion"));
            m_XISelectEvents = reinterpret_cast<Pfn_XISelectEvents>(dlsym(m_xi, "XISelectEvents"));
            m_XIGrabTouchBegin = reinterpret_cast<Pfn_XIGrabTouchBegin>(dlsym(m_xi, "XIGrabTouchBegin"));
            m_XIUngrabTouchBegin = reinterpret_cast<Pfn_XIUngrabTouchBegin>(dlsym(m_xi, "XIUngrabTouchBegin"));
            m_XIQueryDevice = reinterpret_cast<Pfn_XIQueryDevice>(dlsym(m_xi, "XIQueryDevice"));
            m_XIFreeDeviceInfo = reinterpret_cast<Pfn_XIFreeDeviceInfo>(dlsym(m_xi, "XIFreeDeviceInfo"));

            if (m_XIQueryVersion && m_XISelectEvents)
            {
                int eventBase, errorBase;
                if (m_XQueryExtension(m_display, "XInputExtension", &m_xiMajorOpcode, &eventBase, &errorBase))
                {
                    int majorVer, minorVer;
                    if (m_XIQueryVersion(m_display, &majorVer, &minorVer) == X11_Success)
                    {
                        const int version = (majorVer * 1000) + minorVer;
                        constexpr int Version_2_0 = (2 * 1000) + 0;
                        constexpr int Version_2_2 = (2 * 1000) + 2;
                        m_deviceInfo.hasHighDefMouse = version >= Version_2_0;
                        m_deviceInfo.hasTouch = version >= Version_2_2;

                        if (m_deviceInfo.hasTouch)
                        {
                            int deviceCount = 0;
                            XIDeviceInfo* devices = m_XIQueryDevice(m_display, XIAllDevices, &deviceCount);
                            for (int i = 0; i < deviceCount; ++i)
                            {
                                const XIDeviceInfo& device = devices[i];
                                for (int j = 0; j < device.num_classes; ++j)
                                {
                                    const XITouchClassInfo* touchClass = reinterpret_cast<const XITouchClassInfo*>(device.classes[j]);
                                    if (touchClass->type != XITouchClass)
                                        continue;

                                    // Only reading first touch device currently to get maxTouches
                                    // Theoretically there could be multiple touch devices, each
                                    // with multiple multiple touch classes.
                                    m_deviceInfo.maxTouches = touchClass->num_touches;
                                    goto loopEnd;
                                }
                            }
                        loopEnd:
                            m_XIFreeDeviceInfo(devices);
                        }
                    }
                }
            }
        }

        // Enable raw mouse input support
        if (m_deviceInfo.hasHighDefMouse)
        {
            uint8_t mask[XIMaskLen(XI_LASTEVENT)]{};

            XIEventMask em;
            em.deviceid = XIAllMasterDevices;
            em.mask_len = sizeof(mask);
            em.mask = mask;
            XISetMask(mask, XI_RawMotion);
            XISetMask(mask, XI_Motion);
            if (m_deviceInfo.hasTouch)
            {
                XISetMask(mask, XI_TouchBegin);
                XISetMask(mask, XI_TouchUpdate);
                XISetMask(mask, XI_TouchEnd);
            }

            m_XISelectEvents(m_display, m_root, &em, 1);

            MemZero(&em, sizeof(em));
            MemZero(mask, sizeof(mask));
            em.deviceid = XIAllDevices;
            em.mask_len = sizeof(mask);
            em.mask = mask;

            XISetMask(mask, XI_HierarchyChanged);
            m_XISelectEvents(m_display, m_root, &em, 1);
        }

        // Create hidden cursor
        const char data[8]{};
        XColor dummy;
        m_hiddenCursorBitmap = m_XCreateBitmapFromData(m_display, m_root, data, 8, 8);
        m_hiddenCursor = m_XCreatePixmapCursor(m_display, m_hiddenCursorBitmap, m_hiddenCursorBitmap, &dummy, &dummy, 0, 0);

        // Create standard cursors
        m_cursors[static_cast<uint32_t>(PointerCursor::Arrow)] = m_XCreateFontCursor(m_display, XC_left_ptr);
        m_cursors[static_cast<uint32_t>(PointerCursor::Hand)] = m_XCreateFontCursor(m_display, XC_hand2);
        m_cursors[static_cast<uint32_t>(PointerCursor::NotAllowed)] = m_XCreateFontCursor(m_display, XC_X_cursor);
        m_cursors[static_cast<uint32_t>(PointerCursor::TextInput)] = m_XCreateFontCursor(m_display, XC_xterm);
        m_cursors[static_cast<uint32_t>(PointerCursor::ResizeAll)] = m_XCreateFontCursor(m_display, XC_fleur);
        m_cursors[static_cast<uint32_t>(PointerCursor::ResizeTopLeft)] = m_XCreateFontCursor(m_display, XC_top_left_corner);
        m_cursors[static_cast<uint32_t>(PointerCursor::ResizeTopRight)] = m_XCreateFontCursor(m_display, XC_top_right_corner);
        m_cursors[static_cast<uint32_t>(PointerCursor::ResizeBottomLeft)] = m_XCreateFontCursor(m_display, XC_bottom_left_corner);
        m_cursors[static_cast<uint32_t>(PointerCursor::ResizeBottomRight)] = m_XCreateFontCursor(m_display, XC_bottom_right_corner);
        m_cursors[static_cast<uint32_t>(PointerCursor::ResizeHorizontal)] = m_XCreateFontCursor(m_display, XC_sb_h_double_arrow);
        m_cursors[static_cast<uint32_t>(PointerCursor::ResizeVertical)] = m_XCreateFontCursor(m_display, XC_sb_v_double_arrow);
        m_cursors[static_cast<uint32_t>(PointerCursor::Wait)] = m_XCreateFontCursor(m_display, XC_watch);

        return true;
    }

    int DeviceImpl::Run(Application& app, const ViewDesc& desc)
    {
        m_app = &app;

        // Create root window
        ViewImpl view(this, desc);
        view.SetVisible(true, true);

        // Dispatch the Initialized event before we start the loop
        {
            InitializedEvent ev(&view);
            app.OnEvent(ev);
        }

        // Event loop
        while (m_running.load())
        {
            // Update gamepads
            for (GamepadImpl& pad : m_gamepads)
            {
                pad.Update(m_refreshGamepadConnectivity);
            }
            m_refreshGamepadConnectivity = false;

            // Process window messages
            while (m_XPending(m_display))
            {
                XEvent event;
                m_XNextEvent(m_display, &event);
                HandleXEvent(event);
            }

            // Handle view clipping
            if (m_viewClipped == false && m_cursorRelativeMode)
            {
                uint32_t mask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
                m_XGrabPointer(m_display, view.m_window, True, mask, GrabModeAsync, GrabModeAsync, view.m_window, m_hiddenCursor, CurrentTime);
                m_XFlush(m_display);
                m_viewClipped = true;
            }
            else if (m_viewClipped && m_cursorRelativeMode == false)
            {
                m_XUngrabPointer(m_display, CurrentTime);
                m_XFlush(m_display);
                m_viewClipped = false;
            }

            // Tick the application after updates have completed
            app.OnTick();
        }

        // Dispatch the Terminating event now that we've exited the event loop
        {
            TerminatingEvent ev;
            app.OnEvent(ev);
        }

        m_app = nullptr;
        return m_returnCode.load();
    }

    void DeviceImpl::Quit(int rc)
    {
        m_returnCode.store(rc);
        m_running.store(false);
    }

    const DeviceInfo& DeviceImpl::GetInfo() const
    {
        return m_deviceInfo;
    }

    View* DeviceImpl::CreateView(const ViewDesc& desc)
    {
        ViewImpl* view = m_allocator.New<ViewImpl>(this, desc);
        return view;
    }

    void DeviceImpl::DestroyView(View* view)
    {
        m_allocator.Delete(view);
    }

    View* DeviceImpl::GetFocusedView() const
    {
        Window focused = X11_None;
        int revertTo = RevertToNone;
        m_XGetInputFocus(m_display, &focused, &revertTo);

        return GetViewFromWindow(focused);
    }

    View* DeviceImpl::GetHoveredView() const
    {
        Window rootWin = X11_None;
        Window childWin = X11_None;
        int rootX = 0;
        int rootY = 0;
        int winX = 0;
        int winY = 0;
        uint32_t mask;
        m_XQueryPointer(m_display, m_root, &rootWin, &childWin, &rootX, &rootY, &winX, &winY, &mask);

        return GetViewFromWindow(childWin);
    }

    Vec2f DeviceImpl::GetCursorPos(View* view_) const
    {
        ViewImpl* view = static_cast<ViewImpl*>(view_);
        Window win = view ? view->m_window : m_root;

        Window rootWin = X11_None;
        Window childWin = X11_None;
        int rootX = 0;
        int rootY = 0;
        int winX = 0;
        int winY = 0;
        uint32_t mask;
        X11_Bool result = m_XQueryPointer(m_display, win, &rootWin, &childWin, &rootX, &rootY, &winX, &winY, &mask);

        if (result != True)
            return {};

        return { static_cast<float>(winX), static_cast<float>(winY) };
    }

    void DeviceImpl::SetCursorPos(View* view_, const Vec2f& pos)
    {
        ViewImpl* view = static_cast<ViewImpl*>(view_);
        Window win = view ? view->m_window : m_root;

        const int dstX = static_cast<int>(pos.x);
        const int dstY = static_cast<int>(pos.y);
        m_XWarpPointer(m_display, X11_None, win, 0, 0, 0, 0, dstX, dstY);
    }

    void DeviceImpl::SetCursor(PointerCursor cursor)
    {
        if (m_cursor != cursor)
        {
            m_cursor = cursor;
            m_XDefineCursor(m_display, m_root, GetActiveCursor());
        }
    }

    void DeviceImpl::SetCursorRelativeMode(bool relativeMode)
    {
        if (m_cursorRelativeMode == relativeMode)
            return;

        m_cursorRelativeMode = relativeMode;

        ShowCursor(!m_cursorRelativeMode);

        if (m_cursorRelativeMode)
        {
            m_cursorRestorePosition = GetCursorPos(nullptr);
            CenterCursor();
        }
        else
        {
            SetCursorPos(nullptr, m_cursorRestorePosition);
            m_cursorRestorePosition = { 0, 0 };
        }
    }

    uint32_t DeviceImpl::GetMonitorCount() const
    {
        return static_cast<uint32_t>(ScreenCount(m_display));
    }

    uint32_t DeviceImpl::GetMonitors(Monitor* monitors, uint32_t maxCount) const
    {
        const uint32_t count = GetMonitorCount();

        uint32_t i = 0;
        for (; i < count && i < maxCount; ++i)
        {
            Screen* screen = ScreenOfDisplay(m_display, i);
            Window win = RootWindowOfScreen(screen);

            XWindowAttributes attribs;
            m_XGetWindowAttributes(m_display, win, &attribs);

            // TODO: Debug and confirm these values are reasonable
            Monitor& monitor = monitors[i];
            monitor.pos = { attribs.x, attribs.y };
            monitor.size = { attribs.width, attribs.height };
            monitor.pos = { attribs.x, attribs.y };
            monitor.workSize = { attribs.width, attribs.height };
            monitor.primary = true;
        }

        return i;
    }

    Gamepad& DeviceImpl::GetGamepad(uint32_t index)
    {
        HE_ASSERT(index < HE_LENGTH_OF(m_gamepads));
        return m_gamepads[index];
    }

    void DeviceImpl::ShowCursor(bool show)
    {
        if (m_cursorVisible == show)
            return;

        m_cursorVisible = show;
        m_XDefineCursor(m_display, m_root, GetActiveCursor());
    }

    void DeviceImpl::CenterCursor()
    {
        View* view = GetFocusedView();
        if (!view)
            return;

        const Vec2f pos = MakeVec2<float>(view->GetSize()) / 2.0f;
        SetCursorPos(view, pos);
    }

    ViewImpl* DeviceImpl::GetViewFromWindow(Window win) const
    {
        if (win == X11_None)
            return nullptr;

        ViewImpl* view = nullptr;
        if (m_XFindContext(m_display, win, m_context, reinterpret_cast<XPointer*>(&view)) == 0)
            return view;

        return nullptr;
    }

    Cursor DeviceImpl::GetActiveCursor() const
    {
        if (!m_cursorVisible || m_viewClipped || m_cursor <= PointerCursor::None || m_cursor >= PointerCursor::_Count)
            return m_hiddenCursor;

        return m_cursors[static_cast<int32_t>(m_cursor)];
    }

    void DeviceImpl::HandleXEvent(XEvent& event)
    {
        // Give input methods (like IMEs) a chance to handle the event
        if (m_XFilterEvent(&event, X11_None) == True)
            return;

        if (event.type == GenericEvent)
            return HandleGenericXEvent(event);

        ViewImpl* view = GetViewFromWindow(event.xany.window);

        if (view)
            HandleViewXEvent(view, event);
        else
            HandleNonViewXEvent(event);
    }

    void DeviceImpl::HandleGenericXEvent(XEvent& event)
    {
        if (!m_XGetEventData(m_display, &event.xcookie) || event.xcookie.extension != m_xiMajorOpcode)
        {
            m_XFreeEventData(m_display, &event.xcookie);
            return;
        }

        switch (event.xcookie.evtype)
        {
            case XI_RawMotion:
            {
                if (!m_deviceInfo.hasHighDefMouse)
                    break;

                const XIRawEvent* xev = static_cast<const XIRawEvent*>(event.xcookie.data);
                InputDeviceInfo& device = FindInputDeviceInfo(xev->deviceid);

                if (xev->valuators.mask_len == 0)
                    break;
                Vec2f pos{};

                if (XIMaskIsSet(xev->valuators.mask, 0))
                    pos.x = static_cast<float>(xev->raw_values[0]);

                if (XIMaskIsSet(xev->valuators.mask, 1))
                    pos.y = static_cast<float>(xev->raw_values[1]);

                // Ignore duplicate events
                if (xev->time == device.prevTime && pos == device.prevPos)
                    break;;

                device.prevTime = xev->time;
                device.prevPos = pos;

                // Technically each axis could separately report relative or absolute, so this
                // is a bit of a hack. In practice that doesn't seem to happen though.
                const bool absolute = !device.relative[0];

                // Ignore relative motion of 0,0
                if (!absolute && pos.x == 0 && pos.y == 0)
                    break;

                PointerMoveEvent ev(GetFocusedView());
                ev.pointerId = PointerId_Mouse;
                ev.pointerKind = PointerKind::Mouse;
                ev.isPrimary = true;
                ev.pos = pos;
                ev.absolute = absolute;

                if (m_cursorRelativeMode)
                    CenterCursor();

                m_app->OnEvent(ev);
                break;
            }
            case XI_HierarchyChanged:
            {
                const XIHierarchyEvent* xev = static_cast<const XIHierarchyEvent*>(event.xcookie.data);
                for (int i = 0; i < xev->num_info; ++i)
                {
                    if (xev->info[i].flags & XISlaveRemoved)
                    {
                        m_inputDeviceCache.Erase(xev->info[i].deviceid);
                    }
                }
                break;
            }
            case XI_Motion:
            {
                if (m_deviceInfo.hasHighDefMouse)
                    break;

                const XIDeviceEvent* xev = static_cast<const XIDeviceEvent*>(event.xcookie.data);

                if (HasFlag(xev->flags, XIPointerEmulated))
                    break;

                ViewImpl* view = GetViewFromWindow(xev->event);

                PointerMoveEvent ev(view);
                ev.pointerId = PointerId_Mouse;
                ev.pointerKind = PointerKind::Mouse;
                ev.isPrimary = true;
                ev.pos = { static_cast<float>(xev->root_x), static_cast<float>(xev->root_y) };
                ev.absolute = true;

                if (m_cursorRelativeMode)
                    CenterCursor();

                m_app->OnEvent(ev);
                break;
            }
            case XI_TouchBegin:
            {

                const XIDeviceEvent* xev = static_cast<const XIDeviceEvent*>(event.xcookie.data);
                ViewImpl* view = GetViewFromWindow(xev->event);

                if (!HasFlag(view->m_flags, ViewFlag::AcceptTouch))
                    break;

                if (view->m_primaryTouchId == 0)
                    view->m_primaryTouchId = xev->detail;

                PointerDownEvent ev(view);
                ev.pointerId = static_cast<PointerId>(xev->detail);
                ev.pointerKind = PointerKind::Touch;
                ev.size = { 5, 5 }; // No size from xinput so we use a 'reasonable' size
                ev.pressure = 1.0f;
                ev.isPrimary = view->m_primaryTouchId == xev->detail;
                ev.button = PointerButton::Primary;

                if (m_cursorRelativeMode)
                    CenterCursor();

                view->TrackCapture(ev);
                m_app->OnEvent(ev);
                break;
            }
            case XI_TouchEnd:
            {
                const XIDeviceEvent* xev = static_cast<const XIDeviceEvent*>(event.xcookie.data);
                ViewImpl* view = GetViewFromWindow(xev->event);

                if (!HasFlag(view->m_flags, ViewFlag::AcceptTouch))
                    break;

                HE_ASSERT(view->m_primaryTouchId != 0);

                PointerUpEvent ev(view);
                ev.pointerId = static_cast<PointerId>(xev->detail);
                ev.pointerKind = PointerKind::Touch;
                ev.size = { 5, 5 }; // No size from xinput so we use a 'reasonable' size
                ev.pressure = 1.0f;
                ev.isPrimary = view->m_primaryTouchId == xev->detail;
                ev.button = PointerButton::Primary;

                if (m_cursorRelativeMode)
                    CenterCursor();

                if (ev.isPrimary)
                    view->m_primaryTouchId = 0;

                view->TrackCapture(ev);
                m_app->OnEvent(ev);
                break;
            }
            case XI_TouchUpdate:
            {
                const XIDeviceEvent* xev = static_cast<const XIDeviceEvent*>(event.xcookie.data);
                ViewImpl* view = GetViewFromWindow(xev->event);

                if (!HasFlag(view->m_flags, ViewFlag::AcceptTouch))
                    break;

                HE_ASSERT(view->m_primaryTouchId != 0);

                PointerMoveEvent ev(view);
                ev.pointerId = static_cast<PointerId>(xev->detail);
                ev.pointerKind = PointerKind::Touch;
                ev.size = { 5, 5 }; // No size from xinput so we use a 'reasonable' size
                ev.pressure = 1.0f;
                ev.isPrimary = view->m_primaryTouchId == xev->detail;
                ev.pos = { static_cast<float>(xev->root_x), static_cast<float>(xev->root_y) };
                ev.absolute = true;

                if (m_cursorRelativeMode)
                    CenterCursor();

                m_app->OnEvent(ev);
                break;
            }
        }

        m_XFreeEventData(m_display, &event.xcookie);
    }

    void DeviceImpl::HandleNonViewXEvent(XEvent& event)
    {
        switch (event.type)
        {
            case MappingNotify:
            {
                const int request = event.xmapping.request;
                if (request == MappingKeyboard || request == MappingModifier)
                {
                    m_XRefreshKeyboardMapping(&event.xmapping);
                }
                break;
            }
        }
    }

    void DeviceImpl::HandleViewXEvent(ViewImpl* view, XEvent& event)
    {
        switch (event.type)
        {
            case EnterNotify:
            {
                if (m_deviceInfo.hasHighDefMouse)
                    break;

                PointerMoveEvent ev(view);
                ev.pointerId = PointerId_Mouse;
                ev.pointerKind = PointerKind::Mouse;
                ev.isPrimary = true;
                ev.pos = { static_cast<float>(event.xcrossing.x), static_cast<float>(event.xcrossing.y) };
                ev.absolute = true;
                m_app->OnEvent(ev);
                break;
            }
            case ClientMessage:
            {
                if (event.xclient.message_type == m_atomWMProtocols)
                {
                    const Atom protocol = event.xclient.data.l[0];
                    if (protocol == m_atomWMDeleteWindow)
                    {
                        ViewRequestCloseEvent ev(view);
                        m_app->OnEvent(ev);
                    }
                    else if (protocol == m_atomNetWMPing)
                    {
                        XEvent reply = event;
                        reply.xclient.window = m_root;
                        m_XSendEvent(m_display, m_root, False, SubstructureNotifyMask | SubstructureRedirectMask, &reply);
                    }
                }
                else if (event.xclient.message_type == m_atomXdndEnter)
                {
                    const bool useList = HasFlag(event.xclient.data.l[1], 1);

                    m_dndSource  = event.xclient.data.l[0];
                    m_dndVersion = event.xclient.data.l[1] >> 24;
                    m_dndFormat  = X11_None;

                    if (m_dndVersion > SupportedX11DndVersion)
                        return;

                    Atom* formats = nullptr;
                    uint64_t formatCount = 0;
                    if (useList)
                    {
                        formatCount = ReadWindowProperty(m_dndSource, m_atomXdndTypeList, XA_ATOM, reinterpret_cast<uint8_t**>(&formats));
                    }
                    else
                    {
                        formatCount = 3;
                        formats = reinterpret_cast<Atom*>(event.xclient.data.l + 2);
                    }

                    for (uint64_t i = 0; i < formatCount; ++i)
                    {
                        if (formats[i] == m_atomTextUriList)
                        {
                            m_dndFormat = m_atomTextUriList;
                            break;
                        }
                    }

                    if (useList && formats)
                        m_XFree(formats);

                    ViewDndStartEvent ev(view);
                    m_app->OnEvent(ev);
                }
                else if (event.xclient.message_type == m_atomXdndLeave)
                {
                    if (m_dndVersion > SupportedX11DndVersion)
                        return;

                    m_dndSource = X11_None;
                    m_dndVersion = 0;
                    m_dndFormat = X11_None;

                    ViewDndEndEvent ev(view);
                    m_app->OnEvent(ev);
                }
                else if (event.xclient.message_type == m_atomXdndDrop)
                {
                    if (m_dndVersion > SupportedX11DndVersion)
                        return;

                    Time time = CurrentTime;

                    if (m_dndFormat != X11_None)
                    {
                        if (m_dndVersion >= 1)
                            time = event.xclient.data.l[2];

                        m_XConvertSelection(m_display, m_atomXdndSelection, m_dndFormat, m_atomXdndSelection, view->m_window, time);
                    }
                    else
                    {
                        if (m_dndVersion >= 2)
                        {
                            XEvent reply = { ClientMessage };
                            reply.xclient.window = m_dndSource;
                            reply.xclient.message_type = m_atomXdndFinished;
                            reply.xclient.format = 32;
                            reply.xclient.data.l[0] = view->m_window;
                            reply.xclient.data.l[1] = 0; // The drag was rejected
                            reply.xclient.data.l[2] = X11_None;

                            m_XSendEvent(m_display, m_dndSource, False, NoEventMask, &reply);
                            m_XFlush(m_display);
                        }

                        ViewDndEndEvent ev(view);
                        m_app->OnEvent(ev);
                    }
                }
                else if (event.xclient.message_type == m_atomXdndPosition)
                {
                    if (m_dndVersion > SupportedX11DndVersion)
                        return;

                    XEvent reply = { ClientMessage };
                    reply.xclient.window = m_dndSource;
                    reply.xclient.message_type = m_atomXdndStatus;
                    reply.xclient.format = 32;
                    reply.xclient.data.l[0] = view->m_window;
                    reply.xclient.data.l[1] = 0; // reject the drag, we'll change to accept below
                    reply.xclient.data.l[2] = 0; // empty rectangle
                    reply.xclient.data.l[3] = 0;

                    if (m_dndFormat)
                    {
                        const ViewDropEffect effect = m_app->OnDragging(view);
                        Atom action = X11_None;
                        switch (effect)
                        {
                            case ViewDropEffect::Reject: break;
                            case ViewDropEffect::Copy: action = m_atomXdndActionCopy; break;
                            case ViewDropEffect::Move: action = m_atomXdndActionMove; break;
                            case ViewDropEffect::Link: action = m_atomXdndActionLink; break;
                        }

                        if (action != X11_None)
                        {
                            reply.xclient.data.l[1] = 1; // Accept with no rectangle
                            if (m_dndVersion >= 2)
                                reply.xclient.data.l[4] = action;
                        }
                    }

                    m_XSendEvent(m_display, m_dndSource, False, NoEventMask, &reply);
                    m_XFlush(m_display);

                    ViewDndMoveEvent ev(view);
                    ev.pos = { static_cast<float>(event.xclient.data.l[2] >> 16), static_cast<float>(event.xclient.data.l[2] & 0xffff) };
                    m_app->OnEvent();
                }
                break;
            }
            case ConfigureNotify:
            {
                Vec2i pos{ event.xconfigure.x, event.xconfigure.y };
                if (view->m_pos != pos)
                {
                    view->m_pos = pos;
                    ViewMovedEvent ev(view);
                    ev.pos = view->m_pos;
                    m_app->OnEvent(ev);
                }

                Vec2i size{ event.xconfigure.width, event.xconfigure.height };
                if (view->m_size != size)
                {
                    view->m_size = size;
                    ViewResizedEvent ev(view);
                    ev.size = view->m_size;
                    m_app->OnEvent(ev);
                }
                break;
            }
            case ButtonPress:
            {
                PointerButton button = PointerButton::None;
                switch (event.xbutton.button)
                {
                    case Button1: button = PointerButton::Primary; break;
                    case Button2: button = PointerButton::Auxiliary; break;
                    case Button3: button = PointerButton::Secondary; break;
                    case Button4:
                    case Button5:
                    {
                        const float delta = event.xbutton.button == Button4 ? 1.0f : -1.0f;
                        PointerWheelEvent ev(view);
                        ev.pointerId = PointerId_Mouse;
                        ev.pointerKind = PointerKind::Mouse;
                        ev.isPrimary = true;
                        ev.delta = { 0, delta };
                        m_app->OnEvent(ev);
                        break;
                    }
                    case 6: // Button6
                    case 7: // Button7
                    {
                        const float delta = event.xbutton.button == 6 ? -1.0f : 1.0f;
                        PointerWheelEvent ev(view);
                        ev.pointerId = PointerId_Mouse;
                        ev.pointerKind = PointerKind::Mouse;
                        ev.isPrimary = true;
                        ev.delta = { delta, 0 };
                        m_app->OnEvent(ev);
                        break;
                    }
                }

                if (button != PointerButton::None)
                {
                    PointerDownEvent ev(view);
                    ev.pointerId = PointerId_Mouse;
                    ev.pointerKind = PointerKind::Mouse;
                    ev.isPrimary = true;
                    ev.button = button;
                    view->TrackCapture(ev);
                    m_app->OnEvent(ev);
                }

                break;
            }
            case ButtonRelease:
            {
                PointerButton button = PointerButton::None;
                switch (event.xbutton.button)
                {
                    case Button1: button = PointerButton::Primary; break;
                    case Button2: button = PointerButton::Auxiliary; break;
                    case Button3: button = PointerButton::Secondary; break;
                }

                if (button != PointerButton::None)
                {
                    PointerUpEvent ev(view);
                    ev.pointerId = PointerId_Mouse;
                    ev.pointerKind = PointerKind::Mouse;
                    ev.isPrimary = true;
                    ev.button = button;
                    view->TrackCapture(ev);
                    m_app->OnEvent(ev);
                }
                break;
            }
            case MotionNotify:
            {
                if (m_deviceInfo.hasHighDefMouse)
                    break;

                Vec2f pos = { static_cast<float>(event.xmotion.x), static_cast<float>(event.xmotion.y) };
                PointerMoveEvent ev(view);
                ev.pointerId = PointerId_Mouse;
                ev.pointerKind = PointerKind::Mouse;
                ev.isPrimary = true;
                ev.pos = pos;
                ev.absolute = true;

                if (m_cursorRelativeMode)
                    CenterCursor();

                m_app->OnEvent(ev);
                break;
            }
            case KeyPress:
            {
                KeySym keysym = m_XLookupKeysym(&event.xkey, 0);
                Key key = TranslateKey(keysym);

                if (key != Key::None)
                {
                    KeyDownEvent ev(view);
                    ev.key = key;
                    m_app->OnEvent(ev);
                }

                Status status = 0;
                char32_t ch = 0;
                int len = m_Xutf8LookupString(view->m_ic, &event.xkey, reinterpret_cast<char*>(&ch), sizeof(ch), nullptr, &status);

                if (status == XLookupChars || status == XLookupBoth)
                {
                    if (len > 0)
                    {
                        TextEvent ev(view);
                        ev.ch = ch;
                        m_app->OnEvent(ev);
                    }
                }
                break;
            }
            case KeyRelease:
            {
                KeySym keysym = m_XLookupKeysym(&event.xkey, 0);
                Key key = TranslateKey(keysym);

                // If detectable auto repeat is not available then the X server will send us a
                // KeyRelease for each repeated KeyPress, which is not desireable. So in this case
                // we attempt to detect the simulated KeyRelease events by checking for a KeyPress
                // that follows it for the same key in very little time.
                if (!m_hasDetectableAutoRepeat)
                {
                    XEvent next;
                    m_XPeekEvent(m_display, &next);

                    if (next.type == KeyPress
                        && next.xkey.window == event.xkey.window
                        && next.xkey.keycode == event.xkey.keycode)
                    {
                        const Time diff = next.xkey.time - event.xkey.time;
                        if (diff < 10)
                            break;
                    }
                }

                if (key != Key::None)
                {
                    KeyUpEvent ev(view);
                    ev.key = key;
                    m_app->OnEvent(ev);
                }
                break;
            }
            case FocusIn:
            case FocusOut:
            {
                if (view->m_mouseCaptureCount)
                {
                    view->ReleaseMouse();
                    view->m_mouseCaptureCount = 0;
                }

                if (view->m_touchCaptureCount)
                {
                    view->ReleaseTouch();
                    view->m_touchCaptureCount = 0;
                }

                if (m_app)
                {
                    const bool active = event.type == FocusIn;
                    ViewActivatedEvent ev(view);
                    ev.active = active;
                    m_app->OnEvent(ev);
                }
                break;
            }
            case SelectionNotify:
            {
                if (event.xselection.property != m_atomXdndSelection || event.xselection.target != m_dndFormat)
                    break;

                char* data = nullptr;
                const uint64_t result = ReadWindowProperty(
                    event.xselection.requestor,
                    event.xselection.property,
                    event.xselection.target,
                    reinterpret_cast<uint8_t**>(&data));

                if (result)
                {
                    char* p = nullptr;
                    char* line = strtok_r(data, "\r\n", &p);
                    while (line)
                    {
                        const char* path = UriToFilePath(line);
                        if (path)
                        {
                            ViewDropFileEvent ev(view);
                            ev.path = path;
                            m_app->OnEvent(ev);
                        }
                        line = strtok_r(nullptr, "\r\n", &p);
                    }
                    ViewDropFileCompleteEvent ev(view);
                    m_app->OnEvent(ev);

                    m_XFree(data);
                }

                if (m_dndVersion >= 2)
                {
                    const ViewDropEffect effect = m_app->OnDragging(view);
                    Atom action = X11_None;
                    switch (effect)
                    {
                        case ViewDropEffect::Reject: break;
                        case ViewDropEffect::Copy: action = m_atomXdndActionCopy; break;
                        case ViewDropEffect::Move: action = m_atomXdndActionMove; break;
                        case ViewDropEffect::Link: action = m_atomXdndActionLink; break;
                    }

                    XEvent reply = { ClientMessage };
                    reply.xclient.window = m_dndSource;
                    reply.xclient.message_type = m_atomXdndFinished;
                    reply.xclient.format = 32;
                    reply.xclient.data.l[0] = view->m_window;
                    reply.xclient.data.l[1] = result;
                    reply.xclient.data.l[2] = action;

                    m_XSendEvent(m_display, m_dndSource, False, NoEventMask, &reply);
                    m_XFlush(m_display);
                }

                ViewDndEndEvent ev(view);
                m_app->OnEvent(ev);
                break;
            }
        }
    }

    uint64_t DeviceImpl::ReadWindowProperty(Window window, Atom property, Atom type, uint8_t** value)
    {
        Atom actualType = X11_None;
        int actualFormat = 0;
        uint64_t itemCount = 0;
        uint64_t bytesAfter = 0;;

        m_XGetWindowProperty(m_display,
                        window,
                        property,
                        0,
                        LONG_MAX,
                        False,
                        type,
                        &actualType,
                        &actualFormat,
                        &itemCount,
                        &bytesAfter,
                        value);

        return itemCount;
    }

    DeviceImpl::InputDeviceInfo& DeviceImpl::FindInputDeviceInfo(int deviceId)
    {
        const auto result = m_inputDeviceCache.Emplace(deviceId);
        const bool isNew = result.inserted;
        InputDeviceInfo& info = result.entry;

        if (!isNew)
            return info;

        HE_ASSERT(m_XIQueryDevice);
        int deviceCount = 0;
        XIDeviceInfo* device = m_XIQueryDevice(m_display, deviceId, &deviceCount);
        HE_ASSERT(deviceCount == 1);

        info.deviceId = deviceId;

        uint32_t axis = 0;
        for (int i = 0; i < device->num_classes; ++i)
        {
            const XIValuatorClassInfo* val = reinterpret_cast<const XIValuatorClassInfo*>(device->classes[i]);
            if (val->type == XIValuatorClass)
            {
                info.relative[axis++] = val->mode == XIModeRelative;

                if (axis == HE_LENGTH_OF(info.relative))
                    break;
            }
        }

        return info;
    }
}

namespace he::window
{
    Device* _CreateWindowDevice(Allocator& allocator)
    {
        return allocator.New<linux::DeviceImpl>(allocator);
    }
}

#endif
