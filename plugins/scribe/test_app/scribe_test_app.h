// Copyright Chad Engler

#pragma once

#include "he/scribe/layout_engine.h"
#include "he/scribe/renderer.h"

#include "he/core/clock.h"
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
            rhi::TimestampQuerySet* gpuTimerQueries{ nullptr };
            rhi::Buffer* gpuTimerReadback{ nullptr };
            bool hasSubmittedWork{ false };
        };

        struct RenderState
        {
            rhi::Instance* instance{ nullptr };
            rhi::Device* device{ nullptr };
            rhi::SwapChain* swapChain{ nullptr };
            rhi::RenderCmdList* cmdList{ nullptr };
            rhi::SwapChainFormat preferredSwapChainFormat{};
            rhi::PresentTarget presentTarget{};
            uint64_t renderQueueTimestampFrequency{ 0 };
            uint32_t frameIndex{ 0 };
            RenderFrameData frames[rhi::MaxFrameCount]{};
        };

        struct LoadedDemoFont
        {
            String name{};
            Vector<schema::Word> blobWords{};
            scribe::LoadedFontFaceBlob blob{};
        };

        struct LoadedDemoImage
        {
            String name{};
            Vector<schema::Word> blobWords{};
            scribe::LoadedVectorImageBlob blob{};
        };

        struct CachedGlyph
        {
            uint32_t fontFaceIndex{ 0 };
            uint32_t glyphIndex{ 0 };
            scribe::GlyphResource resource{};
        };

        struct CachedImageShape
        {
            uint32_t imageIndex{ 0 };
            uint32_t shapeIndex{ 0 };
            scribe::GlyphResource resource{};
        };

        enum class DemoScene : uint32_t
        {
            LatinWrap,
            CombiningAndFallback,
            RightToLeft,
            ColorGlyphLayers,
            SvgVectorImages,

            _Count,
        };

    private:
        bool Initialize();
        void Terminate();

        bool InitializeView();
        bool InitializeRenderState();
        void TerminateRenderState();
        bool LoadDemoFonts();
        bool LoadDemoImages();
        bool LoadDemoFont(LoadedDemoFont& out, const char* fileName);
        bool LoadOptionalDemoFont(LoadedDemoFont& out, Span<const char*> fileNames);
        bool LoadDemoImage(LoadedDemoImage& out, const char* fileName);
        bool RebuildLayouts();
        bool UpdateOverlayLayout();
        bool PrimeLayoutGlyphs(const scribe::LayoutResult& layout);
        bool PrimeGlyphCache();
        bool EnsureGlyphResource(uint32_t fontFaceIndex, uint32_t glyphIndex, const scribe::GlyphResource*& out);
        bool PrimeImageCache();
        bool EnsureImageShapeResource(uint32_t imageIndex, uint32_t shapeIndex, const scribe::GlyphResource*& out);
        void QueueDraw(const scribe::DrawGlyphDesc& desc);
        void QueueLayout(const scribe::LayoutResult& layout, const Vec2f& origin, float fontSize, float layoutScale = 1.0f);
        void QueueImage(const LoadedDemoImage& image, uint32_t imageIndex, const Vec2f& position, float scale);
        void QueueCaret();
        void UpdateSceneTitle();
        void ResetSceneView();
        void AdvanceScene(int32_t delta);
        rhi::SwapChainFormat FindPreferredSwapChainFormat() const;
        bool BeginFrame();
        void EndFrame();
        bool HasRtlDemoFallbackFont() const;
        bool HasColorDemoFont() const;

    private:
        window::Device* m_windowDevice{ nullptr };
        window::View* m_view{ nullptr };
        RenderState m_render{};
        scribe::Renderer m_renderer{};
        scribe::LayoutEngine m_layoutEngine{};
        scribe::GlyphResource m_caretGlyph{};
        Vector<LoadedDemoFont> m_fonts{};
        Vector<LoadedDemoImage> m_images{};
        Vector<CachedGlyph> m_cachedGlyphs{};
        Vector<CachedImageShape> m_cachedImageShapes{};
        String m_titleText{};
        String m_bodyText{};
        String m_sceneStatsText{};
        String m_renderStatsText{};
        String m_inputHintsText{};
        scribe::LayoutResult m_titleLayout{};
        scribe::LayoutResult m_bodyLayout{};
        scribe::LayoutResult m_sceneStatsLayout{};
        scribe::LayoutResult m_renderStatsLayout{};
        scribe::LayoutResult m_inputHintsLayout{};
        Vec2f m_titleOrigin{ 0.0f, 0.0f };
        Vec2f m_bodyOrigin{ 0.0f, 0.0f };
        Vec2f m_sceneStatsOrigin{ 0.0f, 0.0f };
        Vec2f m_renderStatsOrigin{ 0.0f, 0.0f };
        Vec2f m_inputHintsOrigin{ 0.0f, 0.0f };
        scribe::HitTestResult m_caretHit{};
        Vec2f m_lastPointerPos{ 0.0f, 0.0f };
        MonotonicTime m_lastFrameTime{};
        DemoScene m_scene{ DemoScene::LatinWrap };
        Vec2f m_scenePan{ 0.0f, 0.0f };
        float m_sceneZoom{ 1.0f };
        float m_smoothedFrameMs{ 0.0f };
        float m_lastGpuFrameMs{ 0.0f };
        uint32_t m_lastDrawCount{ 0 };
        uint32_t m_pendingDrawCount{ 0 };
        bool m_isPanning{ false };
        bool m_hasFrameTime{ false };
        bool m_initialized{ false };
        bool m_layoutDirty{ true };
        bool m_hasPointerPos{ false };
        bool m_hasCaret{ false };
    };
}
