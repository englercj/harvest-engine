// Copyright Chad Engler

#pragma once

#include "he/core/allocator.h"
#include "he/core/clock.h"
#include "he/window/device.h"
#include "he/window/event.h"

#include "imgui.h"

#include <unordered_map>
#include <memory>

namespace he::editor
{
    class ImGuiPlatformService
    {
    public:
        using SetupStyleFunc = void(*)(void* ctx, ImGuiStyle& style);
        using SetupFontAtlasFunc = void(*)(void* ctx, ImFontAtlas& atlas, float dpiScale);

    public:
        ImGuiPlatformService();

        bool Initialize(
            window::Device* device,
            window::View* view,
            SetupStyleFunc setupStyle,
            SetupFontAtlasFunc setupFontAtlas,
            void* setupCtx);

        void Terminate();

        void NewFrame();

        void OnEvent(const window::Event& ev);

    private:
        void UpdateMonitors();
        void UpdateMousePos();
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

        SetupStyleFunc m_setupStyle{ nullptr };
        SetupFontAtlasFunc m_setupFontAtlas{ nullptr };
        void* m_setupCtx{ nullptr };

        ImGuiMouseCursor m_lastCursor{ ImGuiMouseCursor_COUNT };
        bool m_needToUpdateMonitors{ false };

        std::unordered_map<float, std::unique_ptr<ImFontAtlas>> m_dpiFontAtlas{};
        ImFontAtlas* m_originalFontAtlas{ nullptr };
    };
}
