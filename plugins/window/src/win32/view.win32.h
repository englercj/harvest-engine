// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/window/event.h"
#include "he/window/view.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "drop_target.win32.h"

#include <Windows.h>

namespace he::window
{
    struct Event;
}

namespace he::window::win32
{
    class DeviceImpl;

    class ViewImpl final : public View
    {
    public:
        ViewImpl(DeviceImpl* device, const ViewDesc& desc) noexcept;
        ~ViewImpl() noexcept;

        void* GetNativeHandle() const override;
        void* GetUserData() const override;
        Vec2i GetPosition() const override;
        Vec2i GetSize() const override;
        float GetDpiScale() const override;
        bool IsFocused() const override;
        bool IsChildFocused() const override;
        bool IsMinimized() const override;
        bool IsMaximized() const override;

        void SetPosition(const Vec2i& pos) override;
        void SetSize(const Vec2i& size) override;
        void SetVisible(bool visible, bool focus) override;
        void SetTitle(const char* text) override;
        void SetAlpha(float alpha) override;
        void SetAcceptInput(bool value) override;

        void Focus() override;
        void Minimize() override;
        void Maximize() override;
        void Restore() override;
        void RequestClose() override;

        Vec2f ViewToScreen(const Vec2f& pos) const override;
        Vec2f ScreenToView(const Vec2f& pos) const override;

        void TrackCapture(const PointerEvent& ev);

    public:
        DeviceImpl* m_device{ nullptr };
        HWND m_window{ nullptr };
        ViewFlag m_flags{ ViewFlag::Default };
        void* m_userData{ nullptr };
        int32_t m_captureCount{ 0 };
        Vec2i m_pos{ 0, 0 };
        Vec2i m_size{ 1, 1 };
        uint32_t m_dpi{ USER_DEFAULT_SCREEN_DPI };
        bool m_hackNeedsFrameUpdate{ true };
        DropTarget m_dropTarget{ this };
    };
}

#endif
