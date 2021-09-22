// Copyright Chad Engler

#include "imgui_platform_service.h"

#include "he/core/alloca.h"
#include "he/core/assert.h"
#include "he/core/enum_ops.h"
#include "he/core/memory_ops.h"

#include "imgui_internal.h"

namespace he::editor
{
    constexpr window::MouseCursor ImGuiMouseCursorMap[]
    {
        window::MouseCursor::Arrow,             // ImGuiMouseCursor_Arrow
        window::MouseCursor::TextInput,         // ImGuiMouseCursor_TextInput
        window::MouseCursor::ResizeAll,         // ImGuiMouseCursor_ResizeAll
        window::MouseCursor::ResizeVertical,    // ImGuiMouseCursor_ResizeNS
        window::MouseCursor::ResizeHorizontal,  // ImGuiMouseCursor_ResizeEW
        window::MouseCursor::ResizeTopRight,    // ImGuiMouseCursor_ResizeNESW
        window::MouseCursor::ResizeTopLeft,     // ImGuiMouseCursor_ResizeNWSE
        window::MouseCursor::Hand,              // ImGuiMouseCursor_Hand
        window::MouseCursor::NotAllowed,        // ImGuiMouseCursor_NotAllowed
    };

    ImGuiPlatformService::ImGuiPlatformService()
    {}

    bool ImGuiPlatformService::Initialize(
        window::Device* device,
        window::View* view,
        SetupStyleFunc setupStyle,
        SetupFontAtlasFunc setupFontAtlas,
        void* setupCtx)
    {
        m_device = device;
        m_view = view;
        m_setupStyle = setupStyle;
        m_setupFontAtlas = setupFontAtlas;
        m_setupCtx = setupCtx;
        m_time = MonotonicClock::Now();

        ImGuiIO& io = ImGui::GetIO();
        m_originalFontAtlas = io.Fonts;

        // Setup backend capabilities flags
        HE_ASSERT(io.BackendPlatformUserData == nullptr);

        io.BackendPlatformName = "imgui_impl_harvest";
        io.BackendPlatformUserData = this;
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
        io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;    // We can create multi-viewports on the Platform side (optional)
        io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can set io.MouseHoveredViewport correctly (optional, not easy)
        io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad; // TODO: Gamepad navigation support

        ImGuiViewport* mainViewport = ImGui::GetMainViewport();
        mainViewport->PlatformHandle = m_view;
        mainViewport->PlatformHandleRaw = m_view->GetNativeHandle();
        mainViewport->PlatformUserData = nullptr;

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            UpdateMonitors();

            // Register platform interface (will be coupled with a renderer interface)
            ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
            platform_io.Platform_CreateWindow = &ImGuiPlatformService::CreateWindow;
            platform_io.Platform_DestroyWindow = &ImGuiPlatformService::DestroyWindow;
            platform_io.Platform_ShowWindow = &ImGuiPlatformService::ShowWindow;
            platform_io.Platform_SetWindowPos = &ImGuiPlatformService::SetWindowPos;
            platform_io.Platform_GetWindowPos = &ImGuiPlatformService::GetWindowPos;
            platform_io.Platform_SetWindowSize = &ImGuiPlatformService::SetWindowSize;
            platform_io.Platform_GetWindowSize = &ImGuiPlatformService::GetWindowSize;
            platform_io.Platform_SetWindowFocus = &ImGuiPlatformService::SetWindowFocus;
            platform_io.Platform_GetWindowFocus = &ImGuiPlatformService::GetWindowFocus;
            platform_io.Platform_GetWindowMinimized = &ImGuiPlatformService::GetWindowMinimized;
            platform_io.Platform_SetWindowTitle = &ImGuiPlatformService::SetWindowTitle;
            platform_io.Platform_SetWindowAlpha = &ImGuiPlatformService::SetWindowAlpha;
            platform_io.Platform_GetWindowDpiScale = &ImGuiPlatformService::GetWindowDpiScale;
            platform_io.Platform_OnChangedViewport = &ImGuiPlatformService::OnChangedViewport;
        }

        // Keyboard mapping. Dear ImGui will use those indices to peek into the io.KeysDown[] array that we will update during the application lifetime.
        io.KeyMap[ImGuiKey_Tab] = static_cast<int32_t>(window::Key::Tab);
        io.KeyMap[ImGuiKey_LeftArrow] = static_cast<int32_t>(window::Key::Left);
        io.KeyMap[ImGuiKey_RightArrow] = static_cast<int32_t>(window::Key::Right);
        io.KeyMap[ImGuiKey_UpArrow] = static_cast<int32_t>(window::Key::Up);
        io.KeyMap[ImGuiKey_DownArrow] = static_cast<int32_t>(window::Key::Down);
        io.KeyMap[ImGuiKey_PageUp] = static_cast<int32_t>(window::Key::PageUp);
        io.KeyMap[ImGuiKey_PageDown] = static_cast<int32_t>(window::Key::PageDown);
        io.KeyMap[ImGuiKey_Home] = static_cast<int32_t>(window::Key::Home);
        io.KeyMap[ImGuiKey_End] = static_cast<int32_t>(window::Key::End);
        io.KeyMap[ImGuiKey_Insert] = static_cast<int32_t>(window::Key::Insert);
        io.KeyMap[ImGuiKey_Delete] = static_cast<int32_t>(window::Key::Delete);
        io.KeyMap[ImGuiKey_Backspace] = static_cast<int32_t>(window::Key::Backspace);
        io.KeyMap[ImGuiKey_Space] = static_cast<int32_t>(window::Key::Space);
        io.KeyMap[ImGuiKey_Enter] = static_cast<int32_t>(window::Key::Enter);
        io.KeyMap[ImGuiKey_Escape] = static_cast<int32_t>(window::Key::Escape);
        io.KeyMap[ImGuiKey_KeyPadEnter] = static_cast<int32_t>(window::Key::Enter);
        io.KeyMap[ImGuiKey_A] = static_cast<int32_t>(window::Key::A);
        io.KeyMap[ImGuiKey_C] = static_cast<int32_t>(window::Key::C);
        io.KeyMap[ImGuiKey_V] = static_cast<int32_t>(window::Key::V);
        io.KeyMap[ImGuiKey_X] = static_cast<int32_t>(window::Key::X);
        io.KeyMap[ImGuiKey_Y] = static_cast<int32_t>(window::Key::Y);
        io.KeyMap[ImGuiKey_Z] = static_cast<int32_t>(window::Key::Z);

        UpdateDpiResources(view->GetDpiScale());

        return true;
    }

    void ImGuiPlatformService::Terminate()
    {
        // Restore the original atlas so ImGui can delete it safely
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts = m_originalFontAtlas;
    }

    void ImGuiPlatformService::NewFrame()
    {
        ImGuiIO& io = ImGui::GetIO();
        IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer backend. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

        Vec2i viewSize = m_view->GetSize();
        io.DisplaySize.x = static_cast<float>(viewSize.x);
        io.DisplaySize.y = static_cast<float>(viewSize.y);

        if (m_needToUpdateMonitors)
            UpdateMonitors();

        // Setup time step
        MonotonicTime now = MonotonicClock::Now();
        Duration delta = now - m_time;
        m_time = now;

        io.DeltaTime = ToFloatPeriod<Seconds>(delta);

        // Update OS mouse position
        UpdateMousePos();

        // Update OS mouse cursor with the cursor requested by imgui
        ImGuiMouseCursor cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
        if (m_lastCursor != cursor)
        {
            m_lastCursor = cursor;
            UpdateMouseCursor();
        }
    }

    void ImGuiPlatformService::OnEvent(const window::Event& ev)
    {
        switch (ev.type)
        {
            case window::EventType::DisplayChanged:
            {
                m_needToUpdateMonitors = true;
                break;
            }
            case window::EventType::ViewActivated:
            {
                const auto& evt = static_cast<const window::ViewActivatedEvent&>(ev);
                if (!evt.active && evt.view == m_view)
                {
                    ImGuiIO& io = ImGui::GetIO();
                    MemZero(io.KeysDown, sizeof(io.KeysDown));
                }
                break;
            }
            case window::EventType::ViewRequestClose:
            {
                const auto& evt = static_cast<const window::ViewRequestCloseEvent&>(ev);
                ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(evt.view);
                if (viewport)
                    viewport->PlatformRequestClose = true;
                break;
            }
            case window::EventType::ViewMoved:
            {
                const auto& evt = static_cast<const window::ViewRequestCloseEvent&>(ev);
                ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(evt.view);
                if (viewport)
                    viewport->PlatformRequestMove = true;
                break;
            }
            case window::EventType::ViewResized:
            {
                const auto& evt = static_cast<const window::ViewResizedEvent&>(ev);
                ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(evt.view);
                if (viewport)
                    viewport->PlatformRequestResize = true;
                break;
            }
            case window::EventType::MouseDown:
            case window::EventType::MouseUp:
            {
                const auto& evt = static_cast<const window::MouseButtonEvent&>(ev);
                const bool down = evt.type == window::EventType::MouseDown;
                ImGuiIO& io = ImGui::GetIO();
                io.MouseDown[static_cast<uint32_t>(evt.button)] = down;
                break;
            }
            case window::EventType::MouseWheel:
            {
                const auto& evt = static_cast<const window::MouseWheelEvent&>(ev);
                ImGuiIO& io = ImGui::GetIO();
                io.MouseWheel += evt.delta.y;
                io.MouseWheelH += evt.delta.x;
                break;
            }
            case window::EventType::Text:
            {
                const auto& evt = static_cast<const window::TextEvent&>(ev);
                ImGuiIO& io = ImGui::GetIO();
                io.AddInputCharacter(evt.ch);
                break;
            }
            case window::EventType::KeyDown:
            case window::EventType::KeyUp:
            {
                const auto& evt = static_cast<const window::KeyEvent&>(ev);
                const bool down = evt.type == window::EventType::KeyDown;
                ImGuiIO& io = ImGui::GetIO();
                io.KeysDown[static_cast<uint32_t>(evt.key)] = down;

                switch (evt.key)
                {
                    case window::Key::Control: io.KeyCtrl = down; break;
                    case window::Key::Shift: io.KeyShift = down; break;
                    case window::Key::Alt: io.KeyAlt = down; break;
                    case window::Key::Super: io.KeySuper = down; break;
                    default: break;
                }
                break;
            }
        }
    }

    void ImGuiPlatformService::UpdateMonitors()
    {
        m_needToUpdateMonitors = false;

        uint32_t count = m_device->GetMonitorCount();
        if (count == 0)
            return;

        window::Monitor* monitors = HE_ALLOCA(window::Monitor, count);

        m_device->GetMonitors(monitors, count);

        ImGuiPlatformIO& io = ImGui::GetPlatformIO();
        io.Monitors.resize(0);

        for (uint32_t i = 0; i < count; ++i)
        {
            window::Monitor& monitor = monitors[i];

            ImGuiPlatformMonitor m;
            m.MainPos = { static_cast<float>(monitor.pos.x), static_cast<float>(monitor.pos.y) };
            m.MainSize = { static_cast<float>(monitor.size.x), static_cast<float>(monitor.size.y) };
            m.WorkPos = { static_cast<float>(monitor.workPos.x), static_cast<float>(monitor.workPos.y) };
            m.WorkSize = { static_cast<float>(monitor.workSize.x), static_cast<float>(monitor.workSize.y) };
            m.DpiScale = monitor.dpiScale;

            UpdateDpiResources(m.DpiScale);

            if (monitor.primary)
                io.Monitors.push_front(m);
            else
                io.Monitors.push_back(m);
        }
    }

    void ImGuiPlatformService::UpdateMousePos()
    {
        ImGuiIO& io = ImGui::GetIO();
        HE_ASSERT(m_view != nullptr);

        const bool viewportsEnabled = (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0;

        // Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
        // (When multi-viewports are enabled, all imgui positions are same as OS positions)
        if (io.WantSetMousePos)
        {
            window::View* relativeView = viewportsEnabled ? nullptr : m_view;
            m_device->SetCursorPos(relativeView, { io.MousePos.x, io.MousePos.y });
        }

        io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
        io.MouseHoveredViewport = 0;

        // Set imgui mouse position
        Vec2f screenPos = m_device->GetCursorPos(nullptr);
        window::View* focusedView = m_device->GetFocusedView();

        if (focusedView)
        {
            // if (::IsChild(focusedView, m_view))
            //     focusedView = m_view;

            // Multi-viewport mode: mouse position in OS absolute coordinates
            // (io.MousePos is (0,0) when the mouse is on the upper-left of the primary monitor)
            // This is the position you can get with GetCursorPos(). In theory adding viewport->Pos
            // is also the reverse operation of doing ScreenToClient().
            if (viewportsEnabled)
            {
                if (ImGui::FindViewportByPlatformHandle(focusedView) != nullptr)
                    io.MousePos = { screenPos.x, screenPos.y };
            }
            // Single viewport mode: mouse position in client window coordinates
            // (io.MousePos is (0,0) when the mouse is on the upper-left corner of the app window.)
            // This is the position you can get with GetCursorPos() + ScreenToClient() or
            // from WM_MOUSEMOVE.
            else if (focusedView == m_view)
            {
                Vec2f clientPos = focusedView->ScreenToView(screenPos);
                io.MousePos = { clientPos.x, clientPos.y };
            }
        }

        // Set hovered window
        window::View* hoveredView = m_device->GetHoveredView();
        if (hoveredView)
        {
            ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(hoveredView);
            if (viewport)
            {
                if ((viewport->Flags & ImGuiViewportFlags_NoInputs) == 0)
                {
                    io.MouseHoveredViewport = viewport->ID;
                }
            }
        }
    }

    void ImGuiPlatformService::UpdateMouseCursor()
    {
        ImGuiIO& io = ImGui::GetIO();

        if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) != 0)
            return;

        ImGuiMouseCursor cursor = ImGui::GetMouseCursor();

        if (cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
            m_device->SetCursor(window::MouseCursor::None);
        else
            m_device->SetCursor(ImGuiMouseCursorMap[cursor]);
    }

    void ImGuiPlatformService::UpdateDpiResources(float dpiScale)
    {
        ImGuiContext& g = *GImGui;

        // Set the style to the scaled value
        m_setupStyle(m_setupCtx, g.Style);
        g.Style.ScaleAllSizes(dpiScale);

        // Get or create the font atlas for this DPI scale
        std::unique_ptr<ImFontAtlas>& atlas = m_dpiFontAtlas[dpiScale];
        if (!atlas)
        {
            atlas = std::make_unique<ImFontAtlas>();
            m_setupFontAtlas(m_setupCtx, *atlas, dpiScale);
        }

        // Find indices of active fonts so we can switch them to the new atlas
        int32_t currentFontIndex = -1;
        int32_t defaultFontIndex = -1;
        for (int32_t i = 0; i < g.IO.Fonts->Fonts.size(); ++i)
        {
            if (g.Font == g.IO.Fonts->Fonts[i])
                currentFontIndex = i;
            if (g.IO.FontDefault == g.IO.Fonts->Fonts[i])
                defaultFontIndex = i;
        }

        // Cache off the lock state of the atlas, and unlock the old one.
        const bool locked = g.IO.Fonts->Locked;
        g.IO.Fonts->Locked = false;

        // Swap ImGui's state to the new font atlas
        g.IO.Fonts = atlas.get();
        g.IO.Fonts->Locked = locked;

        // Set the current font information based on our found indices
        if (currentFontIndex != -1)
        {
            ImGui::SetCurrentFont(g.IO.Fonts->Fonts[currentFontIndex]);
        }

        if (defaultFontIndex != -1)
        {
            g.IO.FontDefault = g.IO.Fonts->Fonts[defaultFontIndex];
        }
    }

    void ImGuiPlatformService::CreateWindow(ImGuiViewport* vp)
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGuiPlatformService* service = static_cast<ImGuiPlatformService*>(io.BackendPlatformUserData);

        window::ViewDesc desc{};
        desc.title = "Untitled";
        desc.size = { static_cast<int32_t>(vp->Size.x), static_cast<int32_t>(vp->Size.y) };
        desc.pos = { static_cast<int32_t>(vp->Pos.x), static_cast<int32_t>(vp->Pos.y) };
        desc.flags = window::ViewFlag::None;

        if ((vp->Flags & ImGuiViewportFlags_NoDecoration) != 0)
            desc.flags |= window::ViewFlag::Borderless;
        if ((vp->Flags & ImGuiViewportFlags_NoTaskBarIcon) == 0)
            desc.flags |= window::ViewFlag::TaskBarIcon;
        if ((vp->Flags & ImGuiViewportFlags_NoFocusOnClick) == 0)
            desc.flags |= window::ViewFlag::FocusOnClick;
        if ((vp->Flags & ImGuiViewportFlags_NoInputs) == 0)
            desc.flags |= window::ViewFlag::AcceptInput;
        if ((vp->Flags & ImGuiViewportFlags_TopMost) != 0)
            desc.flags |= window::ViewFlag::TopMost;

        if (vp->ParentViewportId)
        {
            ImGuiViewport* parent = ImGui::FindViewportByID(vp->ParentViewportId);
            if (parent)
            {
                desc.parent = static_cast<window::View*>(parent->PlatformHandle);
            }
        }

        window::View* view = service->m_device->CreateView(desc);

        vp->PlatformRequestResize = false;
        vp->PlatformHandle = view;
        vp->PlatformHandleRaw = view->GetNativeHandle();
        vp->PlatformUserData = reinterpret_cast<void*>(uintptr_t{ 1 }); // mark this as owning the view
    }

    void ImGuiPlatformService::DestroyWindow(ImGuiViewport* vp)
    {
        // Main viewport has nullptr set, everyone else has unintptr_t{ 1 }
        if (vp->PlatformUserData == nullptr)
            return;

        ImGuiIO& io = ImGui::GetIO();
        ImGuiPlatformService* service = static_cast<ImGuiPlatformService*>(io.BackendPlatformUserData);

        window::View* view = static_cast<window::View*>(vp->PlatformHandle);
        service->m_device->DestroyView(view);

        vp->PlatformHandle = nullptr;
        vp->PlatformHandleRaw = nullptr;
        vp->PlatformUserData = nullptr;
    }

    void ImGuiPlatformService::ShowWindow(ImGuiViewport* vp)
    {
        window::View* view = static_cast<window::View*>(vp->PlatformHandle);

        const bool focus = (vp->Flags & ImGuiViewportFlags_NoFocusOnAppearing) == 0;
        view->SetVisible(true, focus);
    }

    void ImGuiPlatformService::SetWindowPos(ImGuiViewport* vp, ImVec2 pos)
    {
        window::View* view = static_cast<window::View*>(vp->PlatformHandle);
        view->SetPosition({ static_cast<int32_t>(pos.x), static_cast<int32_t>(pos.y) });
    }

    ImVec2 ImGuiPlatformService::GetWindowPos(ImGuiViewport* vp)
    {
        window::View* view = static_cast<window::View*>(vp->PlatformHandle);
        Vec2i pos = view->GetPosition();
        return { static_cast<float>(pos.x), static_cast<float>(pos.y) };
    }

    void ImGuiPlatformService::SetWindowSize(ImGuiViewport* vp, ImVec2 size)
    {
        window::View* view = static_cast<window::View*>(vp->PlatformHandle);
        view->SetSize({ static_cast<int32_t>(size.x), static_cast<int32_t>(size.y) });
    }

    ImVec2 ImGuiPlatformService::GetWindowSize(ImGuiViewport* vp)
    {
        window::View* view = static_cast<window::View*>(vp->PlatformHandle);
        Vec2i size = view->GetSize();
        return { static_cast<float>(size.x), static_cast<float>(size.y) };
    }

    void ImGuiPlatformService::SetWindowFocus(ImGuiViewport* vp)
    {
        window::View* view = static_cast<window::View*>(vp->PlatformHandle);
        view->Focus();
    }

    bool ImGuiPlatformService::GetWindowFocus(ImGuiViewport* vp)
    {
        window::View* view = static_cast<window::View*>(vp->PlatformHandle);
        return view->IsFocused();
    }

    bool ImGuiPlatformService::GetWindowMinimized(ImGuiViewport* vp)
    {
        window::View* view = static_cast<window::View*>(vp->PlatformHandle);
        return view->IsMinimized();
    }

    void ImGuiPlatformService::SetWindowTitle(ImGuiViewport* vp, const char* str)
    {
        window::View* view = static_cast<window::View*>(vp->PlatformHandle);
        view->SetTitle(str);
    }

    void ImGuiPlatformService::SetWindowAlpha(ImGuiViewport* vp, float alpha)
    {
        window::View* view = static_cast<window::View*>(vp->PlatformHandle);
        view->SetAlpha(alpha);
    }

    float ImGuiPlatformService::GetWindowDpiScale(ImGuiViewport* vp)
    {
        window::View* view = static_cast<window::View*>(vp->PlatformHandle);
        return view->GetDpiScale();
    }

    void ImGuiPlatformService::OnChangedViewport(ImGuiViewport* vp)
    {
        // This happens sometimes, and I'm not 100% sure why. Skip them for now.
        if (vp == nullptr || ImGui::GetWindowDpiScale() == 0.0f)
            return;

        ImGuiIO& io = ImGui::GetIO();
        ImGuiPlatformService* service = static_cast<ImGuiPlatformService*>(io.BackendPlatformUserData);

        service->UpdateDpiResources(ImGui::GetWindowDpiScale());
    }
}
