// Copyright Chad Engler

#pragma once

#include "gamepad.win32.h"

#include "he/core/types.h"
#include "he/window/device.h"
#include "he/window/event.h"
#include "he/window/pointer.h"

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
    using Pfn_ChangeWindowMessageFilterEx = BOOL(WINAPI*)(_In_ HWND hwnd, _In_ UINT message, _In_ DWORD action, _Inout_ PCHANGEFILTERSTRUCT pChangeFilterStruct);
    using Pfn_RegisterTouchWindow = BOOL(WINAPI*)(_In_ HWND hwnd, _In_ ULONG ulFlags);
    using Pfn_GetTouchInputInfo = BOOL(WINAPI*)(_In_ HTOUCHINPUT hTouchInput, _In_ UINT cInputs, _Out_writes_(cInputs) PTOUCHINPUT pInputs, _In_ int cbSize);
    using Pfn_CloseTouchInputHandle = BOOL(WINAPI*)(_In_ HTOUCHINPUT hTouchInput);
    using Pfn_GetPointerType = BOOL(WINAPI*)(_In_ UINT32 pointerId, _Out_ POINTER_INPUT_TYPE *pointerType);
    using Pfn_GetPointerFrameInfo = BOOL(WINAPI*)(_In_ UINT32 pointerId, _Inout_ UINT32 *pointerCount, _Out_writes_opt_(*pointerCount) POINTER_INFO *pointerInfo);
    using Pfn_GetPointerFrameInfoHistory = BOOL(WINAPI*)(_In_ UINT32 pointerId, _Inout_ UINT32 *entriesCount, _Inout_ UINT32 *pointerCount, _Out_writes_opt_(*entriesCount * *pointerCount) POINTER_INFO *pointerInfo);
    using Pfn_GetPointerFramePenInfo = BOOL(WINAPI*)(_In_ UINT32 pointerId, _Inout_ UINT32 *pointerCount, _Out_writes_opt_(*pointerCount) POINTER_PEN_INFO *penInfo);
    using Pfn_GetPointerFramePenInfoHistory = BOOL(WINAPI*)(_In_ UINT32 pointerId, _Inout_ UINT32 *entriesCount, _Inout_ UINT32 *pointerCount, _Out_writes_opt_(*entriesCount * *pointerCount) POINTER_PEN_INFO *penInfo);
    using Pfn_GetPointerFrameTouchInfo = BOOL(WINAPI*)(_In_ UINT32 pointerId, _Inout_ UINT32 *pointerCount, _Out_writes_opt_(*pointerCount) POINTER_TOUCH_INFO *touchInfo);
    using Pfn_GetPointerFrameTouchInfoHistory = BOOL(WINAPI*)(_In_ UINT32 pointerId, _Inout_ UINT32 *entriesCount, _Inout_ UINT32 *pointerCount, _Out_writes_opt_(*entriesCount * *pointerCount) POINTER_TOUCH_INFO *touchInfo);
    using Pfn_SkipPointerFrameMessages = BOOL(WINAPI*)(_In_ UINT32 pointerId);

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
        void SyncCursor() const;
        void ShowCursor(bool show);
        void CenterCursor();

        void DispatchPointerEvent(const PointerEvent& data, const POINTER_INFO& info, PEN_FLAGS penFlags);
        void DispatchPointerEventMouse(View* view, const POINTER_INFO& info);
        void DispatchPointerEventPen(View* view, const POINTER_PEN_INFO& info);
        void DispatchPointerEventTouch(View* view, const POINTER_TOUCH_INFO& info);

        bool HandlePointerFrameMouse(View* view, PointerId pointerId);
        bool HandlePointerFrameHistoryMouse(View* view, PointerId pointerId);

        bool HandlePointerFramePen(View* view, PointerId pointerId);
        bool HandlePointerFrameHistoryPen(View* view, PointerId pointerId);

        bool HandlePointerFrameTouch(View* view, PointerId pointerId);
        bool HandlePointerFrameHistoryTouch(View* view, PointerId pointerId);

    public:
        Application* m_app{ nullptr };
        HINSTANCE m_hInstance{ nullptr };
        ViewImpl* m_cursorRelativeView{ nullptr };
        PointerCursor m_cursor{ PointerCursor::Arrow };
        std::atomic<int32_t> m_returnCode{ 0 };
        std::atomic<bool> m_running{ true };
        bool m_cursorVisible{ true };
        bool m_viewClipped{ false };
        Vec2f m_cursorRestorePosition{ 0, 0 };
        DeviceInfo m_deviceInfo{};

        HMODULE m_userLib{ nullptr };
        Pfn_GetDpiForWindow m_GetDpiForWindow{ nullptr };
        Pfn_SetProcessDpiAwarenessContext m_SetProcessDpiAwarenessContext{ nullptr };
        Pfn_AdjustWindowRectExForDpi m_AdjustWindowRectExForDpi{ nullptr };
        Pfn_ChangeWindowMessageFilterEx m_ChangeWindowMessageFilterEx{ nullptr };
        Pfn_RegisterTouchWindow m_RegisterTouchWindow{ nullptr };
        Pfn_GetTouchInputInfo m_GetTouchInputInfo{ nullptr };
        Pfn_CloseTouchInputHandle m_CloseTouchInputHandle{ nullptr };
        Pfn_GetPointerType m_GetPointerType{ nullptr };
        Pfn_GetPointerFrameInfo m_GetPointerFrameInfo{ nullptr };
        Pfn_GetPointerFrameInfoHistory m_GetPointerFrameInfoHistory{ nullptr };
        Pfn_GetPointerFramePenInfo m_GetPointerFramePenInfo{ nullptr };
        Pfn_GetPointerFramePenInfoHistory m_GetPointerFramePenInfoHistory{ nullptr };
        Pfn_GetPointerFrameTouchInfo m_GetPointerFrameTouchInfo{ nullptr };
        Pfn_GetPointerFrameTouchInfoHistory m_GetPointerFrameTouchInfoHistory{ nullptr };
        Pfn_SkipPointerFrameMessages m_SkipPointerFrameMessages{ nullptr };

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
