// Copyright Chad Engler

#pragma once

#include "gamepad.win32.h"

#include "he/core/types.h"
#include "he/window/device.h"

#include <atomic>

#if defined(HE_PLATFORM_API_WIN32)

#include <ShellScalingApi.h>
#include <Windows.h>
#include <Xinput.h>

namespace he::window
{
    class Application;
    class Gamepad;
}

namespace he::window::win32
{
    using Pfn_GetDpiForWindow = UINT(WINAPI*)(_In_ HWND hwnd);
    using Pfn_SetProcessDpiAwarenessContext = DPI_AWARENESS_CONTEXT(WINAPI*)(_In_ DPI_AWARENESS_CONTEXT dpiContext);
    using Pfn_AdjustWindowRectExForDpi = BOOL(WINAPI*)(_Inout_ LPRECT lpRect, _In_ DWORD dwStyle, _In_ BOOL bMenu, _In_ DWORD dwExStyle, _In_ UINT dpi);

    using Pfn_GetDpiForMonitor = HRESULT(WINAPI*)(_In_ HMONITOR hmonitor, _In_ MONITOR_DPI_TYPE dpiType, _Out_ UINT* dpiX, _Out_ UINT* dpiY);

    using Pfn_XInputGetState = DWORD(WINAPI*)(_In_ DWORD dwUserIndex, _Out_ XINPUT_STATE* pState);
    using Pfn_XInputSetState = DWORD(WINAPI*)(_In_ DWORD dwUserIndex, _In_ XINPUT_VIBRATION* pVibration);
    using Pfn_XInputEnable = VOID(WINAPI*)(_In_ BOOL enable);

    class DeviceImpl final : public Device
    {
    public:
        DeviceImpl(Allocator& allocator) noexcept;
        ~DeviceImpl() noexcept;

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

        Gamepad& GetGamepad(uint32_t index) override;

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
        Pfn_XInputSetState m_XInputSetState{ nullptr };
        Pfn_XInputEnable m_XInputEnable{ nullptr };

        static_assert(MaxGamepads <= XUSER_MAX_COUNT, "Cannot handle more than XUSER_MAX_COUNT gamepads.");

        GamepadImpl m_gamepads[MaxGamepads];
        bool m_refreshGamepadConnectivity{ true };
    };
}

#endif
