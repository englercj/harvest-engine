// Copyright Chad Engler

#include "imgui_platform_service.h"

#include "he/core/alloca.h"
#include "he/core/assert.h"
#include "he/core/enum_ops.h"
#include "he/core/memory_ops.h"
#include "he/math/vec2.h"

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

    constexpr ImGuiMouseButton ImGuiMouseButtonMap[]
    {
        ImGuiMouseButton_Left,      // Left
        ImGuiMouseButton_Right,     // Right
        ImGuiMouseButton_Middle,    // Middle
        3,                          // Extra1
        4,                          // Extra2
    };

    constexpr ImGuiKey ImGuiKeyMap[]
    {
        ImGuiKey_None,              // None
        ImGuiKey_Backspace,         // Backspace
        ImGuiKey_Enter,             // Enter
        ImGuiKey_Escape,            // Escape
        ImGuiKey_Space,             // Space
        ImGuiKey_Tab,               // Tab
        ImGuiKey_Pause,             // Pause
        ImGuiKey_PrintScreen,       // PrintScreen
        ImGuiKey_KeypadDecimal,     // NumPad_Decimal
        ImGuiKey_KeypadMultiply,    // NumPad_Multiply
        ImGuiKey_KeypadAdd,         // NumPad_Add
        ImGuiKey_KeypadSubtract,    // NumPad_Subtract
        ImGuiKey_KeypadDivide,      // NumPad_Divide
        ImGuiKey_KeypadEnter,       // NumPad_Enter
        ImGuiKey_Keypad0,           // NumPad_0
        ImGuiKey_Keypad1,           // NumPad_1
        ImGuiKey_Keypad2,           // NumPad_2
        ImGuiKey_Keypad3,           // NumPad_3
        ImGuiKey_Keypad4,           // NumPad_4
        ImGuiKey_Keypad5,           // NumPad_5
        ImGuiKey_Keypad6,           // NumPad_6
        ImGuiKey_Keypad7,           // NumPad_7
        ImGuiKey_Keypad8,           // NumPad_8
        ImGuiKey_Keypad9,           // NumPad_9
        ImGuiKey_F1,                // F1
        ImGuiKey_F2,                // F2
        ImGuiKey_F3,                // F3
        ImGuiKey_F4,                // F4
        ImGuiKey_F5,                // F5
        ImGuiKey_F6,                // F6
        ImGuiKey_F7,                // F7
        ImGuiKey_F8,                // F8
        ImGuiKey_F9,                // F9
        ImGuiKey_F10,               // F10
        ImGuiKey_F11,               // F11
        ImGuiKey_F12,               // F12
        ImGuiKey_Home,              // Home
        ImGuiKey_LeftArrow,         // Left
        ImGuiKey_UpArrow,           // Up
        ImGuiKey_RightArrow,        // Right
        ImGuiKey_DownArrow,         // Down
        ImGuiKey_PageUp,            // PageUp
        ImGuiKey_PageDown,          // PageDown
        ImGuiKey_Insert,            // Insert
        ImGuiKey_Delete,            // Delete
        ImGuiKey_End,               // End
        ImGuiKey_ModAlt,            // Alt
        ImGuiKey_ModCtrl,           // Control
        ImGuiKey_ModShift,          // Shift
        ImGuiKey_ModSuper,          // Super
        ImGuiKey_ScrollLock,        // ScrollLock
        ImGuiKey_NumLock,           // NumLock
        ImGuiKey_CapsLock,          // CapsLock
        ImGuiKey_0,                 // Number_0
        ImGuiKey_1,                 // Number_1
        ImGuiKey_2,                 // Number_2
        ImGuiKey_3,                 // Number_3
        ImGuiKey_4,                 // Number_4
        ImGuiKey_5,                 // Number_5
        ImGuiKey_6,                 // Number_6
        ImGuiKey_7,                 // Number_7
        ImGuiKey_8,                 // Number_8
        ImGuiKey_9,                 // Number_9
        ImGuiKey_A,                 // A
        ImGuiKey_B,                 // B
        ImGuiKey_C,                 // C
        ImGuiKey_D,                 // D
        ImGuiKey_E,                 // E
        ImGuiKey_F,                 // F
        ImGuiKey_G,                 // G
        ImGuiKey_H,                 // H
        ImGuiKey_I,                 // I
        ImGuiKey_J,                 // J
        ImGuiKey_K,                 // K
        ImGuiKey_L,                 // L
        ImGuiKey_M,                 // M
        ImGuiKey_N,                 // N
        ImGuiKey_O,                 // O
        ImGuiKey_P,                 // P
        ImGuiKey_Q,                 // Q
        ImGuiKey_R,                 // R
        ImGuiKey_S,                 // S
        ImGuiKey_T,                 // T
        ImGuiKey_U,                 // U
        ImGuiKey_V,                 // V
        ImGuiKey_W,                 // W
        ImGuiKey_X,                 // X
        ImGuiKey_Y,                 // Y
        ImGuiKey_Z,                 // Z
        ImGuiKey_Semicolon,         // Semicolon
        ImGuiKey_Equal,             // Equals
        ImGuiKey_Comma,             // Comma
        ImGuiKey_Minus,             // Minus
        ImGuiKey_Period,            // Period
        ImGuiKey_Slash,             // Slash
        ImGuiKey_GraveAccent,       // Grave
        ImGuiKey_LeftBracket,       // LeftBracket
        ImGuiKey_Backslash,         // Backslash
        ImGuiKey_RightBracket,      // RightBracket
        ImGuiKey_Apostrophe,        // Apostrophe
    };

    bool ImGuiPlatformService::Initialize(
        window::Device* device,
        window::View* view,
        StyleSetupDelegate setupStyle,
        FontsSetupDelegate setupFonts)
    {
        m_device = device;
        m_view = view;
        m_setupStyle = setupStyle;
        m_setupFonts = setupFonts;
        m_time = MonotonicClock::Now();

        ImGuiIO& io = ImGui::GetIO();
        m_originalFontAtlas = io.Fonts;

        // Setup backend capabilities flags
        HE_ASSERT(io.BackendPlatformUserData == nullptr);

        io.BackendPlatformName = "imgui_impl_harvest";
        io.BackendPlatformUserData = this;
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;           // We can honor GetMouseCursor() values (optional)
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;            // We can honor io.WantSetMousePos requests (optional, rarely used)
        io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;      // We can create multi-viewports on the Platform side (optional)
        io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport;   // We can set io.MouseHoveredViewport correctly (optional, not easy)

        ImGuiViewport* mainViewport = ImGui::GetMainViewport();
        mainViewport->PlatformHandle = m_view;
        mainViewport->PlatformHandleRaw = m_view->GetNativeHandle();
        mainViewport->PlatformUserData = nullptr;

        if (HasFlag(io.ConfigFlags, ImGuiConfigFlags_ViewportsEnable))
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

        MonotonicTime now = MonotonicClock::Now();
        Duration delta = now - m_time;
        m_time = now;

        io.DeltaTime = ToPeriod<Seconds, float>(delta);

        UpdateMouseData();

        // Update OS mouse cursor with the cursor requested by imgui
        ImGuiMouseCursor cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
        if (m_lastCursor != cursor)
        {
            m_lastCursor = cursor;
            UpdateMouseCursor();
        }
    }

    void ImGuiPlatformService::UpdateViews()
    {
        const ImGuiContext& ctx = *ImGui::GetCurrentContext();
        for (int i = 0; i < ctx.Viewports.Size; ++i)
        {
            const ImGuiViewportP* viewport = ctx.Viewports[i];
            if (!viewport)
                continue;

            window::View* view = static_cast<window::View*>(viewport->PlatformHandle);
            if (!view)
                continue;

            const bool acceptInput = !HasFlag(viewport->Flags, ImGuiViewportFlags_NoInputs);
            view->SetAcceptInput(acceptInput);
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
                ImGuiIO& io = ImGui::GetIO();
                io.AddFocusEvent(evt.active && evt.view == m_view);
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
                const ImGuiMouseButton btn = ImGuiMouseButtonMap[static_cast<int32_t>(evt.button)];

                ImGuiIO& io = ImGui::GetIO();
                io.AddMouseButtonEvent(btn, down);
                break;
            }
            case window::EventType::MouseMove:
            {
                const auto& evt = static_cast<const window::MouseMoveEvent&>(ev);
                m_mouseInside = true;

                ImGuiIO& io = ImGui::GetIO();

                Vec2f pos = evt.pos;
                if (!evt.absolute)
                {
                    pos = m_device->GetCursorPos(nullptr);
                }

                if (HasFlag(io.ConfigFlags, ImGuiConfigFlags_ViewportsEnable))
                {
                    io.AddMousePosEvent(pos.x, pos.y);
                }
                else
                {
                    Vec2f viewPos = evt.view->ScreenToView(pos);
                    io.AddMousePosEvent(viewPos.x, viewPos.y);
                }
                break;
            }
            case window::EventType::MouseWheel:
            {
                const auto& evt = static_cast<const window::MouseWheelEvent&>(ev);
                ImGuiIO& io = ImGui::GetIO();
                io.AddMouseWheelEvent(evt.delta.x, evt.delta.y);
                break;
            }
            case window::EventType::Text:
            {
                const auto& evt = static_cast<const window::TextEvent&>(ev);
                ImGuiIO& io = ImGui::GetIO();
                io.AddInputCharacterUTF16(evt.ch);
                break;
            }
            case window::EventType::KeyDown:
            case window::EventType::KeyUp:
            {
                const auto& evt = static_cast<const window::KeyEvent&>(ev);
                const bool down = evt.type == window::EventType::KeyDown;
                const ImGuiKey key = ImGuiKeyMap[static_cast<uint32_t>(evt.key)];

                switch (evt.key)
                {
                    case window::Key::Control: m_isModifierDown[0] = down; break;
                    case window::Key::Shift: m_isModifierDown[1] = down; break;
                    case window::Key::Alt: m_isModifierDown[2] = down; break;
                    case window::Key::Super: m_isModifierDown[3] = down; break;
                    default: break;
                }

                ImGuiIO& io = ImGui::GetIO();
                io.AddKeyEvent(ImGuiKey_ModCtrl, m_isModifierDown[0]);
                io.AddKeyEvent(ImGuiKey_ModShift, m_isModifierDown[1]);
                io.AddKeyEvent(ImGuiKey_ModAlt, m_isModifierDown[2]);
                io.AddKeyEvent(ImGuiKey_ModSuper, m_isModifierDown[3]);
                io.AddKeyEvent(key, down);
                break;
            }
            default:
                break;
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

    void ImGuiPlatformService::UpdateMouseData()
    {
        ImGuiIO& io = ImGui::GetIO();
        HE_ASSERT(m_view != nullptr);

        const bool isViewportsEnabled = HasFlag(io.ConfigFlags, ImGuiConfigFlags_ViewportsEnable);
        const bool isAppFocused = m_view->IsChildFocused() || ImGui::FindViewportByPlatformHandle(m_device->GetFocusedView());
        if (isAppFocused)
        {
            // (Optional) Set OS mouse position from Dear ImGui if requested (rarely used, only
            // when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
            // When multi-viewports are enabled, all Dear ImGui positions are same as OS positions.
            if (io.WantSetMousePos)
            {
                window::View* relativeView = isViewportsEnabled ? nullptr : m_view;
                m_device->SetCursorPos(relativeView, { io.MousePos.x, io.MousePos.y });
            }

            // (Optional) Fallback to provide mouse position when focused.
            const Vec2f screenPos = m_device->GetCursorPos(nullptr);
            if (!io.WantSetMousePos && !m_mouseInside && screenPos != Vec2f_Infinity)
            {
                if (isViewportsEnabled)
                {
                    io.AddMousePosEvent(screenPos.x, screenPos.y);
                }
                else
                {
                    const Vec2f viewPos = m_view->ScreenToView(screenPos);
                    io.AddMousePosEvent(viewPos.x, viewPos.y);
                }
            }
        }

        // (Optional) When using multiple viewports: call io.AddMouseViewportEvent() with the
        // viewport the OS mouse cursor is hovering. If ImGuiBackendFlags_HasMouseHoveredViewport
        // is not set by the backend, Dear imGui will ignore this field and infer the information
        // using its flawed heuristic.
        ImGuiID mouseViewportId = 0;
        window::View* hoveredView = m_device->GetHoveredView();
        if (hoveredView)
        {
            ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(hoveredView);
            if (viewport)
            {
                mouseViewportId = viewport->ID;
            }
        }
        io.AddMouseViewportEvent(mouseViewportId);
    }

    void ImGuiPlatformService::UpdateMouseCursor()
    {
        ImGuiIO& io = ImGui::GetIO();

        if (HasFlag(io.ConfigFlags, ImGuiConfigFlags_NoMouseCursorChange))
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
        m_setupStyle(g.Style);
        g.Style.ScaleAllSizes(dpiScale);

        // Get or create the font atlas for this DPI scale
        UniquePtr<ImFontAtlas>& atlas = m_dpiFontAtlas[dpiScale];
        if (!atlas)
        {
            atlas = MakeUnique<ImFontAtlas>();
            m_setupFonts(*atlas, dpiScale);
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
        g.IO.Fonts = atlas.Get();
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

        if (HasFlag(vp->Flags, ImGuiViewportFlags_NoDecoration))
            desc.flags |= window::ViewFlag::Borderless;
        if (!HasFlag(vp->Flags, ImGuiViewportFlags_NoTaskBarIcon))
            desc.flags |= window::ViewFlag::TaskBarIcon;
        if (!HasFlag(vp->Flags, ImGuiViewportFlags_NoFocusOnClick))
            desc.flags |= window::ViewFlag::FocusOnClick;
        if (!HasFlag(vp->Flags, ImGuiViewportFlags_NoInputs))
            desc.flags |= window::ViewFlag::AcceptInput;
        if (HasFlag(vp->Flags, ImGuiViewportFlags_TopMost))
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

        const bool focus = !HasFlag(vp->Flags, ImGuiViewportFlags_NoFocusOnAppearing);
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
