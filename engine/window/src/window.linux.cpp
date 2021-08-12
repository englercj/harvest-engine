// Copyright Chad Engler
// TODO: check for newly connected/disconnected gamepads
// TODO: View Flags - AcceptInput, FocusOnClick, TaskBarIcon
// TODO: drag and drop (AcceptFiles)

#include "he/window/application.h"
#include "he/window/device.h"
#include "he/window/event.h"
#include "he/window/gamepad.h"
#include "he/window/key.h"
#include "he/window/mouse.h"
#include "he/window/view.h"

#include "he/core/macros.h"
#include "he/core/log.h"
#include "he/math/vec2.h"

#include <atomic>
#include <array>
#include <climits>
#include <cstdint>

// xlib headers define None, so we can't use ::None in this file after including
// those headers. This hacks around that by giving us another symbol to use.
namespace he::window
{
    constexpr Key Key_None = Key::None;
    constexpr MouseCursor MouseCursor_None = MouseCursor::None;
    constexpr MouseButton MouseButton_None = MouseButton::None;
    constexpr GamepadAxis GamepadAxis_None = GamepadAxis::None;
    constexpr GamepadButton GamepadButton_None = GamepadButton::None;
}

#if defined(HE_PLATFORM_LINUX)

#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput2.h>

namespace he::window::Linux
{
    class DeviceImpl;
    class ViewImpl;

    // ------------------------------------------------------------------------------------------------
    using Pfn_XChangeProperty = int(*)(Display*,Window,Atom,Atom,int,int,const unsigned char*,int);
    using Pfn_XCloseDisplay = int(*)(Display*);
    using Pfn_XCloseIM = Status(*)(XIM);
    using Pfn_XCreateFontCursor = Cursor(*)(Display*,unsigned int);
    using Pfn_XCreateBitmapFromData = Pixmap(*)(Display*,Drawable,const char*,unsigned int,unsigned int);
    using Pfn_XCreateIC = XIC(*)(XIM,...);
    using Pfn_XCreatePixmapCursor = Cursor(*)(Display*,Pixmap,Pixmap,XColor*,XColor*,unsigned int,unsigned int);
    using Pfn_XCreateWindow = Window(*)(Display*,Window,int,int,unsigned int,unsigned int,unsigned int,int,unsigned int,Visual*,unsigned long,XSetWindowAttributes*);
    using Pfn_XDefineCursor = int(*)(Display*,Window,Cursor);
    using Pfn_XDeleteContext = int(*)(Display*,XID,XContext);
    using Pfn_XDestroyIC = void(*)(XIC);
    using Pfn_XDestroyWindow = int(*)(Display*,Window);
    using Pfn_XFilterEvent = Bool(*)(XEvent*,Window);
    using Pfn_XFindContext = int(*)(Display*,XID,XContext,XPointer*);
    using Pfn_XFlush = int(*)(Display*);
    using Pfn_XFree = int(*)(void*);
    using Pfn_XFreeCursor = int(*)(Display*,Cursor);
    using Pfn_XFreeEventData = void(*)(Display*,XGenericEventCookie*);
    using Pfn_XFreePixmap = int(*)(Display*,Pixmap);
    using Pfn_XGetEventData = Bool(*)(Display*,XGenericEventCookie*);
    using Pfn_XGetInputFocus = int(*)(Display*,Window*,int*);
    using Pfn_XGetWMNormalHints = Status(*)(Display*,Window,XSizeHints*,long*);
    using Pfn_XGetWindowAttributes = Status(*)(Display*,Window,XWindowAttributes*);
    using Pfn_XGetWindowProperty = int(*)(Display*,Window,Atom,long,long,Bool,Atom,Atom*,int*,unsigned long*,unsigned long*,unsigned char**);
    using Pfn_XGrabPointer = int(*)(Display*,Window,Bool,unsigned int,int,int,Window,Cursor,Time);
    using Pfn_XIconifyWindow = Status(*)(Display*,Window,int);
    using Pfn_XInitThreads = Status(*)(void);
    using Pfn_XInternAtom = Atom(*)(Display*,const char*,Bool);
    using Pfn_XLookupKeysym = KeySym(*)(XKeyEvent *key_event, int index);
    using Pfn_XMapRaised = int(*)(Display*,Window);
    using Pfn_XMapWindow = int(*)(Display*,Window);
    using Pfn_XMoveResizeWindow = int(*)(Display*,Window,int,int,unsigned int,unsigned int);
    using Pfn_XMoveWindow = int(*)(Display*,Window,int,int);
    using Pfn_XNextEvent = int(*)(Display*,XEvent*);
    using Pfn_XOpenDisplay = Display*(*)(const char*);
    using Pfn_XOpenIM = XIM(*)(Display*,XrmDatabase*,char*,char*);
    using Pfn_XPeekEvent = int(*)(Display*,XEvent*);
    using Pfn_XPending = int(*)(Display*);
    using Pfn_XQueryExtension = Bool(*)(Display*,const char*,int*,int*,int*);
    using Pfn_XQueryPointer = Bool(*)(Display*,Window,Window*,Window*,int*,int*,int*,int*,unsigned int*);
    using Pfn_XRaiseWindow = int(*)(Display*,Window);
    using Pfn_XRefreshKeyboardMapping = int(*)(XMappingEvent*);
    using Pfn_XResizeWindow = int(*)(Display*,Window,unsigned int,unsigned int);
    using Pfn_XSaveContext = int(*)(Display*,XID,XContext,const char*);
    using Pfn_XSendEvent = Status(*)(Display*,Window,Bool,long,XEvent*);
    using Pfn_XSetInputFocus = int(*)(Display*,Window,int,Time);
    using Pfn_XSetWMProtocols = Status(*)(Display*,Window,Atom*,int);
    using Pfn_XStoreName = int(*)(Display*,Window,const char*);
    using Pfn_XTranslateCoordinates = Bool(*)(Display*,Window,Window,int,int,int*,int*,Window*);
    using Pfn_XUngrabPointer = int(*)(Display*,Time);
    using Pfn_XUnmapWindow = int(*)(Display*,Window);
    using Pfn_XUnsetICFocus = void(*)(XIC);
    using Pfn_XWarpPointer = int(*)(Display*,Window,Window,int,int,unsigned int,unsigned int,int,int);
    using Pfn_XkbSetDetectableAutoRepeat = Bool(*)(Display*,Bool,Bool*);
    using Pfn_XrmUniqueQuark = XrmQuark(*)();
    using Pfn_Xutf8LookupString = int(*)(XIC,XKeyPressedEvent*,char*,int,KeySym*,Status*);
    using Pfn_Xutf8SetWMProperties = void(*)(Display*,Window,const char*,const char*,char**,int,XSizeHints*,XWMHints*,XClassHint*);

    using Pfn_XIQueryVersion = Status(*)(Display*,int*,int*);
    using Pfn_XISelectEvents = int(*)(Display*,Window,XIEventMask*,int);

    // --------------------------------------------------------------------------------------------
    class ViewImpl : public View
    {
    public:
        ViewImpl(DeviceImpl* device, const ViewDesc& desc);
        ~ViewImpl();

        void* GetNativeHandle() const override;
        void* GetUserData() const override;
        Vec2i GetPosition() const override;
        Vec2i GetSize() const override;
        float GetDpiScale() const override;
        bool IsFocused() const override;
        bool IsMinimized() const override;
        bool IsMaximized() const override;

        void SetPosition(const Vec2i& pos) override;
        void SetSize(const Vec2i& size) override;
        void SetVisible(bool visible, bool focus) override;
        void SetTitle(const char* text) override;
        void SetAlpha(float alpha) override;

        void Focus() override;
        void Minimize() override;
        void Maximize() override;
        void Restore() override;
        void RequestClose() override;

        Vec2f ViewToScreen(const Vec2f& pos) const override;
        Vec2f ScreenToView(const Vec2f& pos) const override;

        void TrackCapture(const MouseButtonEvent& ev);

        void CaptureMouse();
        void ReleaseMouse();

    public:
        DeviceImpl* m_device{ nullptr };
        void* m_userData{ nullptr };
        XIC m_ic{ nullptr };
        ViewFlag m_flags{ ViewFlag::Default };

        Window m_window{ None };
        int m_captureCount{ 0 };
        Vec2i m_pos{ 0, 0 };
        Vec2i m_size{ 1, 1 };

        bool m_maximized{ false };
    };

    // --------------------------------------------------------------------------------------------
    class DeviceImpl : public Device
    {
    public:
        DeviceImpl(Allocator& allocator);
        ~DeviceImpl();

        bool Initialize() override;

        int Run(Application& app, const ViewDesc& desc) override;
        void Quit(int rc) override;

        bool HasHighDefMouse() const override;

        View* CreateView(const ViewDesc& desc) override;
        void DestroyView(View* view) override;

        View* GetFocusedView() const override;
        View* GetHoveredView() const override;

        Vec2f GetCursorPos(View* view) const override;
        void SetCursorPos(View* view, const Vec2f& pos) override;

        void SetCursor(MouseCursor cursor) override;

        void SetCursorRelativeMode(bool relativeMode) override;

        uint32_t GetMonitorCount() const override;
        uint32_t GetMonitors(Monitor* monitors, uint32_t maxCount) const override;

        void ShowCursor(bool show);
        void CenterCursor();

        ViewImpl* GetViewFromWindow(Window win) const;
        Cursor GetActiveCursor() const;
        void HandleXEvent(XEvent& event);

        void OpenGamepads();
        void UpdateGamepads();

    public:
        Application* m_app{ nullptr };
        Display* m_display{ nullptr };
        Window m_root{ None };
        XContext m_context{ None };
        XIM m_im{ nullptr };
        int32_t m_xiMajorOpcode{ 0 };
        MouseCursor m_cursor{ MouseCursor::Arrow };
        Cursor m_hiddenCursor{ None };
        Pixmap m_hiddenCursorBitmap{ None };
        std::atomic<int32_t> m_returnCode{ 0 };
        std::atomic<bool> m_running{ true };
        bool m_cursorRelativeMode{ false };
        bool m_cursorVisible{ true };
        bool m_hasHighDefMouse{ false };
        bool m_hasDetectableAutoRepeat{ false };
        bool m_viewClipped{ false };
        Vec2f m_cursorRestorePosition{ 0, 0 };

        Cursor m_cursors[static_cast<int32_t>(MouseCursor::_Count)];
        Atom m_atomNetActiveWindow{ None };
        Atom m_atomNetWMPing{ None };
        Atom m_atomNetWMState{ None };
        Atom m_atomNetWMStateAbove{ None };
        Atom m_atomNetWMStateMinimized{ None };
        Atom m_atomNetWMStateMaximizedHorz{ None };
        Atom m_atomNetWMStateMaximizedVert{ None };
        Atom m_atomNetWMStateFullscreen{ None };
        Atom m_atomNetWMWindowOpacity{ None };
        Atom m_atomNetWMWindowType{ None };
        Atom m_atomNetWMWindowTypeNormal{ None };
        Atom m_atomMotifWMHints{ None };
        Atom m_atomWMDeleteWindow{ None };
        Atom m_atomWMState{ None };

        void* m_xlib{ nullptr };
        Pfn_XChangeProperty m_XChangeProperty{ nullptr };
        Pfn_XCloseDisplay m_XCloseDisplay{ nullptr };
        Pfn_XCloseIM m_XCloseIM{ nullptr };
        Pfn_XCreateFontCursor m_XCreateFontCursor{ nullptr };
        Pfn_XCreateBitmapFromData m_XCreateBitmapFromData{ nullptr };
        Pfn_XCreateIC m_XCreateIC{ nullptr };
        Pfn_XCreatePixmapCursor m_XCreatePixmapCursor{ nullptr };
        Pfn_XCreateWindow m_XCreateWindow{ nullptr };
        Pfn_XDefineCursor m_XDefineCursor{ nullptr };
        Pfn_XDeleteContext m_XDeleteContext{ nullptr };
        Pfn_XDestroyIC m_XDestroyIC{ nullptr };
        Pfn_XDestroyWindow m_XDestroyWindow{ nullptr };
        Pfn_XFilterEvent m_XFilterEvent{ nullptr };
        Pfn_XFindContext m_XFindContext{ nullptr };
        Pfn_XFlush m_XFlush{ nullptr };
        Pfn_XFree m_XFree{ nullptr };
        Pfn_XFreeCursor m_XFreeCursor{ nullptr };
        Pfn_XFreeEventData m_XFreeEventData{ nullptr };
        Pfn_XFreePixmap m_XFreePixmap{ nullptr };
        Pfn_XGetEventData m_XGetEventData{ nullptr };
        Pfn_XGetInputFocus m_XGetInputFocus{ nullptr };
        Pfn_XGetWMNormalHints m_XGetWMNormalHints{ nullptr };
        Pfn_XGetWindowAttributes m_XGetWindowAttributes{ nullptr };
        Pfn_XGetWindowProperty m_XGetWindowProperty{ nullptr };
        Pfn_XGrabPointer m_XGrabPointer{ nullptr };
        Pfn_XIconifyWindow m_XIconifyWindow{ nullptr };
        Pfn_XInitThreads m_XInitThreads{ nullptr };
        Pfn_XInternAtom m_XInternAtom{ nullptr };
        Pfn_XLookupKeysym m_XLookupKeysym{ nullptr };
        Pfn_XMapRaised m_XMapRaised{ nullptr };
        Pfn_XMapWindow m_XMapWindow{ nullptr };
        Pfn_XMoveResizeWindow m_XMoveResizeWindow{ nullptr };
        Pfn_XMoveWindow m_XMoveWindow{ nullptr };
        Pfn_XNextEvent m_XNextEvent{ nullptr };
        Pfn_XOpenDisplay m_XOpenDisplay{ nullptr };
        Pfn_XOpenIM m_XOpenIM{ nullptr };
        Pfn_XPeekEvent m_XPeekEvent{ nullptr };
        Pfn_XPending m_XPending{ nullptr };
        Pfn_XQueryExtension m_XQueryExtension{ nullptr };
        Pfn_XQueryPointer m_XQueryPointer{ nullptr };
        Pfn_XRaiseWindow m_XRaiseWindow{ nullptr };
        Pfn_XRefreshKeyboardMapping m_XRefreshKeyboardMapping{ nullptr };
        Pfn_XResizeWindow m_XResizeWindow{ nullptr };
        Pfn_XSaveContext m_XSaveContext{ nullptr };
        Pfn_XSendEvent m_XSendEvent{ nullptr };
        Pfn_XSetInputFocus m_XSetInputFocus{ nullptr };
        Pfn_XSetWMProtocols m_XSetWMProtocols{ nullptr };
        Pfn_XStoreName m_XStoreName{ nullptr };
        Pfn_XTranslateCoordinates m_XTranslateCoordinates{ nullptr };
        Pfn_XUngrabPointer m_XUngrabPointer{ nullptr };
        Pfn_XUnmapWindow m_XUnmapWindow{ nullptr };
        Pfn_XUnsetICFocus m_XUnsetICFocus{ nullptr };
        Pfn_XWarpPointer m_XWarpPointer{ nullptr };
        Pfn_XkbSetDetectableAutoRepeat m_XkbSetDetectableAutoRepeat{ nullptr };
        Pfn_XrmUniqueQuark m_XrmUniqueQuark{ nullptr };
        Pfn_Xutf8LookupString m_Xutf8LookupString{ nullptr };
        Pfn_Xutf8SetWMProperties m_Xutf8SetWMProperties{ nullptr };

        void* m_xi{ nullptr };
        Pfn_XIQueryVersion m_XIQueryVersion{ nullptr };
        Pfn_XISelectEvents m_XISelectEvents{ nullptr };

        int32_t m_gamepadFds[MaxGamepads]{};
    };

    // --------------------------------------------------------------------------------------------
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
        return Key_None;
    }

    // --------------------------------------------------------------------------------------------
    ViewImpl::ViewImpl(DeviceImpl* device, const ViewDesc& desc)
        : m_device(device)
        , m_userData(desc.userData)
        , m_flags(desc.flags)
    {
        Display* display = m_device->m_display;
        int screenNum = DefaultScreen(display);
        int depth = DefaultDepth(display, screenNum);
        Visual* visual = DefaultVisual(display, screenNum);

        Window rootWindow = m_device->m_root;
        Window parentWindow = desc.parent ? static_cast<Window>(reinterpret_cast<uintptr_t>(desc.parent)) : rootWindow;

        // Create main window
        XSetWindowAttributes attribs{};
        attribs.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask
            | PointerMotionMask | ButtonPressMask | ButtonReleaseMask
            | FocusChangeMask | EnterWindowMask;

        // VisibilityChangeMask, ExposureMask, LeaveWindowMask, PropertyChangeMask

        if (desc.parent)
        {
            XWindowAttributes parentAttribs;
            m_device->m_XGetWindowAttributes(display, parentWindow, &parentAttribs);
            depth = parentAttribs.depth;
            visual = parentAttribs.visual;
        }

        m_pos = desc.pos;
        m_size = desc.size;
        if (m_size.x == 0) m_size.x = 1920;
        if (m_size.y == 0) m_size.y = 1080;

        m_window = m_device->m_XCreateWindow(
            display,
            parentWindow,
            m_pos.x,
            m_pos.y,
            m_size.x,
            m_size.y,
            0,
            depth,
            InputOutput,
            visual,
            CWBorderPixel | CWEventMask,
            &attribs);

        HE_ASSERT(m_window != None, "Failed to create X window.");

        m_device->m_XSaveContext(display, m_window, m_device->m_context, reinterpret_cast<XPointer>(this));

        // Set motif hints for view types
        if (m_device->m_atomMotifWMHints != None)
        {
            struct ViewHints
            {
                unsigned long flags;
                unsigned long functions;
                unsigned long decorations;
                long input_mode;
                unsigned long status;
            };

            ViewHints hints{};
            hints.flags = (1L << 1); // MWM_HINTS_DECORATIONS
            if (HasFlag(view->m_flags, ViewFlag::Decorated))
                hints.decorations = (1L << 0); // MWM_DECOR_ALL
            else
                hints.decorations = 0;

            m_device->m_XChangeProperty(
                display,
                m_window,
                m_device->m_atomMotifWMHints,
                m_device->m_atomMotifWMHints,
                32,
                PropModeReplace,
                reinterpret_cast<uint8_t*>(&hints),
                sizeof(hints) / sizeof(long));
        }

        // Setup property state
        if (m_device->m_atomNetWMState != None)
        {
            Atom states[3];
            int count = 0;

            if (HasFlag(view->m_flags, ViewFlag::StartMaximized)
                && m_device->m_atomNetWMStateMaximizedHorz != None
                && m_device->m_atomNetWMStateMaximizedVert != None)
            {
                states[count++] = m_device->m_atomNetWMStateMaximizedHorz;
                states[count++] = m_device->m_atomNetWMStateMaximizedVert;
                m_maximized = true;
            }

            if (HasFlag(view->m_flags, ViewFlag::TopMost) && m_device->m_atomNetWMStateAbove != None)
            {
                states[count++] = m_device->m_atomNetWMStateAbove;
            }

            if (count)
            {
                m_device->m_XChangeProperty(display, m_window, m_device->m_atomNetWMState, XA_ATOM, 32, PropModeReplace, reinterpret_cast<uint8_t*>(states), count);
            }
        }

        // Setup supported protocols
        {
            Atom protocols[] =
            {
                m_device->m_atomWMDeleteWindow,
                m_device->m_atomNetWMPing,
            };
            m_device->m_XSetWMProtocols(display, m_window, protocols, HE_LENGTH_OF(protocols));
        }

        // Declare this is a normal window
        if (m_device->m_atomNetWMWindowType != None && m_device->m_atomNetWMWindowTypeNormal != None)
        {
            Atom type = m_device->m_atomNetWMWindowTypeNormal;
            m_device->m_XChangeProperty(display, m_window, m_device->m_atomNetWMWindowType, XA_ATOM, 32, PropModeReplace, reinterpret_cast<uint8_t*>(&type), 1);
        }

        m_ic = m_device->m_XCreateIC(m_device->m_im, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, m_device->m_root, nullptr);
        HE_ASSERT(m_ic != nullptr, "XCreateIC failed.");
    }

    ViewImpl::~ViewImpl()
    {
        if (m_ic)
            m_device->m_XDestroyIC(m_ic);

        Display* display = m_device->m_display;
        if (m_window)
        {
            m_device->m_XDeleteContext(display, m_window, m_device->m_context);
            m_device->m_XUnmapWindow(display, m_window);
            m_device->m_XDestroyWindow(display, m_window);
        }

        m_device->m_XFlush(display);
    }

    void* ViewImpl::GetNativeHandle() const
    {
        uintptr_t handle = static_cast<uintptr_t>(m_window);
        return reinterpret_cast<void*>(handle);
    }

    void* ViewImpl::GetUserData() const
    {
        return m_userData;
    }

    Vec2i ViewImpl::GetPosition() const
    {
        return m_pos;
    }

    Vec2i ViewImpl::GetSize() const
    {
        return m_size;
    }

    float ViewImpl::GetDpiScale() const
    {
        // TODO
        return 1.0f;
    }

    bool ViewImpl::IsFocused() const
    {
        Window focused = None;
        int revertTo = RevertToNone;
        m_device->m_XGetInputFocus(m_device->m_display, &focused, &revertTo);

        return focused == m_window;
    }

    bool ViewImpl::IsMinimized() const
    {
        int result = WithdrawnState;

        struct WindowState
        {
            CARD32 state;
            Window icon;
        };

        Atom actualType;
        int actualFormat;
        unsigned long itemCount = 0;
        unsigned long bytesAfter = 0;
        WindowState* state = nullptr;
        m_device->m_XGetWindowProperty(
            m_device->m_display,
            m_window,
            m_device->m_atomWMState,
            0,
            LONG_MAX,
            False,
            m_device->m_atomWMState,
            &actualType,
            &actualFormat,
            &itemCount,
            &bytesAfter,
            reinterpret_cast<uint8_t**>(&state));

        if (itemCount >= 2)
        {
            result = state->state;
        }

        if (state)
            m_device->m_XFree(state);

        return result == IconicState;
    }

    bool ViewImpl::IsMaximized() const
    {
        if (m_device->m_atomNetWMState == None
            || m_device->m_atomNetWMStateMaximizedHorz == None
            || m_device->m_atomNetWMStateMaximizedVert == None)
        {
            return false;
        }

        Atom actualType;
        int actualFormat;
        unsigned long itemCount = 0;
        unsigned long bytesAfter = 0;
        Atom* states = nullptr;
        m_device->m_XGetWindowProperty(
            m_device->m_display,
            m_window,
            m_device->m_atomNetWMState,
            0,
            LONG_MAX,
            False,
            XA_ATOM,
            &actualType,
            &actualFormat,
            &itemCount,
            &bytesAfter,
            reinterpret_cast<uint8_t**>(&states));

        bool maximized = false;
        for (uint32_t i = 0; i < itemCount; ++i)
        {
            if (states[i] == m_device->m_atomNetWMStateMaximizedHorz
                || states[i] == m_device->m_atomNetWMStateMaximizedVert)
            {
                maximized = true;
                break;
            }
        }

        if (states)
            m_device->m_XFree(states);

        return maximized;
    }

    void ViewImpl::SetPosition(const Vec2i& pos)
    {
        m_device->m_XMoveWindow(m_device->m_display, m_window, pos.x, pos.y);
    }

    void ViewImpl::SetSize(const Vec2i& size)
    {
        m_device->m_XResizeWindow(m_device->m_display, m_window, size.x, size.y);
    }

    void ViewImpl::SetVisible(bool visible, bool focus)
    {
        if (visible && focus)
            m_device->m_XMapRaised(m_device->m_display, m_window);
        else if (visible)
            m_device->m_XMapWindow(m_device->m_display, m_window);
        else
            m_device->m_XUnmapWindow(m_device->m_display, m_window);
    }

    void ViewImpl::SetTitle(const char* text)
    {
        m_device->m_XStoreName(m_device->m_display, m_window, text);
    }

    void ViewImpl::SetAlpha(float alpha)
    {
        CARD32 value = static_cast<CARD32>(0xffffffffu * static_cast<double>(alpha));
        m_device->m_XChangeProperty(
            m_device->m_display,
            m_window,
            m_device->m_atomNetWMWindowOpacity,
            XA_CARDINAL,
            32,
            PropModeReplace,
            reinterpret_cast<uint8_t*>(&value),
            1L);
    }

    void ViewImpl::Focus()
    {
        m_device->m_XRaiseWindow(m_device->m_display, m_window);
        m_device->m_XSetInputFocus(m_device->m_display, m_window, RevertToNone, CurrentTime);
    }

    void ViewImpl::Minimize()
    {
        // TODO
    }

    void ViewImpl::Maximize()
    {
        // TODO
    }

    void ViewImpl::Restore()
    {
        // TODO
    }

    void ViewImpl::RequestClose()
    {
        // TODO
    }

    Vec2f ViewImpl::ViewToScreen(const Vec2f& pos) const
    {
        const int srcX = static_cast<int>(pos.x);
        const int srcY = static_cast<int>(pos.y);
        int dstX = 0;
        int dstY = 0;
        Window childWin = None;
        Bool result = m_device->m_XTranslateCoordinates(m_device->m_display, m_window, m_device->m_root, srcX, srcY, &dstX, &dstY, &childWin);

        if (result == False)
            return pos;

        return { static_cast<float>(dstX), static_cast<float>(dstY) };
    }

    Vec2f ViewImpl::ScreenToView(const Vec2f& pos) const
    {
        const int srcX = static_cast<int>(pos.x);
        const int srcY = static_cast<int>(pos.y);
        int dstX = 0;
        int dstY = 0;
        Window childWin = None;
        Bool result = m_device->m_XTranslateCoordinates(m_device->m_display, m_device->m_root, m_window, srcX, srcY, &dstX, &dstY, &childWin);

        if (result == False)
            return pos;

        return { static_cast<float>(dstX), static_cast<float>(dstY) };
    }

    void ViewImpl::TrackCapture(const MouseButtonEvent& ev)
    {
        if (ev.type == EventType::MouseDown)
        {
            if (m_captureCount++ == 0)
                CaptureMouse();
        }
        else
        {
            if (--m_captureCount == 0)
                ReleaseMouse();
        }
    }

    void ViewImpl::CaptureMouse()
    {
        if (!m_device->m_cursorRelativeMode)
        {
            uint32_t mask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
            Cursor cursor = m_device->GetActiveCursor();
            m_device->m_XGrabPointer(m_device->m_display, m_window, True, mask, GrabModeAsync, GrabModeAsync, None, cursor, CurrentTime);
        }
    }

    void ViewImpl::ReleaseMouse()
    {
        if (!m_device->m_cursorRelativeMode)
        {
            m_device->m_XUngrabPointer(m_device->m_display, CurrentTime);
        }
    }

    // --------------------------------------------------------------------------------------------
    DeviceImpl::DeviceImpl(Allocator& allocator)
        : Device(allocator)
    {}

    DeviceImpl::~DeviceImpl()
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
                if (m_hiddenCursor != None)
                {
                    m_XFreeCursor(m_display, m_hiddenCursor);
                    m_XFreePixmap(m_display, m_hiddenCursorBitmap);
                }

                for (uint32_t i = 0; i < static_cast<uint32_t>(MouseCursor::_Count); ++i)
                {
                    if (m_cursors[i] != None)
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
            HE_LOG_ERROR(window, "Failed to load Xlib.");
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
            HE_LOG_ERROR(window, "XInitThreads failed.");
            return false;
        }

        m_display = m_XOpenDisplay(nullptr);
        if (m_display == nullptr)
        {
            HE_LOG_ERROR(window, "XOpenDisplay failed.");
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
            HE_LOG_ERROR(window, "XOpenIM failed.");
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

        // Initialized event
        {
            InitializedEvent ev(&view);
            app.OnEvent(ev);
        }

        OpenGamepads();

        // Event loop
        while (m_running.load())
        {
            UpdateGamepads();

            while (m_XPending(m_display))
            {
                XEvent event;
                m_XNextEvent(m_display, &event);
                HandleXEvent(event);
            }

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

            app.OnTick();
        }

        // Terminating event
        {
            TerminatingEvent ev;
            app.OnEvent(ev);
        }

        // Shutdown
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
        ViewImpl* view = HE_NEW(m_allocator, ViewImpl, this, desc);
        return view;
    }

    void DeviceImpl::DestroyView(View* view)
    {
        HE_DELETE(m_allocator, static_cast<ViewImpl*>(view));
    }

    View* DeviceImpl::GetFocusedView() const
    {
        Window focused = None;
        int revertTo = RevertToNone;
        m_XGetInputFocus(m_display, &focused, &revertTo);

        return GetViewFromWindow(focused);
    }

    View* DeviceImpl::GetHoveredView() const
    {
        Window rootWin = None;
        Window childWin = None;
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

        Window rootWin = None;
        Window childWin = None;
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
        m_XWarpPointer(m_display, None, win, 0, 0, 0, 0, dstX, dstY);
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

        const Vec2f pos = ToVec2<float>(view->GetSize()) / 2.0f;
        SetCursorPos(view, pos);
    }

    ViewImpl* DeviceImpl::GetViewFromWindow(Window win) const
    {
        if (win == None)
            return nullptr;

        ViewImpl* view = nullptr;
        if (m_XFindContext(m_display, win, m_context, reinterpret_cast<XPointer*>(&view)) == 0)
            return view;

        return nullptr;
    }

    Cursor DeviceImpl::GetActiveCursor() const
    {
        if (!m_cursorVisible || m_viewClipped || m_cursor <= MouseCursor_None || m_cursor >= MouseCursor::_Count)
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

                    MouseMoveEvent ev(GetFocusedView(), pos, false, true);

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
                Vec2f pos{ static_cast<float>(event.xcrossing.x), static_cast<float>(event.xcrossing.y) };
                MouseMoveEvent ev(view, pos, true, false);
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
                MouseButton button = MouseButton_None;
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

                if (button != MouseButton_None)
                {
                    MouseDownEvent ev(view, button);
                    view->TrackCapture(ev);
                    m_app->OnEvent(ev);
                }

                break;
            }
            case ButtonRelease:
            {
                MouseButton button = MouseButton_None;
                switch (event.xbutton.button)
                {
                    case Button1: button = MouseButton::Left; break;
                    case Button2: button = MouseButton::Middle; break;
                    case Button3: button = MouseButton::Right; break;
                }

                if (button != MouseButton_None)
                {
                    MouseUpEvent ev(view, button);
                    view->TrackCapture(ev);
                    m_app->OnEvent(ev);
                }
                break;
            }
            case MotionNotify:
            {
                Vec2f pos = { static_cast<float>(event.xmotion.x), static_cast<float>(event.xmotion.y) };
                MouseMoveEvent ev(view, pos, true, false);

                if (m_cursorRelativeMode)
                    CenterCursor();

                m_app->OnEvent(ev);
                break;
            }
            case KeyPress:
            {
                KeySym keysym = m_XLookupKeysym(&event.xkey, 0);
                Key key = TranslateKey(keysym);

                if (key != Key_None)
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

                if (key != Key_None)
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

    void DeviceImpl::OpenGamepads()
    {
        static_assert(MaxGamepads < 10, "Only support single-digit gamepad counts");

        char jspath[] = "/dev/input/jsX";
        char* num = jspath + HE_LENGTH_OF(jspath) - 2;

        for (uint32_t i = 0; i < MaxGamepads; ++i)
        {
            *num = '0' + i;
            m_gamepadFds[i] = open(jspath, O_RDONLY | O_NONBLOCK);

            if (m_gamepadFds[i] != -1)
            {
                GamepadConnectedEvent ev(i);
                m_app->OnEvent(ev);
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

                        GamepadAxis axis = GamepadAxis_None;
                        switch (js.number)
                        {
                            case 0: axis = GamepadAxis::LThumbX; break;
                            case 1: axis = GamepadAxis::LThumbY; break;
                            case 2: axis = GamepadAxis::RThumbX; break;
                            case 3: axis = GamepadAxis::RThumbY; break;
                            case 4: axis = GamepadAxis::RThumbX; break;
                            case 5: axis = GamepadAxis::RThumbY; break;
                        }

                        if (axis != GamepadAxis_None)
                        {
                            GamepadAxisEvent ev(i, axis, value);
                            m_app->OnEvent(ev);
                        }
                        break;
                    }
                    case JS_EVENT_BUTTON:
                    {
                        GamepadButton button = GamepadButton_None;
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

                        if (button != GamepadButton_None)
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
        return HE_NEW(allocator, Linux::DeviceImpl, allocator);
    }
}

#endif
