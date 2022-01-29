// Copyright Chad Engler
// TODO: drag and drop (AcceptFiles)

#include "he/window/application.h"
#include "he/window/device.h"
#include "he/window/event.h"
#include "he/window/gamepad.h"
#include "he/window/key.h"
#include "he/window/mouse.h"
#include "he/window/view.h"

#include "he/core/alloca.h"
#include "he/core/compiler.h"
#include "he/core/log.h"
#include "he/core/memory_ops.h"
#include "he/core/string.h"
#include "he/core/utils.h"
#include "he/core/wstr.h"
#include "he/math/vec2.h"

#include <atomic>
#include <array>
#include <cstdint>

#if defined(HE_PLATFORM_WINDOWS)

#if !defined(NOMINMAX)
    #define NOMINMAX
#endif

#pragma warning(push)
#pragma warning(disable : 4668)

#include <sdkddkver.h>
#include <shellscalingapi.h>
#include <Shlobj.h>
#include <Windows.h>
#include <Xinput.h>

#pragma warning(pop)

// From Windowsx.h
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

#if !defined(DBT_DEVNODES_CHANGED)
    #define DBT_DEVNODES_CHANGED 0x0007
#endif

namespace he::window::Win32
{
    class DeviceImpl;
    class ViewImpl;

    // ------------------------------------------------------------------------------------------------
    using Pfn_GetDpiForWindow = UINT (WINAPI*)(_In_ HWND hwnd);
    using Pfn_SetProcessDpiAwarenessContext = DPI_AWARENESS_CONTEXT (WINAPI*)(_In_ DPI_AWARENESS_CONTEXT dpiContext);
    using Pfn_AdjustWindowRectExForDpi = BOOL (WINAPI*)(_Inout_ LPRECT lpRect, _In_ DWORD dwStyle, _In_ BOOL bMenu, _In_ DWORD dwExStyle, _In_ UINT dpi);

    using Pfn_GetDpiForMonitor = HRESULT (WINAPI*)(_In_ HMONITOR hmonitor, _In_ MONITOR_DPI_TYPE dpiType, _Out_ UINT* dpiX, _Out_ UINT* dpiY);

    using Pfn_XInputGetState = DWORD (WINAPI*)(_In_ DWORD dwUserIndex, _Out_ XINPUT_STATE* pState);
    using Pfn_XInputEnable = VOID (WINAPI*)(_In_ BOOL enable);

    constexpr wchar_t WindowClassName[] = L"Harvest Application Window";

    struct XInputButtonMapping { uint32_t flag; GamepadButton button; };
    constexpr XInputButtonMapping XInputButtonMappings[] =
    {
        { XINPUT_GAMEPAD_DPAD_UP, GamepadButton::DPad_Up },
        { XINPUT_GAMEPAD_DPAD_DOWN, GamepadButton::DPad_Down },
        { XINPUT_GAMEPAD_DPAD_LEFT, GamepadButton::DPad_Left },
        { XINPUT_GAMEPAD_DPAD_RIGHT, GamepadButton::DPad_Right },
        { XINPUT_GAMEPAD_START, GamepadButton::Start },
        { XINPUT_GAMEPAD_BACK, GamepadButton::Back },
        { XINPUT_GAMEPAD_LEFT_THUMB, GamepadButton::LThumb },
        { XINPUT_GAMEPAD_RIGHT_THUMB, GamepadButton::RThumb },
        { XINPUT_GAMEPAD_LEFT_SHOULDER, GamepadButton::LShoulder },
        { XINPUT_GAMEPAD_RIGHT_SHOULDER, GamepadButton::RShoulder },
        { XINPUT_GAMEPAD_A, GamepadButton::Action1 },
        { XINPUT_GAMEPAD_B, GamepadButton::Action2 },
        { XINPUT_GAMEPAD_X, GamepadButton::Action3 },
        { XINPUT_GAMEPAD_Y, GamepadButton::Action4 },
    };

    // --------------------------------------------------------------------------------------------
    class ViewImpl final : public View
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

        void TrackCapture(const Event& ev);
        void AdjustWindowRegion();

    public:
        DeviceImpl* m_device{ nullptr };
        HWND m_window{ nullptr };
        ViewFlag m_flags{ ViewFlag::Default };
        void* m_userData{ nullptr };
        int m_captureCount{ 0 };
        Vec2i m_pos{ 0, 0 };
        Vec2i m_size{ 1, 1 };
        uint32_t m_dpi{ USER_DEFAULT_SCREEN_DPI };
        bool m_hackNeedsFrameUpdate{ true };
    };

    // --------------------------------------------------------------------------------------------
    class DeviceImpl final : public Device
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

        void UpdateGamepads();

        void SyncCursor() const;
        void ShowCursor(bool show);
        void CenterCursor();

    public:
        Application* m_app{ nullptr };
        HINSTANCE m_hInstance{ nullptr };
        MouseCursor m_cursor{ MouseCursor::Arrow };
        std::atomic<int32_t> m_returnCode{ 0 };
        std::atomic<bool> m_running{ true };
        bool m_cursorRelativeMode{ false };
        bool m_cursorVisible{ true };
        bool m_hasHighDefMouse{ false };
        bool m_viewClipped{ false };
        Vec2f m_cursorRestorePosition{ 0, 0 };

        HMODULE m_userLib{ nullptr };
        Pfn_GetDpiForWindow m_GetDpiForWindow{ nullptr };
        Pfn_SetProcessDpiAwarenessContext m_SetProcessDpiAwarenessContext{ nullptr };
        Pfn_AdjustWindowRectExForDpi m_AdjustWindowRectExForDpi{ nullptr };

        HMODULE m_shcoreLib{ nullptr };
        Pfn_GetDpiForMonitor m_GetDpiForMonitor{ nullptr };

        HMODULE m_xinputLib{ nullptr };
        Pfn_XInputGetState m_XInputGetState{ nullptr };
        Pfn_XInputEnable m_XInputEnable{ nullptr };

        static_assert(MaxGamepads <= XUSER_MAX_COUNT, "Cannot handle more than XUSER_MAX_COUNT gamepads.");

        struct Gamepad
        {
            XINPUT_STATE state{};
            bool connected{ false };
        };
        Gamepad m_gamepads[MaxGamepads]{};
        bool m_refreshGamepadConnectivity{ true };
    };

    // --------------------------------------------------------------------------------------------
    static Key TranslateKey(WPARAM wparam)
    {
        int32_t vkey = wparam & 0xff;
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
                        HE_ASSERT(false, "Unknown hit area.");
                        HE_UNREACHABLE();
                    }
                }
                break;
            }
            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_XBUTTONDOWN:
            {
                MouseButton button = MouseButton::None;
                switch (message)
                {
                    case WM_LBUTTONDOWN: button = MouseButton::Left; break;
                    case WM_RBUTTONDOWN: button = MouseButton::Right; break;
                    case WM_MBUTTONDOWN: button = MouseButton::Middle; break;
                    case WM_XBUTTONDOWN: button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? MouseButton::Extra1 : MouseButton::Extra2; break;
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

                const float xpos = static_cast<float>(GET_X_LPARAM(lParam));
                const float ypos = static_cast<float>(GET_Y_LPARAM(lParam));
                MouseMoveEvent ev(view, { xpos, ypos }, true, false);

                if (device->m_cursorRelativeMode)
                    device->CenterCursor();

                app->OnEvent(ev);
                return 0;
            }
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            {
                Key key = TranslateKey(wParam);
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
                Key key = TranslateKey(wParam);
                if (key != Key::None)
                {
                    KeyUpEvent ev(view, key);
                    app->OnEvent(ev);
                }
                return 0;
            }
            case WM_CHAR:
            {
                TextEvent ev(view, LOWORD(wParam));
                app->OnEvent(ev);
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
                    return 0;

                if (raw.header.dwType == RIM_TYPEMOUSE)
                {
                    const bool absolute = (raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0;
                    const float lastX = static_cast<float>(raw.data.mouse.lLastX);
                    const float lastY = static_cast<float>(raw.data.mouse.lLastY);

                    MouseMoveEvent ev(view, { lastX, lastY }, absolute, true);

                    if (device->m_cursorRelativeMode)
                        device->CenterCursor();

                    app->OnEvent(ev);
                }
                return 0;
            }
        }

        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    // --------------------------------------------------------------------------------------------
    ViewImpl::ViewImpl(DeviceImpl* device, const ViewDesc& desc)
        : m_device(device)
        , m_userData(desc.userData)
        , m_flags(desc.flags)
    {
        const char* title = desc.title ? desc.title : "Harvest Window";
        wchar_t* wtitle = HE_TO_WSTR(title);

        // Create window
        DWORD dwStyle = WS_SYSMENU;
        DWORD dwExStyle = 0;

        if (!HasFlag(m_flags, ViewFlag::AcceptInput))
            dwExStyle |= WS_EX_TRANSPARENT;

        if (HasFlag(m_flags, ViewFlag::Borderless))
            dwStyle |= WS_POPUP;
        else
            dwStyle |= WS_OVERLAPPED;

        // WS_CAPTION: enables aero minimize animation/transition
        // WS_MAXIMIZEBOX, WS_MINIMIZEBOX: enable minimize/maximize
        // WS_THICKFRAME: without this the window cannot be resized and so aero snap, de-maximizing and minimizing won't work
        if (HasFlag(m_flags, ViewFlag::AllowResize))
            dwStyle |= WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME;

        if (HasFlag(m_flags, ViewFlag::StartMaximized))
            dwStyle |= WS_MAXIMIZE;

        if (HasFlag(m_flags, ViewFlag::TaskBarIcon))
            dwExStyle |= WS_EX_APPWINDOW;
        else
            dwExStyle |= WS_EX_TOOLWINDOW;

        if (HasFlag(m_flags, ViewFlag::TopMost))
            dwExStyle |= WS_EX_TOPMOST;

        RECT rect = { 0, 0, 1920, 1080 };
        if (desc.size.x > 0 && desc.size.y > 0)
        {
            rect.left = desc.pos.x;
            rect.top = desc.pos.y;
            rect.right = rect.left + desc.size.x;
            rect.bottom = rect.top + desc.size.y;
        }

        if (device->m_AdjustWindowRectExForDpi)
            device->m_AdjustWindowRectExForDpi(&rect, dwStyle, FALSE, dwExStyle, m_dpi);
        else
            ::AdjustWindowRectEx(&rect, dwStyle, FALSE, dwExStyle);

        if (desc.pos.x != 0 || desc.pos.y != 0)
            m_pos = { rect.left, rect.top };

        m_size = { rect.right - rect.left, rect.bottom - rect.top };

        HWND hParent = desc.parent ? static_cast<HWND>(desc.parent->GetNativeHandle()) : nullptr;

        m_window = ::CreateWindowExW(
            dwExStyle,
            WindowClassName,
            wtitle,
            dwStyle,
            m_pos.x,
            m_pos.y,
            m_size.x,
            m_size.y,
            hParent,
            nullptr,
            device->m_hInstance,
            this);

        HE_ASSERT(m_window != 0, "CreateWindowEx failed.");

        // Inform Windows that it should redraw our frame styles
        ::SetWindowPos(m_window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

        // Cache the DPI of the window now that it is created
        if (device->m_GetDpiForWindow)
            m_dpi = device->m_GetDpiForWindow(m_window);
    }

    ViewImpl::~ViewImpl()
    {
        if (::GetCapture() == m_window)
        {
            HWND parent = ::GetParent(m_window);
            if (parent != nullptr)
                {
                // Transfer capture so if we started dragging from a window that later disappears, we'll
                // still receive the MOUSEUP event.
                ::ReleaseCapture();
                ::SetCapture(parent);
            }
        }

        // Any messages handled beyond this point need to fail to get the View.
        ::SetWindowLongPtrW(m_window, GWLP_USERDATA, 0);
        ::DestroyWindow(m_window);
    }

    void* ViewImpl::GetNativeHandle() const
    {
        return m_window;
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
        return static_cast<float>(m_dpi) / USER_DEFAULT_SCREEN_DPI;
    }

    bool ViewImpl::IsFocused() const
    {
        return ::GetForegroundWindow() == m_window;
    }

    bool ViewImpl::IsMinimized() const
    {
        return ::IsIconic(m_window) != 0;
    }

    bool ViewImpl::IsMaximized() const
    {
        WINDOWPLACEMENT placement;
        placement.length = sizeof(WINDOWPLACEMENT);
        if (!::GetWindowPlacement(m_window, &placement))
            return false;
        return placement.showCmd == SW_SHOWMAXIMIZED;
    }

    void ViewImpl::SetPosition(const Vec2i& pos)
    {
        DWORD dwStyle = GetWindowLongW(m_window, GWL_STYLE);
        DWORD dwExStyle = GetWindowLongW(m_window, GWL_EXSTYLE);

        RECT rect = { static_cast<LONG>(pos.x), static_cast<LONG>(pos.y), static_cast<LONG>(pos.x), static_cast<LONG>(pos.y) };
        if (m_device->m_AdjustWindowRectExForDpi)
            m_device->m_AdjustWindowRectExForDpi(&rect, dwStyle, FALSE, dwExStyle, m_dpi);
        else
            ::AdjustWindowRectEx(&rect, dwStyle, FALSE, dwExStyle);

        ::SetWindowPos(m_window, nullptr, rect.left, rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    void ViewImpl::SetSize(const Vec2i& size)
    {
        DWORD dwStyle = GetWindowLongW(m_window, GWL_STYLE);
        DWORD dwExStyle = GetWindowLongW(m_window, GWL_EXSTYLE);

        RECT rect = { 0, 0, static_cast<LONG>(size.x), static_cast<LONG>(size.y) };
        if (m_device->m_AdjustWindowRectExForDpi)
            m_device->m_AdjustWindowRectExForDpi(&rect, dwStyle, FALSE, dwExStyle, m_dpi);
        else
            ::AdjustWindowRectEx(&rect, dwStyle, FALSE, dwExStyle);

        ::SetWindowPos(m_window, nullptr, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
    }

    void ViewImpl::SetVisible(bool visible, bool focus)
    {
        int nCmdShow = SW_HIDE;
        if (visible)
        {
            if (focus)
                nCmdShow = SW_SHOW;
            else
                nCmdShow = SW_SHOWNA;
        }

        ::ShowWindow(m_window, nCmdShow);
    }

    void ViewImpl::SetTitle(const char* text)
    {
        wchar_t* wtext = HE_TO_WSTR(text);
        ::SetWindowTextW(m_window, wtext);
    }

    void ViewImpl::SetAlpha(float alpha)
    {
        if (alpha < 1.0f)
        {
            DWORD style = ::GetWindowLongW(m_window, GWL_EXSTYLE) | WS_EX_LAYERED;
            ::SetWindowLongW(m_window, GWL_EXSTYLE, style);
            ::SetLayeredWindowAttributes(m_window, 0, BYTE(255 * alpha), LWA_ALPHA);
        }
        else
        {
            DWORD style = ::GetWindowLongW(m_window, GWL_EXSTYLE) & ~WS_EX_LAYERED;
            ::SetWindowLongW(m_window, GWL_EXSTYLE, style);
        }
    }

    void ViewImpl::Focus()
    {
        ::BringWindowToTop(m_window);
        ::SetForegroundWindow(m_window);
        ::SetFocus(m_window);
    }

    void ViewImpl::Minimize()
    {
        ::ShowWindow(m_window, SW_MINIMIZE);
    }

    void ViewImpl::Maximize()
    {
        ::ShowWindow(m_window, SW_MAXIMIZE);
    }

    void ViewImpl::Restore()
    {
        ::ShowWindow(m_window, SW_NORMAL);
    }

    void ViewImpl::RequestClose()
    {
        ::PostMessage(m_window, WM_CLOSE, 0, 0);
    }

    Vec2f ViewImpl::ViewToScreen(const Vec2f& pos) const
    {
        POINT p = { static_cast<LONG>(pos.x), static_cast<LONG>(pos.y) };

        ::ClientToScreen(m_window, &p);

        return { static_cast<float>(p.x), static_cast<float>(p.y) };
    }

    Vec2f ViewImpl::ScreenToView(const Vec2f& pos) const
    {
        POINT p = { static_cast<LONG>(pos.x), static_cast<LONG>(pos.y) };

        ::ScreenToClient(m_window, &p);

        return { static_cast<float>(p.x), static_cast<float>(p.y) };
    }

    void ViewImpl::TrackCapture(const Event& ev)
    {
        HE_ASSERT(ev.type == EventType::MouseDown || ev.type == EventType::MouseUp);

        if (ev.type == EventType::MouseDown)
        {
            if (m_captureCount++ == 0)
                ::SetCapture(m_window);
        }
        else
        {
            if (--m_captureCount == 0)
                ::ReleaseCapture();
        }
    }

    // --------------------------------------------------------------------------------------------
    DeviceImpl::DeviceImpl(Allocator& allocator)
        : Device(allocator)
    {}

    DeviceImpl::~DeviceImpl()
    {
        ::UnregisterClassW(WindowClassName, m_hInstance);
        ::FreeLibrary(m_userLib);
        ::FreeLibrary(m_shcoreLib);
        ::FreeLibrary(m_xinputLib);
    }

    bool DeviceImpl::Initialize()
    {
        m_hInstance = GetModuleHandleW(nullptr);

        // Bind to optional Windows APIs
        m_userLib = LoadLibraryW(L"user32.dll");
        if (m_userLib)
        {
            m_GetDpiForWindow = reinterpret_cast<Pfn_GetDpiForWindow>(::GetProcAddress(m_userLib, "GetDpiForWindow"));
            m_SetProcessDpiAwarenessContext = reinterpret_cast<Pfn_SetProcessDpiAwarenessContext>(::GetProcAddress(m_userLib, "SetProcessDpiAwarenessContext"));
            m_AdjustWindowRectExForDpi = reinterpret_cast<Pfn_AdjustWindowRectExForDpi>(::GetProcAddress(m_userLib, "AdjustWindowRectExForDpi"));
        }

        m_shcoreLib = LoadLibraryW(L"Shcore.dll");
        if (m_shcoreLib)
        {
            m_GetDpiForMonitor = reinterpret_cast<Pfn_GetDpiForMonitor>(::GetProcAddress(m_shcoreLib, "GetDpiForMonitor"));
        }

        m_xinputLib = LoadLibraryW(L"XInput1_4.dll");
        if (!m_xinputLib)
            m_xinputLib = LoadLibraryW(L"XInput1_3.dll");
        if (m_xinputLib)
        {
            m_XInputGetState = reinterpret_cast<Pfn_XInputGetState>(::GetProcAddress(m_xinputLib, "XInputGetState"));
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
        wcex.lpszClassName = WindowClassName;
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

        // Initialized event
        {
            InitializedEvent ev(&view);
            app.OnEvent(ev);
        }

        // Event loop
        while (m_running.load())
        {
            UpdateGamepads();

            MSG msg;
            while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessageW(&msg);
            }

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

            if (m_running.load())
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
        ::GetCursorPos(&p);

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
            monitor.primary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;

            if (data->device->m_GetDpiForMonitor)
            {
                UINT xdpi = 96;
                UINT ydpi = 96;
                data->device->m_GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &xdpi, &ydpi);
                HE_ASSERT(xdpi == ydpi);
                monitor.dpiScale = xdpi / 96.0f;
            }
        }

        data->index++;
        return TRUE;
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

    void DeviceImpl::UpdateGamepads()
    {
        if (!m_XInputGetState)
            return;

        for (uint32_t i = 0; i < MaxGamepads; ++i)
        {
            Gamepad& gamepad = m_gamepads[i];

            if (!gamepad.connected && !m_refreshGamepadConnectivity)
                continue;

            XINPUT_STATE xstate{};
            DWORD error = m_XInputGetState(i, &xstate);
            const bool connected = error == ERROR_SUCCESS;

            // If we got a real error, skip this one
            if (!connected && error != ERROR_DEVICE_NOT_CONNECTED)
            {
                gamepad.connected = false;
                HE_LOG_ERROR(window, HE_MSG("Failed to read gamepad state"), HE_KV(index, i), HE_KV(error, error));
                continue;
            }

            // If connected state changed notify the app.
            if (connected != gamepad.connected)
            {
                gamepad.connected = connected;

                if (connected)
                {
                    MemZero(&gamepad.state, sizeof(gamepad.state));
                    GamepadConnectedEvent ev(i);
                    m_app->OnEvent(ev);
                }
                else
                {
                    GamepadDisconnectedEvent ev(i);
                    m_app->OnEvent(ev);
                }
            }

            if (!connected)
                continue;

            XINPUT_GAMEPAD& pad = gamepad.state.Gamepad;

            const WORD buttons = xstate.Gamepad.wButtons;
            const WORD diff = pad.wButtons ^ buttons;
            if (diff != 0)
            {
                pad.wButtons = buttons;

                for (const XInputButtonMapping& mapping : XInputButtonMappings)
                {
                    if (mapping.flag & diff) // changed
                    {
                        if (mapping.flag & buttons) // is pressed
                        {
                            GamepadButtonDownEvent ev(i, mapping.button);
                            m_app->OnEvent(ev);
                        }
                        else
                        {
                            GamepadButtonUpEvent ev(i, mapping.button);
                            m_app->OnEvent(ev);
                        }
                    }
                }
            }

            if (xstate.Gamepad.sThumbLX != pad.sThumbLX)
            {
                pad.sThumbLX = xstate.Gamepad.sThumbLX;

                GamepadAxisEvent ev(i, GamepadAxis::LThumbX, pad.sThumbLX / 32768.0f);
                m_app->OnEvent(ev);
            }

            if (xstate.Gamepad.sThumbLY != pad.sThumbLY)
            {
                pad.sThumbLY = xstate.Gamepad.sThumbLY;

                GamepadAxisEvent ev(i, GamepadAxis::LThumbY, pad.sThumbLY / 32768.0f);
                m_app->OnEvent(ev);
            }

            if (xstate.Gamepad.sThumbRX != pad.sThumbRX)
            {
                pad.sThumbRX = xstate.Gamepad.sThumbRX;

                GamepadAxisEvent ev(i, GamepadAxis::RThumbX, pad.sThumbRX / 32768.0f);
                m_app->OnEvent(ev);
            }

            if (xstate.Gamepad.sThumbRY != pad.sThumbRY)
            {
                pad.sThumbRY = xstate.Gamepad.sThumbRY;

                GamepadAxisEvent ev(i, GamepadAxis::RThumbY, pad.sThumbRY / 32768.0f);
                m_app->OnEvent(ev);
            }

            if (xstate.Gamepad.bLeftTrigger != pad.bLeftTrigger)
            {
                pad.bLeftTrigger = xstate.Gamepad.bLeftTrigger;

                GamepadAxisEvent ev(i, GamepadAxis::LTrigger, pad.bLeftTrigger / 255.0f);
                m_app->OnEvent(ev);
            }

            if (xstate.Gamepad.bRightTrigger != pad.bRightTrigger)
            {
                pad.bRightTrigger = xstate.Gamepad.bRightTrigger;

                GamepadAxisEvent ev(i, GamepadAxis::RTrigger, pad.bRightTrigger / 255.0f);
                m_app->OnEvent(ev);
            }
        }

        m_refreshGamepadConnectivity = false;
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
            ::SetCursor(LoadCursorW(nullptr, cursorNames[static_cast<int32_t>(m_cursor)]));
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
        return allocator.New<Win32::DeviceImpl>(allocator);
    }
}

#endif
