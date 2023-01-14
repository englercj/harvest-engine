// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/clock.h"
#include "he/core/delegate.h"
#include "he/core/hash_table.h"
#include "he/core/unique_ptr.h"
#include "he/window/device.h"
#include "he/window/event.h"

#include "imgui.h"
#include "implot.h"

namespace he::editor
{
    class ImGuiPlatformService
    {
    public:
        using FontsSetupDelegate = Delegate<void(ImFontAtlas& atlas, float dpiScale)>;

    public:
        bool Initialize(
            window::Device* device,
            window::View* view,
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

        FontsSetupDelegate m_setupFonts{};

        ImGuiMouseCursor m_lastCursor{ ImGuiMouseCursor_COUNT };
        bool m_needToUpdateMonitors{ false };

        float m_lastDpiScale{ 0.0f };
        HashMap<float, UniquePtr<ImFontAtlas>> m_dpiFontAtlas{};
        ImFontAtlas* m_originalFontAtlas{ nullptr };

        bool m_isModifierDown[4]{};
        bool m_mouseInside{ false };
    };
}
