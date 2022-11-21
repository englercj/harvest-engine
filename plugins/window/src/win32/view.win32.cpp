// Copyright Chad Engler

#include "view.win32.h"

#include "common.win32.h"
#include "device.win32.h"

#include "he/core/assert.h"
#include "he/core/enum_ops.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/wstr.h"
#include "he/window/event.h"

#if defined(HE_PLATFORM_API_WIN32)

#include <Windows.h>

#ifndef WM_COPYGLOBALDATA
    #define WM_COPYGLOBALDATA 0x0049
#endif

namespace he::window::win32
{
    ViewImpl::ViewImpl(DeviceImpl* device, const ViewDesc& desc) noexcept
        : m_device(device)
        , m_userData(desc.userData)
        , m_flags(desc.flags)
    {
        const char* title = desc.title ? desc.title : "Harvest Window";
        wchar_t* wtitle = HE_TO_WSTR(title);

        // Create window
        DWORD dwStyle = WS_SYSMENU;
        DWORD dwExStyle = 0;

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
            _heWindowClassName,
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

        HE_ASSERT(m_window != 0, HE_MSG("CreateWindowEx failed."), HE_KV(error, Result::FromLastError()));

        // Inform Windows that it should redraw our frame styles
        ::SetWindowPos(m_window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

        // Cache the DPI of the window now that it is created
        if (device->m_GetDpiForWindow)
            m_dpi = device->m_GetDpiForWindow(m_window);

        // Ensure we get drop file messages despite our elevation level
        if (HasFlag(m_flags, ViewFlag::AcceptFiles))
        {
            if (device->m_ChangeWindowMessageFilterEx)
            {
                device->m_ChangeWindowMessageFilterEx(m_window, WM_DROPFILES, MSGFLT_ALLOW, NULL);
                device->m_ChangeWindowMessageFilterEx(m_window, WM_COPYDATA, MSGFLT_ALLOW, NULL);
                device->m_ChangeWindowMessageFilterEx(m_window, WM_COPYGLOBALDATA, MSGFLT_ALLOW, NULL);
            }
            ::DragAcceptFiles(m_window, TRUE);
        }
    }

    ViewImpl::~ViewImpl() noexcept
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

    bool ViewImpl::IsChildFocused() const
    {
        HWND hWnd = ::GetForegroundWindow();
        return hWnd && (hWnd == m_window || ::IsChild(hWnd, m_window));
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

    void ViewImpl::SetAcceptInput(bool value)
    {
        if (value)
            m_flags |= ViewFlag::AcceptInput;
        else
            m_flags &= ~ViewFlag::AcceptInput;
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
}

#endif
