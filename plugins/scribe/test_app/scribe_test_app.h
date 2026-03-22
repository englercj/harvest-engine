// Copyright Chad Engler

#pragma once

#include "he/scribe/layout_engine.h"
#include "he/scribe/renderer.h"

#include "he/core/string.h"
#include "he/core/vector.h"
#include "he/rhi/types.h"
#include "he/window/application.h"

namespace he
{
    namespace rhi
    {
        struct CmdAllocator;
        struct CpuFence;
        class Device;
        class Instance;
        class RenderCmdList;
        struct SwapChain;
    }

    namespace window
    {
        class Device;
        class View;
    }

    class ScribeTestApp : public window::Application
    {
    public:
        explicit ScribeTestApp(window::Device* device);
        ~ScribeTestApp() noexcept override;

        void OnEvent(const window::Event& ev) override;
        void OnTick() override;

    private:
        struct RenderFrameData
        {
            rhi::CmdAllocator* cmdAlloc{ nullptr };
            rhi::CpuFence* fence{ nullptr };
        };

        struct RenderState
        {
            rhi::Instance* instance{ nullptr };
            rhi::Device* device{ nullptr };
            rhi::SwapChain* swapChain{ nullptr };
            rhi::RenderCmdList* cmdList{ nullptr };
            rhi::SwapChainFormat preferredSwapChainFormat{};
            rhi::PresentTarget presentTarget{};
            uint32_t frameIndex{ 0 };
            RenderFrameData frames[rhi::MaxFrameCount]{};
        };

        struct LoadedDemoFont
        {
            String name{};
            Vector<schema::Word> blobWords{};
            scribe::LoadedFontFaceBlob blob{};
        };

        struct CachedGlyph
        {
            uint32_t fontFaceIndex{ 0 };
            uint32_t glyphIndex{ 0 };
            scribe::GlyphResource resource{};
        };

        enum class DemoScene : uint32_t
        {
            LatinWrap,
            CombiningAndFallback,
            RightToLeft,

            _Count,
        };

    private:
        bool Initialize();
        void Terminate();

        bool InitializeView();
        bool InitializeRenderState();
        void TerminateRenderState();
        bool LoadDemoFonts();
        bool LoadDemoFont(LoadedDemoFont& out, const char* fileName);
        bool RebuildLayouts();
        bool EnsureGlyphResource(uint32_t fontFaceIndex, uint32_t glyphIndex, const scribe::GlyphResource*& out);
        void QueueLayout(const scribe::LayoutResult& layout, const Vec2f& origin, float fontSize);
        void QueueCaret();
        void UpdateSceneTitle();
        void AdvanceScene(int32_t delta);
        void UpdateCaretFromPointer();
        rhi::SwapChainFormat FindPreferredSwapChainFormat() const;
        bool BeginFrame();
        void EndFrame();

    private:
        window::Device* m_windowDevice{ nullptr };
        window::View* m_view{ nullptr };
        RenderState m_render{};
        scribe::Renderer m_renderer{};
        scribe::LayoutEngine m_layoutEngine{};
        scribe::GlyphResource m_caretGlyph{};
        Vector<LoadedDemoFont> m_fonts{};
        Vector<CachedGlyph> m_cachedGlyphs{};
        String m_titleText{};
        String m_bodyText{};
        String m_footerText{};
        scribe::LayoutResult m_titleLayout{};
        scribe::LayoutResult m_bodyLayout{};
        scribe::LayoutResult m_footerLayout{};
        Vec2f m_titleOrigin{ 0.0f, 0.0f };
        Vec2f m_bodyOrigin{ 0.0f, 0.0f };
        Vec2f m_footerOrigin{ 0.0f, 0.0f };
        scribe::HitTestResult m_caretHit{};
        Vec2f m_lastPointerPos{ 0.0f, 0.0f };
        DemoScene m_scene{ DemoScene::LatinWrap };
        bool m_initialized{ false };
        bool m_layoutDirty{ true };
        bool m_hasPointerPos{ false };
        bool m_hasCaret{ false };
    };
}
