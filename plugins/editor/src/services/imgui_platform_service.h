// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/clock.h"
#include "he/core/delegate.h"
#include "he/core/unique_ptr.h"
#include "he/window/device.h"
#include "he/window/event.h"

#include "imgui.h"

#include <unordered_map>

namespace he::editor
{
    class ImGuiPlatformService
    {
    public:
        using StyleSetupDelegate = Delegate<void(ImGuiStyle& style)>;
        using FontsSetupDelegate = Delegate<void(ImFontAtlas& atlas, float dpiScale)>;

    public:
        ImGuiPlatformService();

        bool Initialize(
            window::Device* device,
            window::View* view,
            StyleSetupDelegate setupStyle,
            FontsSetupDelegate setupFonts);

        void Terminate();

        void NewFrame();
        void UpdateViews();

        void OnEvent(const window::Event& ev);

    private:
        void UpdateMonitors();
        void UpdateMouseData();
        void UpdateMouseCursor();
        void UpdateDpiResources(float dpiScale);

        static void CreateWindow(ImGuiViewport* vp);
        static void DestroyWindow(ImGuiViewport* vp);
        static void ShowWindow(ImGuiViewport* vp);
        static void SetWindowPos(ImGuiViewport* vp, ImVec2 pos);
        static ImVec2 GetWindowPos(ImGuiViewport* vp);
        static void SetWindowSize(ImGuiViewport* vp, ImVec2 size);
        static ImVec2 GetWindowSize(ImGuiViewport* vp);
        static void SetWindowFocus(ImGuiViewport* vp);
        static bool GetWindowFocus(ImGuiViewport* vp);
        static bool GetWindowMinimized(ImGuiViewport* vp);
        static void SetWindowTitle(ImGuiViewport* vp, const char* str);
        static void SetWindowAlpha(ImGuiViewport* vp, float alpha);
        static float GetWindowDpiScale(ImGuiViewport* vp);
        static void OnChangedViewport(ImGuiViewport* vp);

    private:
        MonotonicTime m_time{};

        window::Device* m_device{ nullptr };
        window::View* m_view{ nullptr };

        StyleSetupDelegate m_setupStyle{};
        FontsSetupDelegate m_setupFonts{};

        ImGuiMouseCursor m_lastCursor{ ImGuiMouseCursor_COUNT };
        bool m_needToUpdateMonitors{ false };

        std::unordered_map<float, UniquePtr<ImFontAtlas>> m_dpiFontAtlas{};
        ImFontAtlas* m_originalFontAtlas{ nullptr };

        bool m_isModifierDown[4]{};
        bool m_mouseInside{ false };
    };
}
