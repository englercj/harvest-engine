// Copyright Chad Engler

#include "view.linux.h"

#include "device.linux.h"

#include "he/core/assert.h"

#if defined(HE_PLATFORM_LINUX)

#include "x11_all.linux.h"

namespace he::window::linux
{
    ViewImpl::ViewImpl(DeviceImpl* device, const ViewDesc& desc) noexcept
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
        attribs.event_mask = StructureNotifyMask
            | KeyPressMask | KeyReleaseMask
            | ButtonPressMask | ButtonReleaseMask
            | PointerMotionMask
            | FocusChangeMask
            | EnterWindowMask
            | KeymapStateMask;

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

        HE_ASSERT(m_window != X11_None, HE_MSG("Failed to create X window."));

        m_device->m_XSaveContext(display, m_window, m_device->m_context, reinterpret_cast<XPointer>(this));

        // Announce that our view supports Drag-and-drop
        if (HasFlag(m_flags, ViewFlag::AcceptFiles))
        {
            const Atom xdndVersion = SupportedX11DndVersion;
            m_device->m_XChangeProperty(
                display,
                m_window,
                m_device->m_atomXdndAware,
                XA_ATOM,
                32,
                PropModeReplace,
                reinterpret_cast<uint8_t*>(&xdndVersion),
                1);
        }

        // Set motif hints for view types
        if (m_device->m_atomMotifWMHints != X11_None)
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
            if (HasFlag(m_flags, ViewFlag::Borderless))
                hints.decorations = 0;
            else
                hints.decorations = (1L << 0); // MWM_DECOR_ALL

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
        if (m_device->m_atomNetWMState != X11_None)
        {
            Atom states[3];
            int count = 0;

            if (HasFlag(m_flags, ViewFlag::StartMaximized)
                && m_device->m_atomNetWMStateMaximizedHorz != X11_None
                && m_device->m_atomNetWMStateMaximizedVert != X11_None)
            {
                states[count++] = m_device->m_atomNetWMStateMaximizedHorz;
                states[count++] = m_device->m_atomNetWMStateMaximizedVert;
                m_maximized = true;
            }

            if (HasFlag(m_flags, ViewFlag::TopMost) && m_device->m_atomNetWMStateAbove != X11_None)
            {
                states[count++] = m_device->m_atomNetWMStateAbove;
            }

            if (count)
            {
                m_device->m_XChangeProperty(
                    display,
                    m_window,
                    m_device->m_atomNetWMState,
                    XA_ATOM,
                    32,
                    PropModeReplace,
                    reinterpret_cast<uint8_t*>(states),
                    count);
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
        if (m_device->m_atomNetWMWindowType != X11_None && m_device->m_atomNetWMWindowTypeNormal != X11_None)
        {
            Atom type = m_device->m_atomNetWMWindowTypeNormal;
            m_device->m_XChangeProperty(display, m_window, m_device->m_atomNetWMWindowType, XA_ATOM, 32, PropModeReplace, reinterpret_cast<uint8_t*>(&type), 1);
        }

        m_ic = m_device->m_XCreateIC(m_device->m_im, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, m_device->m_root, nullptr);
        HE_ASSERT(m_ic != nullptr, HE_MSG("XCreateIC failed."));
    }

    ViewImpl::~ViewImpl() noexcept
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
        Window focused = X11_None;
        int revertTo = RevertToNone;
        m_device->m_XGetInputFocus(m_device->m_display, &focused, &revertTo);

        return focused == m_window;
    }

    bool ViewImpl::IsChildFocused() const
    {
        Window focused = X11_None;
        int revertTo = RevertToNone;
        m_device->m_XGetInputFocus(m_device->m_display, &focused, &revertTo);
        // TODO! QueryTree()?
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
        if (m_device->m_atomNetWMState == X11_None
            || m_device->m_atomNetWMStateMaximizedHorz == X11_None
            || m_device->m_atomNetWMStateMaximizedVert == X11_None)
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

    void ViewImpl::SetAcceptInput(bool value)
    {
        // TODO
        HE_UNUSED(value);
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
        Window childWin = X11_None;
        X11_Bool result = m_device->m_XTranslateCoordinates(m_device->m_display, m_window, m_device->m_root, srcX, srcY, &dstX, &dstY, &childWin);

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
        Window childWin = X11_None;
        X11_Bool result = m_device->m_XTranslateCoordinates(m_device->m_display, m_device->m_root, m_window, srcX, srcY, &dstX, &dstY, &childWin);

        if (result == False)
            return pos;

        return { static_cast<float>(dstX), static_cast<float>(dstY) };
    }

    void ViewImpl::TrackCapture(const PointerEvent& ev)
    {
        HE_ASSERT(ev.kind == EventKind::PointerDown || ev.kind == EventKind::PointerUp);

        if (ev.kind == EventKind::PointerDown)
        {
            if (ev.pointerKind == PointerKind::Mouse)
            {
                if (m_mouseCaptureCount++ == 0)
                    CaptureMouse();
            }
            else if (ev.pointerKind == PointerKind::Touch)
            {
                if (m_touchCaptureCount++ == 0)
                    CaptureTouch();
            }
        }
        else
        {
            if (ev.pointerKind == PointerKind::Mouse)
            {
                if (--m_mouseCaptureCount == 0)
                    ReleaseMouse();
            }
            else if (ev.pointerKind == PointerKind::Touch)
            {
                if (--m_touchCaptureCount == 0)
                    ReleaseTouch();
            }
        }
    }

    void ViewImpl::CaptureMouse()
    {
        if (!m_device->m_cursorRelativeMode)
        {
            uint32_t mask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
            Cursor cursor = m_device->GetActiveCursor();
            m_device->m_XGrabPointer(m_device->m_display, m_window, True, mask, GrabModeAsync, GrabModeAsync, X11_None, cursor, CurrentTime);
        }
    }

    void ViewImpl::ReleaseMouse()
    {
        if (!m_device->m_cursorRelativeMode)
        {
            m_device->m_XUngrabPointer(m_device->m_display, CurrentTime);
        }
    }

    void ViewImpl::CaptureTouch()
    {
        if (!m_device->m_deviceInfo.hasTouch)
            return;

        uint8_t mask[XIMaskLen(XI_LASTEVENT)]{};

        XIGrabModifiers mods;
        mods.modifiers = XIAnyModifier;
        mods.status = 0;

        XIEventMask em;
        em.deviceid = XIAllDevices;
        em.mask_len = sizeof(mask);
        em.mask = mask;

        XISetMask(em.mask, XI_TouchBegin);
        XISetMask(em.mask, XI_TouchUpdate);
        XISetMask(em.mask, XI_TouchEnd);
        XISetMask(em.mask, XI_Motion);

        m_device->m_XIGrabTouchBegin(m_device->m_display, XIAllDevices, m_window, True, &em, 1, &mods);
    }

    void ViewImpl::ReleaseTouch()
    {
        if (!m_device->m_deviceInfo.hasTouch)
            return;

        XIGrabModifiers mods;
        mods.modifiers = XIAnyModifier;
        mods.status = 0;

        m_device->m_XIUngrabTouchBegin(m_device->m_display, XIAllDevices, m_window, 1, &mods);
    }
}

#endif
