// Copyright Chad Engler

#pragma once

#include "he/scribe/compiled_font.h"
#include "he/scribe/layout_engine.h"
#include "he/scribe/retained_text.h"
#include "he/scribe/retained_vector_image.h"
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
        static constexpr uint32_t InvalidIndex = 0xFFFFFFFFu;

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

        struct CachedImageShape
        {
            uint32_t imageIndex{ 0 };
            uint32_t shapeIndex{ 0 };
            scribe::GlyphResource resource{};
        };

        struct SceneTextBlock
        {
            String text{};
            scribe::LayoutResult layout{};
            scribe::RetainedTextModel retainedText{};
            Vector<uint32_t> faceIndices{};
            Vector<scribe::TextStyle> styles{};
            Vector<scribe::TextStyleSpan> styleSpans{};
            Vector<scribe::TextFeatureSetting> features{};
            Vec2f origin{ 0.0f, 0.0f };
            float fontSize{ 16.0f };
            Vec4f color{ 0.0f, 0.0f, 0.0f, 1.0f };
            uint32_t fontFaceIndex{ 0 };
            bool useAllFaces{ true };
            bool pixelAlignBaseline{ false };
            bool pixelAlignCapHeight{ false };
        };

        struct SceneVectorImageBlock
        {
            String name{};
            scribe::RetainedVectorImageModel retainedImage{};
            Vec2f origin{ 0.0f, 0.0f };
            float scale{ 1.0f };
        };

        enum class DemoScene : uint32_t
        {
            FeatureOverview,
            RichParagraphs,
            EmojiPage,
            SvgGallery,
            SmallTextAlignment,

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
        void QueueCaret();
        void UpdateSceneTitle();
        void ResetSceneView();
        void AdvanceScene(int32_t delta);
        rhi::SwapChainFormat FindPreferredSwapChainFormat() const;
        bool BeginFrame();
        void EndFrame();
        bool HasRtlDemoFallbackFont() const;
        bool HasColorDemoFont() const;
        uint32_t GetSceneMissingGlyphCount() const;
        uint32_t GetSceneFallbackGlyphCount() const;
        Vec2f GetSceneBlockRenderOrigin(const SceneTextBlock& block) const;
        Vec2f GetSceneImageRenderOrigin(const SceneVectorImageBlock& block) const;

    private:
        window::Device* m_windowDevice{ nullptr };
        window::View* m_view{ nullptr };
        RenderState m_render{};
        scribe::Renderer m_renderer{};
        scribe::LayoutEngine m_layoutEngine{};
        scribe::GlyphResource m_caretGlyph{};
        Vector<LoadedDemoFont> m_fonts{};
        Vector<LoadedDemoImage> m_images{};
        Vector<String> m_svgLoadErrors{};
        Vector<CachedImageShape> m_cachedImageShapes{};
        String m_titleText{};
        String m_bodyText{};
        String m_sceneStatsText{};
        String m_renderStatsText{};
        String m_inputHintsText{};
        scribe::LayoutResult m_titleLayout{};
        scribe::LayoutResult m_bodyLayout{};
        scribe::RetainedTextModel m_retainedTitleText{};
        scribe::RetainedTextModel m_retainedBodyText{};
        Vector<SceneTextBlock> m_sceneBlocks{};
        Vector<SceneVectorImageBlock> m_sceneImages{};
        scribe::LayoutResult m_sceneStatsLayout{};
        scribe::LayoutResult m_renderStatsLayout{};
        scribe::LayoutResult m_inputHintsLayout{};
        scribe::RetainedTextModel m_retainedSceneStatsText{};
        scribe::RetainedTextModel m_retainedRenderStatsText{};
        scribe::RetainedTextModel m_retainedInputHintsText{};
        Vec2f m_titleOrigin{ 0.0f, 0.0f };
        Vec2f m_bodyOrigin{ 0.0f, 0.0f };
        Vec2f m_sceneStatsOrigin{ 0.0f, 0.0f };
        Vec2f m_renderStatsOrigin{ 0.0f, 0.0f };
        Vec2f m_inputHintsOrigin{ 0.0f, 0.0f };
        scribe::HitTestResult m_caretHit{};
        Vec2f m_lastPointerPos{ 0.0f, 0.0f };
        MonotonicTime m_lastFrameTime{};
        DemoScene m_scene{ DemoScene::FeatureOverview };
        Vec2f m_scenePan{ 0.0f, 0.0f };
        float m_sceneZoom{ 1.0f };
        float m_bodyFontSize{ 24.0f };
        float m_smoothedFrameMs{ 0.0f };
        float m_lastGpuFrameMs{ 0.0f };
        uint32_t m_sceneVertexEstimate{ 0 };
        uint32_t m_overlayVertexEstimate{ 0 };
        uint32_t m_lastDrawCount{ 0 };
        uint32_t m_uiFontIndex{ 0 };
        uint32_t m_iconFontIndex{ 1 };
        uint32_t m_rtlFontIndex{ InvalidIndex };
        uint32_t m_colorFontIndex{ InvalidIndex };
        uint32_t m_sansRegularFontIndex{ InvalidIndex };
        uint32_t m_sansBoldFontIndex{ InvalidIndex };
        uint32_t m_sansItalicFontIndex{ InvalidIndex };
        uint32_t m_sansBoldItalicFontIndex{ InvalidIndex };
        uint32_t m_monoFontIndex{ InvalidIndex };
        uint32_t m_serifRegularFontIndex{ InvalidIndex };
        uint32_t m_serifBoldFontIndex{ InvalidIndex };
        uint32_t m_serifItalicFontIndex{ InvalidIndex };
        uint32_t m_serifBoldItalicFontIndex{ InvalidIndex };
        uint32_t m_featureFontIndex{ InvalidIndex };
        uint32_t m_symbolFontIndex{ InvalidIndex };
        bool m_isPanning{ false };
        bool m_hasFrameTime{ false };
        bool m_initialized{ false };
        bool m_layoutDirty{ true };
        bool m_hasPointerPos{ false };
        bool m_hasCaret{ false };
    };
}
