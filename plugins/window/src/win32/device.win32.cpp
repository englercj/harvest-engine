// Copyright Chad Engler

#include "device.win32.h"

#include "common.win32.h"
#include "view.win32.h"

#include "he/core/assert.h"
#include "he/core/enum_ops.h"
#include "he/core/string.h"
#include "he/core/vector.h"
#include "he/core/wstr.h"
#include "he/math/vec2.h"
#include "he/window/application.h"
#include "he/window/event.h"

#if defined(HE_PLATFORM_API_WIN32)

#include <ShellScalingApi.h>
#include <Windows.h>
#include <Xinput.h>

// From Windowsx.h
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

#if !defined(DBT_DEVNODES_CHANGED)
    #define DBT_DEVNODES_CHANGED 0x0007
#endif

namespace he::window::win32
{
    static Key TranslateKey(WPARAM wparam, LPARAM lparam)
    {
        int32_t vkey = wparam & 0xff;

        if (vkey == VK_RETURN && (HIWORD(lparam) & KF_EXTENDED))
            return Key::NumPad_Enter;

        switch (vkey)
        {
            case VK_BACK: return Key::Backspace;
            case VK_RETURN: return Key::Enter;
            case VK_ESCAPE: return Key::Escape;
            case VK_SPACE: return Key::Space;
            case VK_TAB: return Key::Tab;
            case VK_PAUSE: return Key::Pause;
            case VK_SNAPSHOT: return Key::PrintScreen;
            case VK_DECIMAL: return Key::NumPad_Decimal;
            case VK_MULTIPLY: return Key::NumPad_Multiply;
            case VK_ADD: return Key::NumPad_Add;
            case VK_SUBTRACT: return Key::NumPad_Subtract;
            case VK_DIVIDE: return Key::NumPad_Divide;
            case VK_NUMPAD0: return Key::NumPad_0;
            case VK_NUMPAD1: return Key::NumPad_1;
            case VK_NUMPAD2: return Key::NumPad_2;
            case VK_NUMPAD3: return Key::NumPad_3;
            case VK_NUMPAD4: return Key::NumPad_4;
            case VK_NUMPAD5: return Key::NumPad_5;
            case VK_NUMPAD6: return Key::NumPad_6;
            case VK_NUMPAD7: return Key::NumPad_7;
            case VK_NUMPAD8: return Key::NumPad_8;
            case VK_NUMPAD9: return Key::NumPad_9;
            case VK_F1: return Key::F1;
            case VK_F2: return Key::F2;
            case VK_F3: return Key::F3;
            case VK_F4: return Key::F4;
            case VK_F5: return Key::F5;
            case VK_F6: return Key::F6;
            case VK_F7: return Key::F7;
            case VK_F8: return Key::F8;
            case VK_F9: return Key::F9;
            case VK_F10: return Key::F10;
            case VK_F11: return Key::F11;
            case VK_F12: return Key::F12;
            case VK_HOME: return Key::Home;
            case VK_LEFT: return Key::Left;
            case VK_UP: return Key::Up;
            case VK_RIGHT: return Key::Right;
            case VK_DOWN: return Key::Down;
            case VK_PRIOR: return Key::PageUp;
            case VK_NEXT: return Key::PageDown;
            case VK_INSERT: return Key::Insert;
            case VK_DELETE: return Key::Delete;
            case VK_END: return Key::End;
            case VK_MENU: return Key::Alt;
            case VK_CONTROL: return Key::Control;
            case VK_SHIFT: return Key::Shift;
            case VK_LWIN:
            case VK_RWIN: return Key::Super;
            case VK_SCROLL: return Key::ScrollLock;
            case VK_NUMLOCK: return Key::NumLock;
            case VK_CAPITAL: return Key::CapsLock;
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
            case 'A': return Key::A;
            case 'B': return Key::B;
            case 'C': return Key::C;
            case 'D': return Key::D;
            case 'E': return Key::E;
            case 'F': return Key::F;
            case 'G': return Key::G;
            case 'H': return Key::H;
            case 'I': return Key::I;
            case 'J': return Key::J;
            case 'K': return Key::K;
            case 'L': return Key::L;
            case 'M': return Key::M;
            case 'N': return Key::N;
            case 'O': return Key::O;
            case 'P': return Key::P;
            case 'Q': return Key::Q;
            case 'R': return Key::R;
            case 'S': return Key::S;
            case 'T': return Key::T;
            case 'U': return Key::U;
            case 'V': return Key::V;
            case 'W': return Key::W;
            case 'X': return Key::X;
            case 'Y': return Key::Y;
            case 'Z': return Key::Z;
            case VK_OEM_1: return Key::Semicolon;
            case VK_OEM_PLUS: return Key::Equals;
            case VK_OEM_COMMA: return Key::Comma;
            case VK_OEM_MINUS: return Key::Minus;
            case VK_OEM_PERIOD: return Key::Period;
            case VK_OEM_2: return Key::Slash;
            case VK_OEM_3: return Key::Grave;
            case VK_OEM_4: return Key::LeftBracket;
            case VK_OEM_5: return Key::Backslash;
            case VK_OEM_6: return Key::RightBracket;
            case VK_OEM_7: return Key::Apostrophe;
        }

        return Key::None;
    }

    static PointerKind GetMouseMessagePointerKind()
    {
        constexpr LPARAM PenOrTouchSignature = 0xFF515700;
        constexpr LPARAM SignatureMask = 0xFFFFFF00;
        constexpr LPARAM TouchFlag = 0x80;

        // See: https://learn.microsoft.com/en-us/windows/win32/tablet/system-events-and-mouse-messages#distinguishing-pen-input-from-mouse-and-touch
        const LPARAM extra = ::GetMessageExtraInfo();
        const bool isPenOrTouch = (extra & SignatureMask) == PenOrTouchSignature;

        if (isPenOrTouch)
        {
            if (HasFlag(extra, TouchFlag))
                return PointerKind::Touch;
            else
                return PointerKind::Pen;
        }

        return PointerKind::Mouse;
    }

#if HE_ENABLE_ASSERTIONS
    static PointerId GetMouseMessagePointerId()
    {
        // The lower 7 bits returned from GetMessageExtraInfo are used to represent the
        // cursor ID. For mouse this is zero, or a variable value for the pen ID.
        constexpr LPARAM CursorIdMask = 0x7f;

        const LPARAM extra = ::GetMessageExtraInfo();
        const PointerId pointerId = (extra & CursorIdMask) + 1; // +1 converts it to Harvest ID space

        // Ensure that our assumptions about how these values work are actually valid
        HE_ASSERT(pointerId != 0, HE_KV(pointer_id, pointerId));
        HE_ASSERT(GetMouseMessagePointerKind() != PointerKind::Mouse || pointerId == PointerId_Mouse, HE_KV(pointer_id, pointerId));
        HE_ASSERT(GetMouseMessagePointerKind() == PointerKind::Mouse || pointerId != PointerId_Mouse, HE_KV(pointer_id, pointerId));

        return pointerId;
    }
#endif

    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_NCCREATE)
        {
            // store window instance pointer in window user data
            void* userdata = reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams;
            ::SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(userdata));
        }

        ViewImpl* view = reinterpret_cast<ViewImpl*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));

        if (!view)
            return ::DefWindowProc(hWnd, message, wParam, lParam);

        DeviceImpl* device = view->m_device;
        Application* app = device->m_app;

        switch (message)
        {
            case WM_CLOSE:
            {
                ViewRequestCloseEvent ev(view);
                app->OnEvent(ev);
                return 0;
            }
            case WM_MOVE:
            {
                view->m_pos = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                ViewMovedEvent ev(view);
                ev.pos = view->m_pos;
                app->OnEvent(ev);
                return 0;
            }
            case WM_SIZE:
            {
                view->m_size = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                ViewResizedEvent ev(view);
                ev.size = view->m_size;
                app->OnEvent(ev);
                return 0;
            }
            case WM_POINTERACTIVATE:
            {
                if (!HasFlag(view->m_flags, ViewFlag::FocusOnClick))
                    return PA_NOACTIVATE;
                break;
            }
            case WM_MOUSEACTIVATE:
            {
                if (!HasFlag(view->m_flags, ViewFlag::FocusOnClick))
                    return MA_NOACTIVATE;
                break;
            }
            case WM_NCCALCSIZE:
            {
                if (wParam == TRUE && HasFlag(view->m_flags, ViewFlag::Borderless))
                {
                    // Borderless windows technically have a thick border that is simply drawn over.
                    // When maximized, Windows will bleed our border over the edges of the monitor
                    // because it thinks the client size of the window is the entire monitor size.
                    // We adjust the size here to compensate.
                    if (view->IsMaximized())
                    {
                        // Gather the size of the border that Windows will bleed over
                        WINDOWINFO winInfo{};
                        winInfo.cbSize = sizeof(winInfo);
                        ::GetWindowInfo(hWnd, &winInfo);

                        LPNCCALCSIZE_PARAMS params = reinterpret_cast<LPNCCALCSIZE_PARAMS>(lParam);

                        // The first rectangle contains the client rectangle of the resized window.
                        // Decrease window size on all sides by the border size.
                        params->rgrc[0].left += winInfo.cxWindowBorders;
                        params->rgrc[0].top += winInfo.cxWindowBorders;
                        params->rgrc[0].right -= winInfo.cxWindowBorders;
                        params->rgrc[0].bottom -= winInfo.cxWindowBorders;

                        // The second rectangle contains the destination rectangle for the content
                        // currently displayed in the window's client rect. Windows will blit the
                        // previous client content into this new location to simulate the move of
                        // the window until the window can repaint itself. This should also be
                        // adjusted to our new window size.
                        params->rgrc[1].left = params->rgrc[0].left;
                        params->rgrc[1].top = params->rgrc[0].top;
                        params->rgrc[1].right = params->rgrc[0].right;
                        params->rgrc[1].bottom = params->rgrc[0].bottom;

                        // A third rectangle is passed in that contains the source rectangle
                        // (client area from window pre-maximize). It's value is not changed.

                        // The new window position. Pull in the window on all sides by the width
                        // of the window border so that the window fits entirely on screen.
                        // params->lppos->x += winInfo.cxWindowBorders;
                        // params->lppos->y += winInfo.cxWindowBorders;
                        // params->lppos->cx -= 2 * winInfo.cxWindowBorders;
                        // params->lppos->cy -= 2 * winInfo.cxWindowBorders;

                        // Informs Windows to use the values as we altered them.
                        return WVR_VALIDRECTS;
                    }
                    return 0;
                }
                break;
            }
            case WM_NCHITTEST:
            {
                // If we've disabled input, pass through the hit test
                if (!HasFlag(view->m_flags, ViewFlag::AcceptInput))
                    return HTTRANSPARENT;

                // Perform application defined hit testing if decoration is disabled.
                if (HasFlag(view->m_flags, ViewFlag::Borderless))
                {
                    POINT hitPoint = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                    if (::ScreenToClient(hWnd, &hitPoint))
                    {
                        ViewHitArea hit = app->OnHitTest(view, { hitPoint.x, hitPoint.y });

                        const bool allowResize = HasFlag(view->m_flags, ViewFlag::AllowResize);

                        switch (hit)
                        {
                            case ViewHitArea::ButtonClose: return HTCLOSE;
                            case ViewHitArea::ButtonMinimize: return HTMINBUTTON;
                            case ViewHitArea::ButtonMaximizeAndRestore: return HTMAXBUTTON;
                            case ViewHitArea::Draggable: return HTCAPTION;
                            case ViewHitArea::Normal: return HTCLIENT;
                            case ViewHitArea::NotInView: return HTNOWHERE;
                            case ViewHitArea::ResizeTopLeft: return allowResize ? HTTOPLEFT : HTCLIENT;
                            case ViewHitArea::ResizeTop: return allowResize ? HTTOP : HTCLIENT;
                            case ViewHitArea::ResizeTopRight: return allowResize ? HTTOPRIGHT : HTCLIENT;
                            case ViewHitArea::ResizeRight: return allowResize ? HTRIGHT : HTCLIENT;
                            case ViewHitArea::ResizeBottomRight: return allowResize ? HTBOTTOMRIGHT : HTCLIENT;
                            case ViewHitArea::ResizeBottom: return allowResize ? HTBOTTOM : HTCLIENT;
                            case ViewHitArea::ResizeBottomLeft: return allowResize ? HTBOTTOMLEFT : HTCLIENT;
                            case ViewHitArea::ResizeLeft: return allowResize ? HTLEFT : HTCLIENT;
                            case ViewHitArea::SystemMenu: return HTSYSMENU;
                        }
                        HE_VERIFY(false, HE_MSG("Unknown hit area."));
                    }
                }
                break;
            }
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            {
                Key key = TranslateKey(wParam, lParam);
                if (key != Key::None)
                {
                    KeyDownEvent ev(view);
                    ev.key = key;
                    app->OnEvent(ev);
                }
                return 0;
            }
            case WM_KEYUP:
            case WM_SYSKEYUP:
            {
                Key key = TranslateKey(wParam, lParam);
                if (key != Key::None)
                {
                    KeyUpEvent ev(view);
                    ev.key = key;
                    app->OnEvent(ev);
                }
                return 0;
            }
            case WM_CHAR:
            {
                if (wParam > 0 && wParam < 0x10000)
                {
                    TextEvent ev(view);
                    ev.ch = LOWORD(wParam);
                    app->OnEvent(ev);
                }
                return 0;
            }
            case WM_ACTIVATE:
            {
                if (view->m_captureCount)
                {
                    ::ReleaseCapture();
                    view->m_captureCount = 0;
                }

                const bool active = LOWORD(wParam) != WA_INACTIVE;
                ViewActivatedEvent ev(view);
                ev.active = active;

                if (app)
                    app->OnEvent(ev);

                return 0;
            }
            case WM_MOVING:
            case WM_SIZING:
            case WM_TIMER:
            {
                app->OnTick();
                return 0;
            }
            case WM_DPICHANGED:
            {
                view->m_dpi = HIWORD(wParam);

                RECT* const prcNewWindow = (RECT*)lParam;
                ::SetWindowPos(
                    hWnd,
                    nullptr,
                    prcNewWindow->left,
                    prcNewWindow->top,
                    prcNewWindow->right - prcNewWindow->left,
                    prcNewWindow->bottom - prcNewWindow->top,
                    SWP_NOZORDER | SWP_NOACTIVATE);

                ViewDpiScaleChangedEvent ev(view);
                ev.scale = view->GetDpiScale();
                app->OnEvent(ev);
                break;
            }
            case WM_SETCURSOR:
            {
                device->SyncCursor();
                break;
            }
            case WM_DEVICECHANGE:
            {
                if (LOWORD(wParam) == DBT_DEVNODES_CHANGED)
                    device->m_refreshGamepadConnectivity = true;
                break;
            }
            case WM_DISPLAYCHANGE:
            {
                DisplayChangedEvent ev;
                app->OnEvent(ev);
                break;
            }
            case WM_INPUT:
            {
                // handling of high def mouse input
                // https://docs.microsoft.com/en-us/windows/desktop/dxtecharts/taking-advantage-of-high-dpi-mouse-movement

                if (!device->m_deviceInfo.hasHighDefMouse)
                    break;

                RAWINPUT raw;
                UINT rawSize = sizeof(raw);
                UINT size = ::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &rawSize, sizeof(RAWINPUTHEADER));

                if (size != rawSize)
                    break;

                if (raw.header.dwType != RIM_TYPEMOUSE)
                    break;

                PointerKind pointerKind = GetMouseMessagePointerKind();
                if (pointerKind != PointerKind::Mouse || (::GetMessageExtraInfo() & 0x82) == 0x82)
                    break;

                const RAWMOUSE& rawMouse = raw.data.mouse;
                const bool absolute = HasFlag(rawMouse.usFlags, MOUSE_MOVE_ABSOLUTE);

                Vec2f pos{ static_cast<float>(rawMouse.lLastX), static_cast<float>(rawMouse.lLastY) };

                // When raw mouse gives absolute coordinates they are normalized in the range [0,65535]
                // so we need to adjust them by the screen size to get what we expect.
                // https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-rawmouse
                if (absolute)
                {
                    const bool isVirtualDesktop = HasFlag(rawMouse.usFlags, MOUSE_VIRTUAL_DESKTOP);
                    const int width = ::GetSystemMetrics(isVirtualDesktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
                    const int height = ::GetSystemMetrics(isVirtualDesktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);

                    pos.x = (pos.x / 65535.0f) * width;
                    pos.y = (pos.y / 65535.0f) * height;
                }
                // Ignore relative motion of 0,0
                else if (pos.x == 0 && pos.y == 0)
                {
                    break;
                }

                HE_ASSERT(GetMouseMessagePointerId() == PointerId_Mouse);

                PointerMoveEvent ev(view);
                ev.pointerId = PointerId_Mouse;
                ev.pointerKind = PointerKind::Mouse;
                ev.isPrimary = true;
                ev.pos = pos;
                ev.absolute = absolute;

                if (device->m_cursorRelativeMode)
                    device->CenterCursor();

                app->OnEvent(ev);
                break;
            }
            case WM_POINTERDOWN:
            case WM_POINTERUP:
            case WM_POINTERWHEEL:
            case WM_POINTERHWHEEL:
            {
                if (!device->m_GetPointerType)
                    break;

                const WORD pointerId = GET_POINTERID_WPARAM(wParam);
                POINTER_INPUT_TYPE type{};
                if (!device->m_GetPointerType(pointerId, &type))
                    break;

                bool handled = false;
                switch (type)
                {
                    case PT_MOUSE: handled = device->HandlePointerFrameMouse(view, pointerId); break;
                    case PT_PEN: handled = device->HandlePointerFramePen(view, pointerId); break;
                    case PT_TOUCH: handled = device->HandlePointerFrameTouch(view, pointerId); break;
                }

                if (handled)
                {
                    if (device->m_SkipPointerFrameMessages)
                        device->m_SkipPointerFrameMessages(pointerId);
                    return 0;
                }

                break;
            }
            case WM_POINTERUPDATE:
            {
                if (!device->m_GetPointerType)
                    break;

                const WORD pointerId = GET_POINTERID_WPARAM(wParam);
                POINTER_INPUT_TYPE type{};
                if (!device->m_GetPointerType(pointerId, &type))
                    break;

                bool handled = false;
                switch (type)
                {
                    case PT_MOUSE: handled = device->HandlePointerFrameHistoryMouse(view, pointerId); break;
                    case PT_PEN: handled = device->HandlePointerFrameHistoryPen(view, pointerId); break;
                    case PT_TOUCH: handled = device->HandlePointerFrameHistoryTouch(view, pointerId); break;
                }

                if (handled)
                {
                    if (device->m_SkipPointerFrameMessages)
                        device->m_SkipPointerFrameMessages(pointerId);
                    return 0;
                }

                break;
            }
            case WM_DROPFILES:
            {
                if (!HasFlag(view->m_flags, ViewFlag::AcceptFiles))
                    break;

                HDROP drop = reinterpret_cast<HDROP>(wParam);

                const UINT count = ::DragQueryFileW(drop, 0xffffffff, NULL, 0);

                Vector<wchar_t> buf{};
                String path{};

                for (UINT i = 0; i < count; ++i)
                {
                    const UINT length = ::DragQueryFileW(drop, i, NULL, 0);
                    buf.Resize(length + 1);

                    if (::DragQueryFileW(drop, i, buf.Data(), buf.Size()))
                    {
                        WCToMBStr(path, buf.Data());

                        ViewDropFileEvent ev(view);
                        ev.path = path;
                        app->OnEvent(ev);
                    }
                }

                ViewDropFileCompleteEvent ev(view);
                app->OnEvent(ev);
                ::DragFinish(drop);
                return 0;
            }
        }

        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    struct VisitMonitorData
    {
        const DeviceImpl* device;
        Monitor* monitors;
        uint32_t index;
        uint32_t maxCount;
    };

    static BOOL CALLBACK VisitMonitorsCallback(HMONITOR hMonitor, HDC, LPRECT, LPARAM dwData)
    {
        VisitMonitorData* data = reinterpret_cast<VisitMonitorData*>(dwData);
        if (data->index >= data->maxCount)
            return FALSE;

        MONITORINFO info{};
        info.cbSize = sizeof(MONITORINFO);
        if (::GetMonitorInfoW(hMonitor, &info))
        {
            Monitor& monitor = data->monitors[data->index];
            monitor.pos = { info.rcMonitor.left, info.rcMonitor.top };
            monitor.size = { info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top };
            monitor.workPos = { info.rcWork.left, info.rcWork.top };
            monitor.workSize = { info.rcWork.right - info.rcWork.left, info.rcWork.bottom - info.rcWork.top };
            monitor.dpiScale = 1.0f;
            monitor.primary = HasFlag(info.dwFlags, MONITORINFOF_PRIMARY);

            UINT xdpi = 96;
            UINT ydpi = 96;
            if (data->device->m_GetDpiForMonitor)
            {
                data->device->m_GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &xdpi, &ydpi);
                HE_ASSERT(xdpi == ydpi);
            }
            else
            {
                const HDC dc = ::GetDC(NULL);
                if (dc)
                {
                    xdpi = ::GetDeviceCaps(dc, LOGPIXELSX);
                    ydpi = ::GetDeviceCaps(dc, LOGPIXELSY);
                    HE_ASSERT(xdpi == ydpi);
                    ::ReleaseDC(NULL, dc);
                }
            }
            monitor.dpiScale = xdpi / 96.0f;
        }

        data->index++;
        return TRUE;
    }

    DeviceImpl::DeviceImpl(Allocator& allocator) noexcept
        : Device(allocator)
        , m_gamepads{ { this, 0 }, { this, 1 }, { this, 2 }, { this, 3 } }
    {}

    DeviceImpl::~DeviceImpl() noexcept
    {
        ::UnregisterClassW(_heWindowClassName, m_hInstance);
        ::FreeLibrary(m_userLib);
        ::FreeLibrary(m_shcoreLib);
        ::FreeLibrary(m_xinputLib);
    }

    bool DeviceImpl::Initialize()
    {
        ::EnableMouseInPointer(TRUE);
        HE_ASSERT(::IsMouseInPointerEnabled());

        m_hInstance = ::GetModuleHandleW(nullptr);

        // Bind to optional Windows APIs
        m_userLib = ::LoadLibraryW(L"user32.dll");
        if (m_userLib)
        {
            m_GetDpiForWindow = reinterpret_cast<Pfn_GetDpiForWindow>(::GetProcAddress(m_userLib, "GetDpiForWindow"));
            m_SetProcessDpiAwarenessContext = reinterpret_cast<Pfn_SetProcessDpiAwarenessContext>(::GetProcAddress(m_userLib, "SetProcessDpiAwarenessContext"));
            m_AdjustWindowRectExForDpi = reinterpret_cast<Pfn_AdjustWindowRectExForDpi>(::GetProcAddress(m_userLib, "AdjustWindowRectExForDpi"));
            m_ChangeWindowMessageFilterEx = reinterpret_cast<Pfn_ChangeWindowMessageFilterEx>(::GetProcAddress(m_userLib, "ChangeWindowMessageFilterEx"));
            m_RegisterTouchWindow = reinterpret_cast<Pfn_RegisterTouchWindow>(::GetProcAddress(m_userLib, "RegisterTouchWindow"));
            m_GetTouchInputInfo = reinterpret_cast<Pfn_GetTouchInputInfo>(::GetProcAddress(m_userLib, "GetTouchInputInfo"));
            m_CloseTouchInputHandle = reinterpret_cast<Pfn_CloseTouchInputHandle>(::GetProcAddress(m_userLib, "CloseTouchInputHandle"));
            m_GetPointerType = reinterpret_cast<Pfn_GetPointerType>(::GetProcAddress(m_userLib, "GetPointerType"));
            m_GetPointerFrameInfo = reinterpret_cast<Pfn_GetPointerFrameInfo>(::GetProcAddress(m_userLib, "GetPointerFrameInfo"));
            m_GetPointerFrameInfo = reinterpret_cast<Pfn_GetPointerFrameInfo>(::GetProcAddress(m_userLib, "GetPointerFrameInfo"));
            m_GetPointerFrameInfoHistory = reinterpret_cast<Pfn_GetPointerFrameInfoHistory>(::GetProcAddress(m_userLib, "GetPointerFrameInfoHistory"));
            m_GetPointerFramePenInfo = reinterpret_cast<Pfn_GetPointerFramePenInfo>(::GetProcAddress(m_userLib, "GetPointerFramePenInfo"));
            m_GetPointerFramePenInfoHistory = reinterpret_cast<Pfn_GetPointerFramePenInfoHistory>(::GetProcAddress(m_userLib, "GetPointerFramePenInfoHistory"));
            m_GetPointerFrameTouchInfo = reinterpret_cast<Pfn_GetPointerFrameTouchInfo>(::GetProcAddress(m_userLib, "GetPointerFrameTouchInfo"));
            m_GetPointerFrameTouchInfoHistory = reinterpret_cast<Pfn_GetPointerFrameTouchInfoHistory>(::GetProcAddress(m_userLib, "GetPointerFrameTouchInfoHistory"));
            m_SkipPointerFrameMessages = reinterpret_cast<Pfn_SkipPointerFrameMessages>(::GetProcAddress(m_userLib, "SkipPointerFrameMessages"));
        }

        m_shcoreLib = ::LoadLibraryW(L"Shcore.dll");
        if (m_shcoreLib)
        {
            m_GetDpiForMonitor = reinterpret_cast<Pfn_GetDpiForMonitor>(::GetProcAddress(m_shcoreLib, "GetDpiForMonitor"));
        }

        m_xinputLib = ::LoadLibraryW(L"XInput1_4.dll");
        if (!m_xinputLib)
            m_xinputLib = ::LoadLibraryW(L"XInput1_3.dll");
        if (m_xinputLib)
        {
            m_XInputGetState = reinterpret_cast<Pfn_XInputGetState>(::GetProcAddress(m_xinputLib, "XInputGetState"));
            m_XInputSetState = reinterpret_cast<Pfn_XInputSetState>(::GetProcAddress(m_xinputLib, "XInputSetState"));
            m_XInputEnable = reinterpret_cast<Pfn_XInputEnable>(::GetProcAddress(m_xinputLib, "XInputEnable"));
        }

        // Enable high DPI support
        if (m_SetProcessDpiAwarenessContext)
            m_SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        // Enable xinput
        if (m_XInputEnable)
            m_XInputEnable(TRUE);

        // Initialize window class description
        WNDCLASSEXW wcex{};
        wcex.cbSize = sizeof(wcex);
        wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wcex.lpfnWndProc = WindowProc;
        wcex.hInstance = m_hInstance;
        wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BACKGROUND + 1);
        wcex.lpszClassName = _heWindowClassName;
        // wcex.hIcon = desc.iconResourceId ? ::LoadIconW(m_hInstance, MAKEINTRESOURCEW(desc.iconResourceId)) : nullptr;
        // wcex.hIconSm = desc.iconSmResourceId ? ::LoadIconW(m_hInstance, MAKEINTRESOURCEW(desc.iconSmResourceId)) : nullptr;
        ::RegisterClassExW(&wcex);

        // Always have support for mouse
        m_deviceInfo.hasMouse = true;

        // Enable raw mouse input
        RAWINPUTDEVICE rid;
        rid.usUsagePage = 0x01; // HID_USAGE_PAGE_GENERIC
        rid.usUsage = 0x02; // HID_USAGE_GENERIC_MOUSE
        rid.dwFlags = 0;
        rid.hwndTarget = nullptr;
        if (::RegisterRawInputDevices(&rid, 1, sizeof(rid)))
        {
            m_deviceInfo.hasHighDefMouse = true;
        }

        // Check for touch support
        const int32_t digitizer = ::GetSystemMetrics(SM_DIGITIZER);
        if (HasFlag(digitizer, NID_READY))
        {
            if (HasAnyFlags(digitizer, NID_INTEGRATED_TOUCH | NID_EXTERNAL_TOUCH))
            {
                m_deviceInfo.hasTouch = true;
                m_deviceInfo.maxTouches = ::GetSystemMetrics(SM_MAXIMUMTOUCHES);
            }

            if (HasAnyFlags(digitizer, NID_INTEGRATED_PEN | NID_EXTERNAL_PEN))
                m_deviceInfo.hasPen = true;
        }

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
            if (m_XInputGetState)
            {
                for (GamepadImpl& pad : m_gamepads)
                {
                    pad.Update(m_refreshGamepadConnectivity);
                }
                m_refreshGamepadConnectivity = false;
            }

            // Process window messages
            MSG msg;
            while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessageW(&msg);
            }

            // Handle cursor clipping
            if (m_viewClipped == false && m_cursorRelativeMode)
            {
                RECT rect =
                {
                    view.GetPosition().x, view.GetPosition().y,
                    view.GetPosition().x + view.GetSize().x, view.GetPosition().y + view.GetSize().y
                };

                ::ClipCursor(&rect);
                m_viewClipped = true;
            }
            else if (m_viewClipped && m_cursorRelativeMode == false)
            {
                ::ClipCursor(nullptr);
                m_viewClipped = false;
            }

            // Tick the application after updates have completed
            if (m_running.load())
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
        return m_allocator.New<ViewImpl>(this, desc);
    }

    void DeviceImpl::DestroyView(View* view)
    {
        m_allocator.Delete(view);
    }

    View* DeviceImpl::GetFocusedView() const
    {
        HWND hWnd = ::GetForegroundWindow();
        if (!hWnd || reinterpret_cast<HINSTANCE>(::GetWindowLongPtrW(hWnd, GWLP_HINSTANCE)) != m_hInstance)
            return nullptr;

        return reinterpret_cast<View*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }

    View* DeviceImpl::GetHoveredView() const
    {
        POINT pos;
        if (!::GetCursorPos(&pos))
            return nullptr;

        HWND hWnd = ::WindowFromPoint(pos);
        if (!hWnd || reinterpret_cast<HINSTANCE>(::GetWindowLongPtrW(hWnd, GWLP_HINSTANCE)) != m_hInstance)
            return nullptr;

        return reinterpret_cast<View*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }

    Vec2f DeviceImpl::GetCursorPos(View* view_) const
    {
        ViewImpl* view = static_cast<ViewImpl*>(view_);

        POINT p = {};
        if (!::GetCursorPos(&p))
            return Vec2f_Infinity;

        if (view)
            ::ScreenToClient(view->m_window, &p);

        return { static_cast<float>(p.x), static_cast<float>(p.y) };
    }

    void DeviceImpl::SetCursorPos(View* view_, const Vec2f& pos)
    {
        ViewImpl* view = static_cast<ViewImpl*>(view_);

        POINT p = { static_cast<LONG>(pos.x), static_cast<LONG>(pos.y) };

        if (view)
            ::ClientToScreen(view->m_window, &p);

        ::SetCursorPos(p.x, p.y);
    }

    void DeviceImpl::SetCursor(PointerCursor cursor)
    {
        if (m_cursor != cursor)
        {
            m_cursor = cursor;
            SyncCursor();
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

    static BOOL CALLBACK CountMonitorsCallback(HMONITOR, HDC, LPRECT, LPARAM dwData)
    {
        uint32_t* count = reinterpret_cast<uint32_t*>(dwData);
        (*count)++;
        return TRUE;
    }

    uint32_t DeviceImpl::GetMonitorCount() const
    {
        uint32_t count = 0;
        ::EnumDisplayMonitors(nullptr, nullptr, CountMonitorsCallback, reinterpret_cast<LPARAM>(&count));
        return count;
    }

    uint32_t DeviceImpl::GetMonitors(Monitor* monitors, uint32_t maxCount) const
    {
        VisitMonitorData data;
        data.device = this;
        data.monitors = monitors;
        data.index = 0;
        data.maxCount = maxCount;
        ::EnumDisplayMonitors(nullptr, nullptr, VisitMonitorsCallback, reinterpret_cast<LPARAM>(&data));
        return data.index;
    }

    Gamepad& DeviceImpl::GetGamepad(uint32_t index)
    {
        HE_ASSERT(index < HE_LENGTH_OF(m_gamepads));
        return m_gamepads[index];
    }

    void DeviceImpl::SyncCursor() const
    {
        if (!m_cursorVisible)
            return;

        if (m_cursor > PointerCursor::None && m_cursor < PointerCursor::_Count)
        {
            static LPCTSTR cursorNames[] =
            {
                IDC_ARROW,      // Arrow
                IDC_HAND,       // Hand
                IDC_NO,         // NotAllowed
                IDC_IBEAM,      // TextInput
                IDC_SIZEALL,    // ResizeAll
                IDC_SIZENWSE,   // ResizeTopLeft
                IDC_SIZENESW,   // ResizeTopRight
                IDC_SIZENESW,   // ResizeBottomLeft
                IDC_SIZENWSE,   // ResizeBottomRight
                IDC_SIZEWE,     // ResizeHorizontal
                IDC_SIZENS,     // ResizeVertical
                IDC_WAIT,       // Wait
            };
            static_assert(HE_LENGTH_OF(cursorNames) == static_cast<int32_t>(PointerCursor::_Count), "");
            ::SetCursor(::LoadCursorW(nullptr, cursorNames[static_cast<int32_t>(m_cursor)]));
        }
        else
        {
            ::SetCursor(nullptr);
        }
    }

    void DeviceImpl::ShowCursor(bool show)
    {
        if (show == m_cursorVisible)
            return;

        m_cursorVisible = show;

        if (!m_cursorVisible)
            ::SetCursor(nullptr);
        else
            SyncCursor();
    }

    void DeviceImpl::CenterCursor()
    {
        View* view = GetFocusedView();
        if (!view)
            return;

        const Vec2f pos = MakeVec2<float>(view->GetSize()) / 2.0f;
        SetCursorPos(view, pos);
    }

    static PointerButton GetPointerButton(POINTER_BUTTON_CHANGE_TYPE buttonType, PEN_FLAGS penFlags)
    {
        switch (buttonType)
        {
            case POINTER_CHANGE_FIRSTBUTTON_DOWN:
            case POINTER_CHANGE_FIRSTBUTTON_UP:
                return PointerButton::Primary;

            case POINTER_CHANGE_SECONDBUTTON_DOWN:
            case POINTER_CHANGE_SECONDBUTTON_UP:
                return PointerButton::Secondary;

            case POINTER_CHANGE_THIRDBUTTON_DOWN:
            case POINTER_CHANGE_THIRDBUTTON_UP:
                return PointerButton::Auxiliary;

            case POINTER_CHANGE_FOURTHBUTTON_DOWN:
            case POINTER_CHANGE_FOURTHBUTTON_UP:
                return PointerButton::Extra1;

            case POINTER_CHANGE_FIFTHBUTTON_DOWN:
            case POINTER_CHANGE_FIFTHBUTTON_UP:
                return PointerButton::Extra2;

            default:
                break;
        }

        if (HasFlag(penFlags, PEN_FLAG_ERASER))
            return PointerButton::Eraser;

        return PointerButton::None;
    }

    template <typename T>
    static T MakeAndCopyPointerEvent(const PointerEvent& data)
    {
        T ev(data.view);
        ev.pointerId = data.pointerId;
        ev.pointerKind = data.pointerKind;
        ev.size = data.size;
        ev.tilt = data.tilt;
        ev.rotation = data.rotation;
        ev.pressure = data.pressure;
        ev.isPrimary = data.isPrimary;
        return ev;
    }

    void DeviceImpl::DispatchPointerEvent(const PointerEvent& data, const POINTER_INFO& info, PEN_FLAGS penFlags)
    {
        if (HasFlag(info.pointerFlags, POINTER_FLAG_DOWN))
        {
            PointerDownEvent ev = MakeAndCopyPointerEvent<PointerDownEvent>(data);
            ev.button = GetPointerButton(info.ButtonChangeType, penFlags);
            static_cast<ViewImpl*>(ev.view)->TrackCapture(ev);
            m_app->OnEvent(ev);
        }
        else if (HasFlag(info.pointerFlags, POINTER_FLAG_UP))
        {
            PointerUpEvent ev = MakeAndCopyPointerEvent<PointerUpEvent>(data);
            ev.button = GetPointerButton(info.ButtonChangeType, penFlags);
            static_cast<ViewImpl*>(ev.view)->TrackCapture(ev);
            m_app->OnEvent(ev);
        }
        else if (HasFlag(info.pointerFlags, POINTER_FLAG_WHEEL))
        {
            PointerWheelEvent ev = MakeAndCopyPointerEvent<PointerWheelEvent>(data);
            const float delta = info.InputData / static_cast<float>(WHEEL_DELTA);
            ev.delta = { 0, delta };
            m_app->OnEvent(ev);
        }
        else if (HasFlag(info.pointerFlags, POINTER_FLAG_HWHEEL))
        {
            PointerWheelEvent ev = MakeAndCopyPointerEvent<PointerWheelEvent>(data);
            const float delta = info.InputData / static_cast<float>(WHEEL_DELTA);
            ev.delta = { delta, 0 };
            m_app->OnEvent(ev);
        }
        else if (HasFlag(info.pointerFlags, POINTER_FLAG_UPDATE))
        {
            if (data.pointerKind == PointerKind::Mouse && m_deviceInfo.hasHighDefMouse)
                return;

            PointerMoveEvent ev = MakeAndCopyPointerEvent<PointerMoveEvent>(data);
            ev.pos = { static_cast<float>(info.ptPixelLocation.x), static_cast<float>(info.ptPixelLocation.y) };
            ev.absolute = true;
            m_app->OnEvent(ev);
        }
    }

    void DeviceImpl::DispatchPointerEventMouse(View* view, const POINTER_INFO& info)
    {
        PointerEvent data(EventKind::PointerDown, view);
        data.pointerId = info.pointerId;
        data.pointerKind = PointerKind::Mouse;
        data.size = { 1, 1 };
        data.tilt = { 0, 0 };
        data.rotation = 0;
        data.pressure = 0;
        data.isPrimary = HasFlag(info.pointerFlags, POINTER_FLAG_PRIMARY);
        DispatchPointerEvent(data, info, PEN_FLAG_NONE);
    }

    void DeviceImpl::DispatchPointerEventPen(View* view, const POINTER_PEN_INFO& info)
    {
        const INT32 tiltX = HasFlag(info.penMask, PEN_MASK_TILT_X) ? info.tiltX : 0;
        const INT32 tiltY = HasFlag(info.penMask, PEN_MASK_TILT_Y) ? info.tiltY : 0;
        const UINT32 rotation = HasFlag(info.penMask, PEN_MASK_ROTATION) ? info.rotation : 0;
        const UINT32 pressure = HasFlag(info.penMask, PEN_MASK_PRESSURE) ? info.pressure : 0;

        PointerEvent data(EventKind::PointerDown, view);
        data.pointerId = info.pointerInfo.pointerId;
        data.pointerKind = PointerKind::Pen;
        data.size = { 1, 1 };
        data.tilt = { tiltX, tiltY };
        data.rotation = rotation;
        data.pressure = pressure / 1024.0f; // Normalize the win32 [0, 1024] range to our [0.0, 1.0] range
        data.isPrimary = HasFlag(info.pointerInfo.pointerFlags, POINTER_FLAG_PRIMARY);
        DispatchPointerEvent(data, info.pointerInfo, info.penFlags);
    }

    void DeviceImpl::DispatchPointerEventTouch(View* view, const POINTER_TOUCH_INFO& info)
    {
        const RECT contact = HasFlag(info.touchMask, TOUCH_MASK_CONTACTAREA) ? info.rcContact : RECT{};
        const UINT32 rotation = HasFlag(info.touchMask, TOUCH_MASK_ORIENTATION) ? info.orientation : 0;
        const UINT32 pressure = HasFlag(info.touchMask, TOUCH_MASK_PRESSURE) ? info.pressure : 0;

        const LONG width = contact.right - contact.left;
        const LONG height = contact.bottom - contact.top;

        PointerEvent data(EventKind::PointerDown, view);
        data.pointerId = info.pointerInfo.pointerId;
        data.pointerKind = PointerKind::Touch;
        data.size = { width ? width : 1, height ? height : 1 };
        data.tilt = { 0, 0 };
        data.rotation = rotation;
        data.pressure = pressure / 1024.0f; // Normalize the win32 [0, 1024] range to our [0.0, 1.0] range
        data.isPrimary = HasFlag(info.pointerInfo.pointerFlags, POINTER_FLAG_PRIMARY);
        DispatchPointerEvent(data, info.pointerInfo, PEN_FLAG_NONE);
    }

    bool DeviceImpl::HandlePointerFrameMouse(View* view, PointerId pointerId)
    {
        if (!m_GetPointerFrameInfo)
            return false;

        UINT32 infoCount = 0;
        if (!m_GetPointerFrameInfo(pointerId, &infoCount, NULL) || infoCount == 0)
            return false;

    #if HE_ENABLE_ASSERTIONS
        const UINT32 prevInfoCount = infoCount;
    #endif
        POINTER_INFO* infos = HE_ALLOCA(POINTER_INFO, infoCount);
        if (!m_GetPointerFrameInfo(pointerId, &infoCount, infos))
            return false;

        HE_ASSERT(prevInfoCount == infoCount);

        for (UINT32 i = 0; i < infoCount; ++i)
        {
            const POINTER_INFO& info = infos[i];
            DispatchPointerEventMouse(view, info);
        }

        return true;
    }

    bool DeviceImpl::HandlePointerFrameHistoryMouse(View* view, PointerId pointerId)
    {
        if (!m_GetPointerFrameInfoHistory)
            return false;

        UINT32 entryCount = 0;
        UINT32 pointerCount = 0;
        if (!m_GetPointerFrameInfoHistory(pointerId, &entryCount, &pointerCount, NULL))
            return false;

        if (entryCount == 0 || pointerCount == 0)
            return false;

        const UINT32 totalInfoCount = entryCount * pointerCount;
        POINTER_INFO* infos = HE_ALLOCA(POINTER_INFO, totalInfoCount);
        if (!m_GetPointerFrameInfoHistory(pointerId, &entryCount, &pointerCount, infos))
            return false;

        HE_ASSERT(totalInfoCount == (entryCount * pointerCount));

        // entries are in reverse chronological order, so we need to loop in reverse.
        UINT32 i = totalInfoCount;
        while (i)
        {
            --i;
            const POINTER_INFO& info = infos[i];
            DispatchPointerEventMouse(view, info);
        }

        return true;
    }

    bool DeviceImpl::HandlePointerFramePen(View* view, PointerId pointerId)
    {
        if (!m_GetPointerFramePenInfo)
            return false;

        UINT32 infoCount = 0;
        if (!m_GetPointerFramePenInfo(pointerId, &infoCount, NULL) || infoCount == 0)
            return false;

    #if HE_ENABLE_ASSERTIONS
        const UINT32 prevInfoCount = infoCount;
    #endif
        POINTER_PEN_INFO* infos = HE_ALLOCA(POINTER_PEN_INFO, infoCount);
        if (!m_GetPointerFramePenInfo(pointerId, &infoCount, infos))
            return false;

        HE_ASSERT(prevInfoCount == infoCount);

        for (UINT32 i = 0; i < infoCount; ++i)
        {
            const POINTER_PEN_INFO& info = infos[i];
            DispatchPointerEventPen(view, info);
        }

        return true;
    }

    bool DeviceImpl::HandlePointerFrameHistoryPen(View* view, PointerId pointerId)
    {
        if (!m_GetPointerFramePenInfoHistory)
            return false;

        UINT32 entryCount = 0;
        UINT32 pointerCount = 0;
        if (!m_GetPointerFramePenInfoHistory(pointerId, &entryCount, &pointerCount, NULL))
            return false;

        if (entryCount == 0 || pointerCount == 0)
            return false;

        const UINT32 totalInfoCount = entryCount * pointerCount;
        POINTER_PEN_INFO* infos = HE_ALLOCA(POINTER_PEN_INFO, totalInfoCount);
        if (!m_GetPointerFramePenInfoHistory(pointerId, &entryCount, &pointerCount, infos))
            return false;

        HE_ASSERT(totalInfoCount == (entryCount * pointerCount));

        // entries are in reverse chronological order, so we need to loop in reverse.
        UINT32 i = totalInfoCount;
        while (i)
        {
            --i;
            const POINTER_PEN_INFO& info = infos[i];
            DispatchPointerEventPen(view, info);
        }

        return true;
    }

    bool DeviceImpl::HandlePointerFrameTouch(View* view, PointerId pointerId)
    {
        if (!m_GetPointerFrameTouchInfo)
            return false;

        UINT32 infoCount = 0;
        if (!m_GetPointerFrameTouchInfo(pointerId, &infoCount, NULL) || infoCount == 0)
            return false;

    #if HE_ENABLE_ASSERTIONS
        const UINT32 prevInfoCount = infoCount;
    #endif
        POINTER_TOUCH_INFO* infos = HE_ALLOCA(POINTER_TOUCH_INFO, infoCount);
        if (!m_GetPointerFrameTouchInfo(pointerId, &infoCount, infos))
            return false;

        HE_ASSERT(prevInfoCount == infoCount);

        for (UINT32 i = 0; i < infoCount; ++i)
        {
            const POINTER_TOUCH_INFO& info = infos[i];
            DispatchPointerEventTouch(view, info);
        }

        return true;
    }

    bool DeviceImpl::HandlePointerFrameHistoryTouch(View* view, PointerId pointerId)
    {
        if (!m_GetPointerFrameTouchInfoHistory)
            return false;

        UINT32 entryCount = 0;
        UINT32 pointerCount = 0;
        if (!m_GetPointerFrameTouchInfoHistory(pointerId, &entryCount, &pointerCount, NULL))
            return false;

        if (entryCount == 0 || pointerCount == 0)
            return false;

        const UINT32 totalInfoCount = entryCount * pointerCount;
        POINTER_TOUCH_INFO* infos = HE_ALLOCA(POINTER_TOUCH_INFO, totalInfoCount);
        if (!m_GetPointerFrameTouchInfoHistory(pointerId, &entryCount, &pointerCount, infos))
            return false;

        HE_ASSERT(totalInfoCount == (entryCount * pointerCount));

        // entries are in reverse chronological order, so we need to loop in reverse.
        UINT32 i = totalInfoCount;
        while (i)
        {
            --i;
            const POINTER_TOUCH_INFO& info = infos[i];
            DispatchPointerEventTouch(view, info);
        }

        return true;
    }
}

namespace he::window
{
    Device* _CreateWindowDevice(Allocator& allocator)
    {
        return allocator.New<win32::DeviceImpl>(allocator);
    }
}

#endif
