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
                ViewMovedEvent ev(view, view->m_pos);
                app->OnEvent(ev);
                return 0;
            }
            case WM_SIZE:
            {
                view->m_size = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                ViewResizedEvent ev(view, view->m_size);
                app->OnEvent(ev);
                return 0;
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
            case WM_NCLBUTTONDOWN:
            case WM_NCRBUTTONDOWN:
            case WM_NCMBUTTONDOWN:
            {
                if (HasFlag(view->m_flags, ViewFlag::Borderless))
                {
                    // Windows handles HTCLOSE by closing the window, but doesn't seem to do
                    // anything for the other button hit tests. The default HTCLOSE handling
                    // also seems to care about mouse position in a way that doesn't always
                    // align to the application's view of the window. We want to expose
                    // the button values from WM_NCHITTEST to get the accessibility features
                    // (like screen reader support) that Windows provides by default. However,
                    // we prevent the default OS behavior on click and let the application
                    // handle it instead to enable more advanced handling and consistency.
                    if (wParam == HTCLOSE || wParam == HTMINBUTTON || wParam == HTMAXBUTTON)
                    {
                        MouseButton button = MouseButton::None;
                        switch (message)
                        {
                            case WM_NCLBUTTONDOWN: button = MouseButton::Left; break;
                            case WM_NCRBUTTONDOWN: button = MouseButton::Right; break;
                            case WM_NCMBUTTONDOWN: button = MouseButton::Middle; break;
                        }

                        MouseDownEvent ev(view, button);
                        view->TrackCapture(ev);
                        app->OnEvent(ev);
                        return 0;
                    }
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
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_XBUTTONDOWN:
            case WM_LBUTTONDBLCLK:
            case WM_RBUTTONDBLCLK:
            case WM_MBUTTONDBLCLK:
            case WM_XBUTTONDBLCLK:
            {
                MouseButton button = MouseButton::None;
                switch (message)
                {
                    case WM_LBUTTONDOWN:
                    case WM_LBUTTONDBLCLK:
                        button = MouseButton::Left;
                        break;
                    case WM_RBUTTONDOWN:
                    case WM_RBUTTONDBLCLK:
                        button = MouseButton::Right;
                        break;
                    case WM_MBUTTONDOWN:
                    case WM_MBUTTONDBLCLK:
                        button = MouseButton::Middle;
                        break;
                    case WM_XBUTTONDOWN:
                    case WM_XBUTTONDBLCLK:
                        button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? MouseButton::Extra1 : MouseButton::Extra2;
                        break;
                }

                MouseDownEvent ev(view, button);
                view->TrackCapture(ev);
                app->OnEvent(ev);
                return 0;
            }
            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_MBUTTONUP:
            case WM_XBUTTONUP:
            {
                MouseButton button = MouseButton::None;
                switch (message)
                {
                    case WM_LBUTTONUP: button = MouseButton::Left; break;
                    case WM_RBUTTONUP: button = MouseButton::Right; break;
                    case WM_MBUTTONUP: button = MouseButton::Middle; break;
                    case WM_XBUTTONUP: button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? MouseButton::Extra1 : MouseButton::Extra2; break;
                }

                MouseUpEvent ev(view, button);
                view->TrackCapture(ev);
                app->OnEvent(ev);
                return 0;
            }
            case WM_MOUSEWHEEL:
            {
                const float delta = GET_WHEEL_DELTA_WPARAM(wParam) / static_cast<float>(WHEEL_DELTA);
                MouseWheelEvent ev(view, { 0, delta });
                app->OnEvent(ev);
                return 0;
            }
            case WM_MOUSEHWHEEL:
            {
                const float delta = GET_WHEEL_DELTA_WPARAM(wParam) / static_cast<float>(WHEEL_DELTA);
                MouseWheelEvent ev(view, { delta, 0 });
                app->OnEvent(ev);
                return 0;
            }
            case WM_NCMOUSEMOVE:
            case WM_MOUSEMOVE:
            {
                if (device->m_hasHighDefMouse)
                    break;

                POINT pos{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                ::ClientToScreen(hWnd, &pos);

                Vec2f posf{ static_cast<float>(pos.x), static_cast<float>(pos.y) };
                MouseMoveEvent ev(view, posf, true);

                if (device->m_cursorRelativeMode)
                    device->CenterCursor();

                app->OnEvent(ev);
                return 0;
            }
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            {
                Key key = TranslateKey(wParam, lParam);
                if (key != Key::None)
                {
                    KeyDownEvent ev(view, key);
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
                    KeyUpEvent ev(view, key);
                    app->OnEvent(ev);
                }
                return 0;
            }
            case WM_CHAR:
            {
                if (wParam > 0 && wParam < 0x10000)
                {
                    TextEvent ev(view, LOWORD(wParam));
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
                ViewActivatedEvent ev(view, active);
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

                ViewDpiScaleChangedEvent ev(view, view->GetDpiScale());
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
                    view->m_device->m_refreshGamepadConnectivity = true;
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

                RAWINPUT raw;
                UINT rawSize = sizeof(raw);
                UINT size = ::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &rawSize, sizeof(RAWINPUTHEADER));

                if (size != rawSize)
                    break;

                if (raw.header.dwType != RIM_TYPEMOUSE)
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

                MouseMoveEvent ev(view, pos, absolute);

                if (device->m_cursorRelativeMode)
                    device->CenterCursor();

                app->OnEvent(ev);
                return 0;
            }
            case WM_DROPFILES:
            {
                if (!HasFlag(view->m_flags, ViewFlag::AcceptFiles))
                    break;

                HDROP drop = reinterpret_cast<HDROP>(wParam);

                // Move the mouse to the position of the drop
                POINT pt{};
                if (!device->m_cursorRelativeMode && ::DragQueryPoint(drop, &pt))
                {
                    ::ClientToScreen(hWnd, &pt);

                    Vec2f pos{ static_cast<float>(pt.x), static_cast<float>(pt.y) };
                    MouseMoveEvent ev(view, pos, true);

                    app->OnEvent(ev);
                }

                // Read the dropped paths
                const UINT count = ::DragQueryFileW(drop, 0xffffffff, NULL, 0);
                Vector<String> paths;
                paths.Resize(count);

                Vector<wchar_t> buf;

                for (UINT i = 0; i < count; ++i)
                {
                    const UINT length = ::DragQueryFileW(drop, i, NULL, 0);
                    buf.Resize(length + 1);

                    ::DragQueryFileW(drop, i, buf.Data(), buf.Size());
                    WCToMBStr(paths[i], buf.Data());
                }

                ViewDropFilesEvent ev(view, paths);
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
        m_hInstance = ::GetModuleHandleW(nullptr);

        // Bind to optional Windows APIs
        m_userLib = ::LoadLibraryW(L"user32.dll");
        if (m_userLib)
        {
            m_GetDpiForWindow = reinterpret_cast<Pfn_GetDpiForWindow>(::GetProcAddress(m_userLib, "GetDpiForWindow"));
            m_SetProcessDpiAwarenessContext = reinterpret_cast<Pfn_SetProcessDpiAwarenessContext>(::GetProcAddress(m_userLib, "SetProcessDpiAwarenessContext"));
            m_AdjustWindowRectExForDpi = reinterpret_cast<Pfn_AdjustWindowRectExForDpi>(::GetProcAddress(m_userLib, "AdjustWindowRectExForDpi"));
            m_ChangeWindowMessageFilterEx = reinterpret_cast<Pfn_ChangeWindowMessageFilterEx>(::GetProcAddress(m_userLib, "ChangeWindowMessageFilterEx"));
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

        // Enable raw mouse input
        RAWINPUTDEVICE rid;
        rid.usUsagePage = 0x01; // HID_USAGE_PAGE_GENERIC
        rid.usUsage = 0x02; // HID_USAGE_GENERIC_MOUSE
        rid.dwFlags = 0;
        rid.hwndTarget = nullptr;
        if (::RegisterRawInputDevices(&rid, 1, sizeof(rid)))
        {
            m_hasHighDefMouse = true;
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

    bool DeviceImpl::HasHighDefMouse() const
    {
        return m_hasHighDefMouse;
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

    void DeviceImpl::SetCursor(MouseCursor cursor)
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

        if (m_cursor > MouseCursor::None && m_cursor < MouseCursor::_Count)
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
            static_assert(HE_LENGTH_OF(cursorNames) == static_cast<int32_t>(MouseCursor::_Count), "");
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
}

namespace he::window
{
    Device* _CreateDevice(Allocator& allocator)
    {
        return allocator.New<win32::DeviceImpl>(allocator);
    }
}

#endif
