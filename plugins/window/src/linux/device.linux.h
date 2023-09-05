// Copyright Chad Engler

#pragma once

#include "gamepad.linux.h"

#include "he/math/types.h"
#include "he/window/device.h"
#include "he/window/event.h"
#include "he/window/pointer.h"

#include <atomic>

#if defined(HE_PLATFORM_LINUX)

#include "x11_all.linux.h"

namespace he::window::linux
{
    class ViewImpl;

    using Pfn_XChangeProperty = int(*)(Display*, Window, Atom, Atom, int, int, const unsigned char*, int);
    using Pfn_XCloseDisplay = int(*)(Display*);
    using Pfn_XCloseIM = Status(*)(XIM);
    using Pfn_XConvertSelection = int(*)(Display*, Atom, Atom, Atom, Window, Time);
    using Pfn_XCreateFontCursor = Cursor(*)(Display*, unsigned int);
    using Pfn_XCreateBitmapFromData = Pixmap(*)(Display*, Drawable, const char*, unsigned int, unsigned int);
    using Pfn_XCreateIC = XIC(*)(XIM, ...);
    using Pfn_XCreatePixmapCursor = Cursor(*)(Display*, Pixmap, Pixmap, XColor*, XColor*, unsigned int, unsigned int);
    using Pfn_XCreateWindow = Window(*)(Display*, Window, int, int, unsigned int, unsigned int, unsigned int, int, unsigned int, Visual*, unsigned long, XSetWindowAttributes*);
    using Pfn_XDefineCursor = int(*)(Display*, Window, Cursor);
    using Pfn_XDeleteContext = int(*)(Display*, XID, XContext);
    using Pfn_XDestroyIC = void(*)(XIC);
    using Pfn_XDestroyWindow = int(*)(Display*, Window);
    using Pfn_XFilterEvent = X11_Bool(*)(XEvent*, Window);
    using Pfn_XFindContext = int(*)(Display*, XID, XContext, XPointer*);
    using Pfn_XFlush = int(*)(Display*);
    using Pfn_XFree = int(*)(void*);
    using Pfn_XFreeCursor = int(*)(Display*, Cursor);
    using Pfn_XFreeEventData = void(*)(Display*, XGenericEventCookie*);
    using Pfn_XFreePixmap = int(*)(Display*, Pixmap);
    using Pfn_XGetAtomName = char*(*)(Display*, Atom);
    using Pfn_XGetEventData = X11_Bool(*)(Display*, XGenericEventCookie*);
    using Pfn_XGetInputFocus = int(*)(Display*, Window*, int*);
    using Pfn_XGetWMNormalHints = Status(*)(Display*, Window, XSizeHints*, long*);
    using Pfn_XGetWindowAttributes = Status(*)(Display*, Window, XWindowAttributes*);
    using Pfn_XGetWindowProperty = int(*)(Display*, Window, Atom, long, long, X11_Bool, Atom, Atom*, int*, unsigned long*, unsigned long*, unsigned char**);
    using Pfn_XGrabPointer = int(*)(Display*, Window, X11_Bool, unsigned int, int, int, Window, Cursor, Time);
    using Pfn_XIconifyWindow = Status(*)(Display*, Window, int);
    using Pfn_XInitThreads = Status(*)(void);
    using Pfn_XInternAtom = Atom(*)(Display*, const char*, X11_Bool);
    using Pfn_XLookupKeysym = KeySym(*)(XKeyEvent *key_event, int index);
    using Pfn_XMapRaised = int(*)(Display*, Window);
    using Pfn_XMapWindow = int(*)(Display*, Window);
    using Pfn_XMoveResizeWindow = int(*)(Display*, Window, int, int, unsigned int, unsigned int);
    using Pfn_XMoveWindow = int(*)(Display*, Window, int, int);
    using Pfn_XNextEvent = int(*)(Display*, XEvent*);
    using Pfn_XOpenDisplay = Display*(*)(const char*);
    using Pfn_XOpenIM = XIM(*)(Display*, XrmDatabase*, char*, char*);
    using Pfn_XPeekEvent = int(*)(Display*, XEvent*);
    using Pfn_XPending = int(*)(Display*);
    using Pfn_XQueryExtension = X11_Bool(*)(Display*, const char*, int*, int*, int*);
    using Pfn_XQueryPointer = X11_Bool(*)(Display*, Window, Window*, Window*, int*, int*, int*, int*, unsigned int*);
    using Pfn_XRaiseWindow = int(*)(Display*, Window);
    using Pfn_XRefreshKeyboardMapping = int(*)(XMappingEvent*);
    using Pfn_XResizeWindow = int(*)(Display*, Window, unsigned int, unsigned int);
    using Pfn_XSaveContext = int(*)(Display*, XID, XContext, const char*);
    using Pfn_XSendEvent = Status(*)(Display*, Window, X11_Bool, long, XEvent*);
    using Pfn_XSetInputFocus = int(*)(Display*, Window, int, Time);
    using Pfn_XSetWMProtocols = Status(*)(Display*, Window, Atom*, int);
    using Pfn_XStoreName = int(*)(Display*, Window, const char*);
    using Pfn_XTranslateCoordinates = X11_Bool(*)(Display*, Window, Window, int, int, int*, int*, Window*);
    using Pfn_XUngrabPointer = int(*)(Display*, Time);
    using Pfn_XUnmapWindow = int(*)(Display*, Window);
    using Pfn_XUnsetICFocus = void(*)(XIC);
    using Pfn_XWarpPointer = int(*)(Display*, Window, Window, int, int, unsigned int, unsigned int, int, int);
    using Pfn_XkbSetDetectableAutoRepeat = X11_Bool(*)(Display*, X11_Bool, X11_Bool*);
    using Pfn_XrmUniqueQuark = XrmQuark(*)();
    using Pfn_Xutf8LookupString = int(*)(XIC, XKeyPressedEvent*, char*, int, KeySym*, Status*);
    using Pfn_Xutf8SetWMProperties = void(*)(Display*, Window, const char*, const char*, char**, int, XSizeHints*, XWMHints*, XClassHint*);

    using Pfn_XIQueryVersion = Status(*)(Display*, int*, int*);
    using Pfn_XISelectEvents = int(*)(Display*, Window, XIEventMask*, int);
    using Pfn_XIGrabTouchBegin = int(*)(Display*, int, Window, int, XIEventMask*, int, XIGrabModifiers*);
    using Pfn_XIUngrabTouchBegin = Status(*)(Display*, int, Window, int, XIGrabModifiers*);
    using Pfn_XIQueryDevice = XIDeviceInfo*(*)(Display*, int, int*);
    using Pfn_XIFreeDeviceInfo = void(*)(XIDeviceInfo*);

    constexpr int SupportedX11DndVersion = 5;

    class DeviceImpl final : public Device
    {
    public:
        DeviceImpl(Allocator& allocator) noexcept;
        ~DeviceImpl() noexcept;

        bool Initialize() override;

        int Run(Application& app) override;
        void Quit(int rc) override;

        const DeviceInfo& GetDeviceInfo() const override;

        View* CreateView(const ViewDesc& desc) override;
        void DestroyView(View* view) override;

        View* FocusedView() const override;
        View* HoveredView() const override;

        Vec2f GetCursorPos(View* view) const override;
        void SetCursorPos(View* view, const Vec2f& pos) override;

        void SetCursor(PointerCursor cursor) override;

        void EnableRelativeCursor(View* view) override;
        void DisableRelativeCursor() override;

        uint32_t MonitorCount() const override;
        uint32_t GetMonitors(Monitor* monitors, uint32_t maxCount) const override;

        Gamepad& GetGamepad(uint32_t index) override;

    public:
        void ShowCursor(bool show);
        void CenterCursor();

        ViewImpl* GetViewFromWindow(Window win) const;
        Cursor GetActiveCursor() const;
        void HandleXEvent(XEvent& event);
        void HandleGenericXEvent(XEvent& event);
        void HandleNonViewXEvent(XEvent& event);
        void HandleViewXEvent(ViewImpl* view, XEvent& event);

        uint64_t ReadWindowProperty(Window window, Atom property, Atom type, uint8_t** value);

        struct InputDeviceInfo
        {
            int deviceId{ 0 };
            bool relative[2]{};

            uint32_t prevTime{ 0 };
            Vec2f prevPos{};
        };

        InputDeviceInfo& FindInputDeviceInfo(int deviceId);

    public:
        Application* m_app{ nullptr };
        Display* m_display{ nullptr };
        Window m_root{ X11_None };
        XContext m_context{ X11_None };
        XIM m_im{ nullptr };
        int32_t m_xiMajorOpcode{ 0 };
        PointerCursor m_cursor{ PointerCursor::Arrow };
        ViewImpl* m_cursorRelativeView{ nullptr };
        Cursor m_hiddenCursor{ X11_None };
        Pixmap m_hiddenCursorBitmap{ X11_None };
        std::atomic<int32_t> m_returnCode{ 0 };
        std::atomic<bool> m_running{ true };
        bool m_cursorVisible{ true };
        bool m_hasDetectableAutoRepeat{ false };
        bool m_viewClipped{ false };
        Vec2f m_cursorRestorePosition{ 0, 0 };
        DeviceInfo m_deviceInfo{};

        HashMap<int, InputDeviceInfo> m_inputDeviceCache{};

        int m_dndVersion{ 0 };
        Window m_dndSource{ X11_None };
        Atom m_dndFormat{ X11_None };

        Cursor m_cursors[static_cast<int32_t>(PointerCursor::_Count)];
        Atom m_atomNetActiveWindow{ X11_None };
        Atom m_atomNetWMPing{ X11_None };
        Atom m_atomNetWMState{ X11_None };
        Atom m_atomNetWMStateAbove{ X11_None };
        Atom m_atomNetWMStateMinimized{ X11_None };
        Atom m_atomNetWMStateMaximizedHorz{ X11_None };
        Atom m_atomNetWMStateMaximizedVert{ X11_None };
        Atom m_atomNetWMStateFullscreen{ X11_None };
        Atom m_atomNetWMWindowOpacity{ X11_None };
        Atom m_atomNetWMWindowType{ X11_None };
        Atom m_atomNetWMWindowTypeNormal{ X11_None };
        Atom m_atomMotifWMHints{ X11_None };
        Atom m_atomWMDeleteWindow{ X11_None };
        Atom m_atomWMProtocols{ X11_None };
        Atom m_atomWMState{ X11_None };
        Atom m_atomXdndAware{ X11_None };
        Atom m_atomXdndEnter{ X11_None };
        Atom m_atomXdndLeave{ X11_None };
        Atom m_atomXdndPosition{ X11_None };
        Atom m_atomXdndStatus{ X11_None };
        Atom m_atomXdndActionCopy{ X11_None };
        Atom m_atomXdndActionMove{ X11_None };
        Atom m_atomXdndActionLink{ X11_None };
        Atom m_atomXdndDrop{ X11_None };
        Atom m_atomXdndFinished{ X11_None };
        Atom m_atomXdndSelection{ X11_None };
        Atom m_atomXdndTypeList{ X11_None };
        Atom m_atomTextUriList{ X11_None };

        void* m_xlib{ nullptr };
        Pfn_XChangeProperty m_XChangeProperty{ nullptr };
        Pfn_XCloseDisplay m_XCloseDisplay{ nullptr };
        Pfn_XCloseIM m_XCloseIM{ nullptr };
        Pfn_XConvertSelection m_XConvertSelection{ nullptr };
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
        Pfn_XGetAtomName m_XGetAtomName{ nullptr };
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
        Pfn_XIGrabTouchBegin m_XIGrabTouchBegin{ nullptr };
        Pfn_XIUngrabTouchBegin m_XIUngrabTouchBegin{ nullptr };
        Pfn_XIQueryDevice m_XIQueryDevice{ nullptr };
        Pfn_XIFreeDeviceInfo m_XIFreeDeviceInfo{ nullptr };

        GamepadImpl m_gamepads[MaxGamepads];
        bool m_refreshGamepadConnectivity{ true };
    };
}

#endif
