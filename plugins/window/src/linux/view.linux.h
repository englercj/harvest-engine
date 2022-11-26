// Copyright Chad Engler

#pragma once

#include "he/window/event.h"
#include "he/window/view.h"

#if defined(HE_PLATFORM_LINUX)

#include "x11_all.linux.h"

namespace he::window
{
    struct Event;
}

namespace he::window::linux
{
    class DeviceImpl;

    class ViewImpl : public View
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

        void CaptureMouse();
        void ReleaseMouse();

        void CaptureTouch();
        void ReleaseTouch();

    public:
        DeviceImpl* m_device{ nullptr };
        void* m_userData{ nullptr };
        XIC m_ic{ nullptr };
        ViewFlag m_flags{ ViewFlag::Default };

        Window m_window{ X11_None };
        int m_mouseCaptureCount{ 0 };
        int m_touchCaptureCount{ 0 };
        int m_primaryTouchId{ 0 };
        Vec2i m_pos{ 0, 0 };
        Vec2i m_size{ 1, 1 };

        bool m_maximized{ false };
    };
}

#endif
