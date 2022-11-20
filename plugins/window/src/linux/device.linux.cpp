// Copyright Chad Engler

#include "device.linux.h"

#include "gamepad.linux.h"
#include "view.linux.h"

#include "he/core/log.h"
#include "he/window/event.h"
#include "he/window/key.h"

#if defined(HE_PLATFORM_LINUX)

#include <dlfcn.h>
#include <fcntl.h>

#include "x11_all.h"

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

    DeviceImpl::DeviceImpl(Allocator& allocator) noexcept
        : Device(allocator)
        , m_gamepads{ { this, 0 }, { this, 1 }, { this, 2 }, { this, 3 } }
    {}

    DeviceImpl::~DeviceImpl() noexcept
    {
        for (int32_t& fd : m_gamepadFds)
        {
            if (fd != -1)
            {
                close(fd);
                fd = -1;
            }
        }

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

                for (uint32_t i = 0; i < static_cast<uint32_t>(MouseCursor::_Count); ++i)
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
        m_atomWMState = m_XInternAtom(m_display, "WM_STATE", False);

        m_root = RootWindow(m_display, DefaultScreen(m_display));
        m_context = ((XContext)m_XrmUniqueQuark()); // XUniqueContext();

        // Attempt to enable detectable auto-repeat so we can tell when X11 is simulating
        // repeated key presses and prevent it from sending virtual key up events.
        if (m_XkbSetDetectableAutoRepeat)
        {
            Bool supported = False;
            Bool enabled = m_XkbSetDetectableAutoRepeat(m_display, true, &supported);

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

        // Detect raw mouse input support
        m_xi = dlopen("libXi.so.6", RTLD_LAZY | RTLD_LOCAL);
        if (m_xi)
        {
            m_XIQueryVersion = reinterpret_cast<Pfn_XIQueryVersion>(dlsym(m_xi, "XIQueryVersion"));
            m_XISelectEvents = reinterpret_cast<Pfn_XISelectEvents>(dlsym(m_xi, "XISelectEvents"));

            if (m_XIQueryVersion && m_XISelectEvents)
            {
                int eventBase, errorBase;
                if (m_XQueryExtension(m_display, "XInputExtension", &m_xiMajorOpcode, &eventBase, &errorBase))
                {
                    int majorVer, minorVer;
                    if (m_XIQueryVersion(m_display, &majorVer, &minorVer) == Success)
                    {
                        m_hasHighDefMouse = true;
                    }
                }
            }
        }

        // Enable raw mouse input support
        if (m_hasHighDefMouse)
        {
            XIEventMask em;
            uint8_t mask[XIMaskLen(XI_RawMotion)]{};

            em.deviceid = XIAllMasterDevices;
            em.mask_len = sizeof(mask);
            em.mask = mask;
            XISetMask(mask, XI_RawMotion);

            m_XISelectEvents(m_display, m_root, &em, 1);
        }

        // Create hidden cursor
        const char data[8]{};
        XColor dummy;
        m_hiddenCursorBitmap = m_XCreateBitmapFromData(m_display, m_root, data, 8, 8);
        m_hiddenCursor = m_XCreatePixmapCursor(m_display, m_hiddenCursorBitmap, m_hiddenCursorBitmap, &dummy, &dummy, 0, 0);

        // Create standard cursors
        m_cursors[static_cast<uint32_t>(MouseCursor::Arrow)] = m_XCreateFontCursor(m_display, XC_left_ptr);
        m_cursors[static_cast<uint32_t>(MouseCursor::Hand)] = m_XCreateFontCursor(m_display, XC_hand2);
        m_cursors[static_cast<uint32_t>(MouseCursor::NotAllowed)] = m_XCreateFontCursor(m_display, XC_X_cursor);
        m_cursors[static_cast<uint32_t>(MouseCursor::TextInput)] = m_XCreateFontCursor(m_display, XC_xterm);
        m_cursors[static_cast<uint32_t>(MouseCursor::ResizeAll)] = m_XCreateFontCursor(m_display, XC_fleur);
        m_cursors[static_cast<uint32_t>(MouseCursor::ResizeTopLeft)] = m_XCreateFontCursor(m_display, XC_top_left_corner);
        m_cursors[static_cast<uint32_t>(MouseCursor::ResizeTopRight)] = m_XCreateFontCursor(m_display, XC_top_right_corner);
        m_cursors[static_cast<uint32_t>(MouseCursor::ResizeBottomLeft)] = m_XCreateFontCursor(m_display, XC_bottom_left_corner);
        m_cursors[static_cast<uint32_t>(MouseCursor::ResizeBottomRight)] = m_XCreateFontCursor(m_display, XC_bottom_right_corner);
        m_cursors[static_cast<uint32_t>(MouseCursor::ResizeHorizontal)] = m_XCreateFontCursor(m_display, XC_sb_h_double_arrow);
        m_cursors[static_cast<uint32_t>(MouseCursor::ResizeVertical)] = m_XCreateFontCursor(m_display, XC_sb_v_double_arrow);
        m_cursors[static_cast<uint32_t>(MouseCursor::Wait)] = m_XCreateFontCursor(m_display, XC_watch);

        return true;
    }

    int DeviceImpl::Run(Application& app, const ViewDesc& desc)
    {
        m_app = &app;

        // Create root window
        ViewImpl view(this, desc);
        view.SetVisible(true, true);

        // Open gamepad files
        for (GamepadImpl& pad : m_gamepads)
        {
            pad.Open();
        }

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
                pad.Update();
            }

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

    bool DeviceImpl::HasHighDefMouse() const
    {
        return m_hasHighDefMouse;
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
        Bool result = m_XQueryPointer(m_display, win, &rootWin, &childWin, &rootX, &rootY, &winX, &winY, &mask);

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

    void DeviceImpl::SetCursor(MouseCursor cursor)
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
        if (!m_cursorVisible || m_viewClipped || m_cursor <= MouseCursor::None || m_cursor >= MouseCursor::_Count)
            return m_hiddenCursor;

        return m_cursors[static_cast<int32_t>(m_cursor)];
    }

    void DeviceImpl::HandleXEvent(XEvent& event)
    {
        if (event.type == GenericEvent)
        {
            if (m_hasHighDefMouse
                && event.xcookie.extension == m_xiMajorOpcode
                && m_XGetEventData(m_display, &event.xcookie)
                && event.xcookie.evtype == XI_RawMotion)
            {
                XIRawEvent* raw = static_cast<XIRawEvent*>(event.xcookie.data);
                if (raw->valuators.mask_len)
                {
                    Vec2f pos{};
                    const double* values = raw->raw_values;

                    if (XIMaskIsSet(raw->valuators.mask, 0))
                        pos.x = static_cast<float>(values[0]);

                    if (XIMaskIsSet(raw->valuators.mask, 1))
                        pos.y = static_cast<float>(values[1]);

                    MouseMoveEvent ev(GetFocusedView(), pos, false);

                    if (m_cursorRelativeMode)
                        CenterCursor();

                    m_app->OnEvent(ev);
                }
            }

            m_XFreeEventData(m_display, &event.xcookie);
            return;
        }

        ViewImpl* view = GetViewFromWindow(event.xany.window);

        if (!view)
            return;

        switch (event.type)
        {
            case EnterNotify:
            {
                if (m_hasHighDefMouse)
                    break;

                Vec2f pos{ static_cast<float>(event.xcrossing.x), static_cast<float>(event.xcrossing.y) };
                MouseMoveEvent ev(view, pos, true);
                m_app->OnEvent(ev);
                break;
            }
            case ClientMessage:
            {
                if (static_cast<Atom>(event.xclient.data.l[0]) == m_atomWMDeleteWindow)
                {
                    ViewRequestCloseEvent ev(view);
                    m_app->OnEvent(ev);
                }
                break;
            }
            case ConfigureNotify:
            {
                Vec2i pos{ event.xconfigure.x, event.xconfigure.y };
                if (view->m_pos != pos)
                {
                    view->m_pos = pos;
                    ViewMovedEvent ev(view, view->m_pos);
                    m_app->OnEvent(ev);
                }

                Vec2i size{ event.xconfigure.width, event.xconfigure.height };
                if (view->m_size != size)
                {
                    view->m_size = size;
                    ViewResizedEvent ev(view, view->m_size);
                    m_app->OnEvent(ev);
                }
                break;
            }
            case ButtonPress:
            {
                MouseButton button = MouseButton::None;
                switch (event.xbutton.button)
                {
                    case Button1: button = MouseButton::Left; break;
                    case Button2: button = MouseButton::Middle; break;
                    case Button3: button = MouseButton::Right; break;
                    case Button4:
                    case Button5:
                    {
                        const float delta = event.xbutton.button == Button4 ? 1.0f : -1.0f;
                        MouseWheelEvent ev(view, { 0, delta });
                        m_app->OnEvent(ev);
                        break;
                    }
                    case 6: // Button6
                    case 7: // Button7
                    {
                        const float delta = event.xbutton.button == 6 ? 1.0f : -1.0f;
                        MouseWheelEvent ev(view, { delta, 0 });
                        m_app->OnEvent(ev);
                        break;
                    }
                }

                if (button != MouseButton::None)
                {
                    MouseDownEvent ev(view, button);
                    view->TrackCapture(ev);
                    m_app->OnEvent(ev);
                }

                break;
            }
            case ButtonRelease:
            {
                MouseButton button = MouseButton::None;
                switch (event.xbutton.button)
                {
                    case Button1: button = MouseButton::Left; break;
                    case Button2: button = MouseButton::Middle; break;
                    case Button3: button = MouseButton::Right; break;
                }

                if (button != MouseButton::None)
                {
                    MouseUpEvent ev(view, button);
                    view->TrackCapture(ev);
                    m_app->OnEvent(ev);
                }
                break;
            }
            case MotionNotify:
            {
                if (m_hasHighDefMouse)
                    break;

                Vec2f pos = { static_cast<float>(event.xmotion.x), static_cast<float>(event.xmotion.y) };
                MouseMoveEvent ev(view, pos, true);

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
                    KeyDownEvent ev(view, key);
                    m_app->OnEvent(ev);
                }

                Status status = 0;
                char32_t ch = 0;
                int len = m_Xutf8LookupString(view->m_ic, &event.xkey, reinterpret_cast<char*>(&ch), sizeof(ch), nullptr, &status);

                if (status == XLookupChars || status == XLookupBoth)
                {
                    if (len > 0)
                    {
                        TextEvent ev(view, ch);
                        m_app->OnEvent(ev);
                    }
                }
                break;
            }
            case KeyRelease:
            {
                // TODO: Check if `m_hasDetectableAutoRepeat == false` and skip KeyRelease
                // events that are being repeated.
                // Example: https://github.com/glfw/glfw/blob/4afa227a056681d2628894b0893527bf69496a41/src/x11_window.c#L1335
                KeySym keysym = m_XLookupKeysym(&event.xkey, 0);
                Key key = TranslateKey(keysym);

                if (key != Key::None)
                {
                    KeyUpEvent ev(view, key);
                    m_app->OnEvent(ev);
                }
                break;
            }
            case KeymapNotify:
                m_XRefreshKeyboardMapping(&event.xmapping);
                break;
            case FocusIn:
            case FocusOut:
            {
                if (view->m_captureCount)
                {
                    view->ReleaseMouse();
                    view->m_captureCount = 0;
                }

                const bool active = event.type == FocusIn;
                ViewActivatedEvent ev(view, active);
                if (m_app)
                    m_app->OnEvent(ev);
                break;
            }
        }
    }

    void DeviceImpl::UpdateGamepads()
    {
        js_event js{};

        for (uint32_t i = 0; i < MaxGamepads; ++i)
        {
            int32_t fd = m_gamepadFds[i];

            if (fd == -1)
                continue;

            while (read(fd, &js, sizeof(js)) != -1)
            {
                // don't care if this is an init event, so just mask it off
                uint8_t type = js.type & ~JS_EVENT_INIT;

                switch (type)
                {
                    case JS_EVENT_AXIS:
                    {
                        // Normalize the axis value
                        float value = js.value / 32767.0f;

                        GamepadAxis axis = GamepadAxis::None;
                        switch (js.number)
                        {
                            case 0: axis = GamepadAxis::LThumbX; break;
                            case 1: axis = GamepadAxis::LThumbY; break;
                            case 2: axis = GamepadAxis::RThumbX; break;
                            case 3: axis = GamepadAxis::RThumbY; break;
                            case 4: axis = GamepadAxis::RThumbX; break;
                            case 5: axis = GamepadAxis::RThumbY; break;
                        }

                        if (axis != GamepadAxis::None)
                        {
                            GamepadAxisEvent ev(i, axis, value);
                            m_app->OnEvent(ev);
                        }
                        break;
                    }
                    case JS_EVENT_BUTTON:
                    {
                        GamepadButton button = GamepadButton::None;
                        switch (js.number)
                        {
                            case 0: button = GamepadButton::Action1; break;
                            case 1: button = GamepadButton::Action2; break;
                            case 2: button = GamepadButton::Action3; break;
                            case 3: button = GamepadButton::Action4; break;
                            case 4: button = GamepadButton::LShoulder; break;
                            case 5: button = GamepadButton::RShoulder; break;
                            case 6: button = GamepadButton::Back; break;
                            case 7: button = GamepadButton::Start; break;
                            // case 8: button = GamepadButton::Guide; break;
                            case 9: button = GamepadButton::LThumb; break;
                            case 10: button = GamepadButton::RThumb; break;
                            case 11: button = GamepadButton::DPad_Up; break;
                            case 12: button = GamepadButton::DPad_Down; break;
                            case 13: button = GamepadButton::DPad_Left; break;
                            case 14: button = GamepadButton::DPad_Right; break;
                        }

                        if (button != GamepadButton::None)
                        {
                            if (js.value == 1)
                            {
                                GamepadButtonDownEvent ev(i, button);
                                m_app->OnEvent(ev);
                            }
                            else
                            {
                                GamepadButtonUpEvent ev(i, button);
                                m_app->OnEvent(ev);
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
}

namespace he::window
{
    Device* _CreateDevice(Allocator& allocator)
    {
        return allocator.New<linux::DeviceImpl>(allocator);
    }
}

#endif
