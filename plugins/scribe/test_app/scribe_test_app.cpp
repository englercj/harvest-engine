// Copyright Chad Engler

#include "scribe_test_app.h"

#include "font_compile_geometry.h"
#include "font_import_utils.h"

#include "he/scribe/schema/runtime_blob.hsc.h"

#include "he/core/assert.h"
#include "he/core/file.h"
#include "he/core/log.h"
#include "he/core/macros.h"
#include "he/core/result_fmt.h"
#include "he/core/string_fmt.h"
#include "he/core/utils.h"
#include "he/rhi/cmd_list.h"
#include "he/rhi/cmd_queue.h"
#include "he/rhi/device.h"
#include "he/rhi/instance.h"
#include "he/window/device.h"
#include "he/window/event.h"
#include "he/window/key.h"
#include "he/window/view.h"

namespace he
{
    namespace
    {
        constexpr const char* TestIconAccount = "\xf3\xb0\x80\x84";
        constexpr const char* RtlSample = "\xd9\x85\xd8\xb1\xd8\xad\xd8\xa8\xd8\xa7 \xd8\xa8\xd8\xa7\xd9\x84\xd8\xb9\xd8\xa7\xd9\x84\xd9\x85";

        bool ResolveFontPath(String& out, const char* fileName)
        {
            if (File::Exists(fileName))
            {
                out = fileName;
                return true;
            }

            static const char* Candidates[] =
            {
                "plugins/editor/src/fonts/",
                "../../../plugins/editor/src/fonts/",
            };

            for (const char* base : Candidates)
            {
                out = base;
                out += fileName;
                if (File::Exists(out.Data()))
                {
                    return true;
                }
            }

            out.Clear();
            return false;
        }

        bool BuildLoadedFontFaceFromFile(const char* fileName, Vector<schema::Word>& storage, scribe::LoadedFontFaceBlob& out)
        {
            String path;
            if (!ResolveFontPath(path, fileName))
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Failed to resolve demo font path."),
                    HE_KV(file_name, fileName));
                return false;
            }

            const scribe::FontSourceFormat sourceFormat = scribe::editor::DeduceFontSourceFormat(fileName);
            if (sourceFormat == scribe::FontSourceFormat::Unknown)
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Unsupported demo font format."),
                    HE_KV(path, path));
                return false;
            }

            Vector<uint8_t> fontBytes;
            if (!scribe::editor::ReadFontSourceBytes(fontBytes, path.Data()))
            {
                return false;
            }

            scribe::editor::FontFaceInfo faceInfo{};
            if (!scribe::editor::InspectFontFace(fontBytes, 0, sourceFormat, faceInfo))
            {
                return false;
            }

            scribe::editor::CompiledFontRenderData renderData{};
            if (!scribe::editor::BuildCompiledFontRenderData(renderData, fontBytes, 0))
            {
                return false;
            }

            schema::Builder shapingBuilder;
            scribe::FontFaceShapingData::Builder shaping = shapingBuilder.AddStruct<scribe::FontFaceShapingData>();
            shaping.SetFaceIndex(faceInfo.faceIndex);
            shaping.SetSourceFormat(faceInfo.sourceFormat);
            shaping.SetSourceBytes(shapingBuilder.AddBlob(Span<const uint8_t>(fontBytes)));
            shapingBuilder.SetRoot(shaping);

            schema::Builder metadataBuilder;
            scribe::FontFaceImportMetadata::Builder metadata = metadataBuilder.AddStruct<scribe::FontFaceImportMetadata>();
            scribe::editor::FillFontFaceImportMetadata(metadata, faceInfo);
            metadataBuilder.SetRoot(metadata);

            schema::Builder renderBuilder;
            scribe::FontFaceRenderData::Builder render = renderBuilder.AddStruct<scribe::FontFaceRenderData>();
            render.SetCurveTextureWidth(renderData.curveTextureWidth);
            render.SetCurveTextureHeight(renderData.curveTextureHeight);
            render.SetBandTextureWidth(renderData.bandTextureWidth);
            render.SetBandTextureHeight(renderData.bandTextureHeight);
            render.SetBandOverlapEpsilon(renderData.bandOverlapEpsilon);

            auto glyphs = render.InitGlyphs(renderData.glyphs.Size());
            for (uint32_t glyphIndex = 0; glyphIndex < renderData.glyphs.Size(); ++glyphIndex)
            {
                const scribe::editor::CompiledGlyphRenderEntry& srcGlyph = renderData.glyphs[glyphIndex];
                scribe::FontFaceGlyphRenderData::Builder dstGlyph = glyphs[glyphIndex];
                dstGlyph.SetAdvanceX(srcGlyph.advanceX);
                dstGlyph.SetAdvanceY(srcGlyph.advanceY);
                dstGlyph.SetBoundsMinX(srcGlyph.boundsMinX);
                dstGlyph.SetBoundsMinY(srcGlyph.boundsMinY);
                dstGlyph.SetBoundsMaxX(srcGlyph.boundsMaxX);
                dstGlyph.SetBoundsMaxY(srcGlyph.boundsMaxY);
                dstGlyph.SetBandScaleX(srcGlyph.bandScaleX);
                dstGlyph.SetBandScaleY(srcGlyph.bandScaleY);
                dstGlyph.SetBandOffsetX(srcGlyph.bandOffsetX);
                dstGlyph.SetBandOffsetY(srcGlyph.bandOffsetY);
                dstGlyph.SetGlyphBandLocX(srcGlyph.glyphBandLocX);
                dstGlyph.SetGlyphBandLocY(srcGlyph.glyphBandLocY);
                dstGlyph.SetBandMaxX(srcGlyph.bandMaxX);
                dstGlyph.SetBandMaxY(srcGlyph.bandMaxY);
                dstGlyph.SetFillRule(srcGlyph.fillRule);
                dstGlyph.SetFlags(srcGlyph.flags);
            }
            renderBuilder.SetRoot(render);

            schema::Builder rootBuilder;
            scribe::CompiledFontFaceBlob::Builder root = rootBuilder.AddStruct<scribe::CompiledFontFaceBlob>();
            scribe::RuntimeBlobHeader::Builder header = root.InitHeader();
            header.SetFormatVersion(scribe::RuntimeBlobFormatVersion);
            header.SetKind(scribe::RuntimeBlobKind::FontFace);
            header.SetFlags(0);
            root.SetShapingData(rootBuilder.AddBlob(Span<const schema::Word>(shapingBuilder).AsBytes()));
            root.SetCurveData(rootBuilder.AddBlob(Span<const scribe::PackedCurveTexel>(renderData.curveTexels.Data(), renderData.curveTexels.Size()).AsBytes()));
            root.SetBandData(rootBuilder.AddBlob(Span<const scribe::PackedBandTexel>(renderData.bandTexels.Data(), renderData.bandTexels.Size()).AsBytes()));
            root.SetPaintData(rootBuilder.AddBlob({}));
            root.SetMetadataData(rootBuilder.AddBlob(Span<const schema::Word>(metadataBuilder).AsBytes()));
            root.SetRenderData(rootBuilder.AddBlob(Span<const schema::Word>(renderBuilder).AsBytes()));
            rootBuilder.SetRoot(root);

            storage = Span<const schema::Word>(rootBuilder);
            return scribe::LoadCompiledFontFaceBlob(out, storage);
        }
    }

    ScribeTestApp::ScribeTestApp(window::Device* device)
        : m_windowDevice(device)
    {}

    ScribeTestApp::~ScribeTestApp() noexcept
    {
        Terminate();
    }

    void ScribeTestApp::OnEvent(const window::Event& ev)
    {
        switch (ev.kind)
        {
            case window::EventKind::Initialized:
            {
                m_initialized = Initialize();
                if (!m_initialized)
                {
                    m_windowDevice->Quit(-1);
                }
                break;
            }

            case window::EventKind::Terminating:
            {
                Terminate();
                break;
            }

            case window::EventKind::ViewRequestClose:
            {
                m_windowDevice->Quit(0);
                break;
            }

            case window::EventKind::ViewResized:
            {
                const auto& evt = static_cast<const window::ViewResizedEvent&>(ev);
                if ((evt.size.x > 0) && (evt.size.y > 0) && m_render.swapChain && m_render.device)
                {
                    rhi::SwapChainDesc desc{};
                    desc.bufferCount = HE_LENGTH_OF(m_render.frames);
                    desc.format = m_render.preferredSwapChainFormat;
                    desc.size = evt.size;

                    Result r = m_render.device->UpdateSwapChain(m_render.swapChain, desc);
                    if (!r)
                    {
                        HE_LOG_ERROR(he_scribe,
                            HE_MSG("Failed to resize scribe testbed swap chain."),
                            HE_KV(result, r));
                    }
                }
                m_layoutDirty = true;
                break;
            }

            case window::EventKind::ViewDpiScaleChanged:
            {
                m_layoutDirty = true;
                break;
            }

            case window::EventKind::PointerMove:
            {
                const auto& evt = static_cast<const window::PointerMoveEvent&>(ev);
                if (evt.absolute)
                {
                    m_lastPointerPos = evt.pos;
                    m_hasPointerPos = true;
                }
                break;
            }

            case window::EventKind::PointerDown:
            {
                if (m_windowDevice && m_view)
                {
                    m_lastPointerPos = m_windowDevice->GetCursorPos(m_view);
                    m_hasPointerPos = true;
                }
                UpdateCaretFromPointer();
                break;
            }

            case window::EventKind::KeyDown:
            {
                const auto& evt = static_cast<const window::KeyDownEvent&>(ev);
                switch (evt.key)
                {
                    case window::Key::Escape:
                        m_windowDevice->Quit(0);
                        break;

                    case window::Key::Left:
                        AdvanceScene(-1);
                        break;

                    case window::Key::Right:
                        AdvanceScene(1);
                        break;

                    default:
                        break;
                }
                break;
            }

            default:
                break;
        }
    }

    void ScribeTestApp::OnTick()
    {
        if (!m_initialized || !m_view || !m_render.device || !m_render.swapChain || m_view->IsMinimized())
        {
            return;
        }

        if (m_layoutDirty && !RebuildLayouts())
        {
            return;
        }

        if (!BeginFrame())
        {
            return;
        }

        const float dpiScale = Max(m_view->GetDpiScale(), 1.0f);
        const float titleFontSize = 34.0f * dpiScale;
        const float bodyFontSize = 24.0f * dpiScale;
        const float footerFontSize = 16.0f * dpiScale;

        scribe::FrameDesc frameDesc{};
        frameDesc.cmdList = m_render.cmdList;
        frameDesc.targetView = m_render.presentTarget.renderTargetView;
        frameDesc.targetState = rhi::TextureState::Present;
        frameDesc.targetSize = {
            static_cast<uint32_t>(Max(m_view->GetSize().x, 0)),
            static_cast<uint32_t>(Max(m_view->GetSize().y, 0))
        };
        frameDesc.clearTarget = true;
        frameDesc.clearColor = { 0.055f, 0.066f, 0.082f, 1.0f };

        if (!m_renderer.BeginFrame(frameDesc))
        {
            EndFrame();
            return;
        }

        QueueLayout(m_titleLayout, m_titleOrigin, titleFontSize);
        QueueLayout(m_bodyLayout, m_bodyOrigin, bodyFontSize);
        QueueLayout(m_footerLayout, m_footerOrigin, footerFontSize);
        QueueCaret();

        m_renderer.EndFrame();
        EndFrame();
    }

    bool ScribeTestApp::Initialize()
    {
        if (!InitializeView()
            || !InitializeRenderState()
            || !m_renderer.Initialize(*m_render.device, m_render.preferredSwapChainFormat.format)
            || !m_renderer.CreateDebugGlyphResource(m_caretGlyph)
            || !LoadDemoFonts())
        {
            return false;
        }

        UpdateSceneTitle();
        m_layoutDirty = true;
        return true;
    }

    void ScribeTestApp::Terminate()
    {
        if (!m_windowDevice)
        {
            return;
        }

        if (m_render.device)
        {
            for (CachedGlyph& glyph : m_cachedGlyphs)
            {
                m_renderer.DestroyGlyphResource(glyph.resource);
            }
            m_cachedGlyphs.Clear();

            m_renderer.DestroyGlyphResource(m_caretGlyph);
        }

        m_renderer.Terminate();
        TerminateRenderState();
        if (m_view)
        {
            m_windowDevice->DestroyView(m_view);
        }
        m_view = nullptr;
        m_fonts.Clear();
        m_titleLayout.Clear();
        m_bodyLayout.Clear();
        m_footerLayout.Clear();
        m_initialized = false;
    }

    bool ScribeTestApp::InitializeView()
    {
        window::ViewDesc desc{};
        desc.title = "Harvest Scribe Testbed";
        desc.size = { 1440, 960 };

        m_view = m_windowDevice->CreateView(desc);
        if (!m_view)
        {
            HE_LOG_ERROR(he_scribe, HE_MSG("Failed to create scribe testbed view."));
            return false;
        }

        m_view->SetVisible(true, true);
        return true;
    }

    bool ScribeTestApp::InitializeRenderState()
    {
        HE_ASSERT(m_view);

        {
            rhi::InstanceDesc desc{};
            desc.enableDebugCpu = true;
            desc.enableDebugGpu = true;
            desc.enableDebugBreakOnError = true;
            desc.enableDebugBreakOnWarning = true;

            Result r = rhi::Instance::Create(desc, m_render.instance);
            if (!r)
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Failed to create scribe testbed RHI instance."),
                    HE_KV(result, r));
                return false;
            }
        }

        {
            rhi::DeviceDesc desc{};
            HE_RHI_SET_NAME(desc, "Scribe Testbed Device");

            Result r = m_render.instance->CreateDevice(desc, m_render.device);
            if (!r)
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Failed to create scribe testbed RHI device."),
                    HE_KV(result, r));
                return false;
            }
        }

        rhi::RenderCmdQueue& cmdQueue = m_render.device->GetRenderCmdQueue();
        for (uint32_t i = 0; i < HE_LENGTH_OF(m_render.frames); ++i)
        {
            RenderFrameData& frame = m_render.frames[i];

            rhi::CmdAllocatorDesc allocDesc{};
            allocDesc.type = rhi::CmdListType::Render;
            HE_RHI_SET_NAME(allocDesc, "Scribe Testbed Frame Command Allocator");

            Result r = m_render.device->CreateCmdAllocator(allocDesc, frame.cmdAlloc);
            if (!r)
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Failed to create scribe testbed frame allocator."),
                    HE_KV(result, r));
                return false;
            }

            rhi::CpuFenceDesc fenceDesc{};
            r = m_render.device->CreateCpuFence(fenceDesc, frame.fence);
            if (!r)
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Failed to create scribe testbed frame fence."),
                    HE_KV(result, r));
                return false;
            }

            cmdQueue.Signal(frame.fence);
        }

        {
            rhi::CmdListDesc desc{};
            desc.alloc = m_render.frames[0].cmdAlloc;
            HE_RHI_SET_NAME(desc, "Scribe Testbed Command List");

            Result r = m_render.device->CreateRenderCmdList(desc, m_render.cmdList);
            if (!r)
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Failed to create scribe testbed render command list."),
                    HE_KV(result, r));
                return false;
            }
        }

        m_render.preferredSwapChainFormat = FindPreferredSwapChainFormat();

        {
            rhi::SwapChainDesc desc{};
            desc.nativeViewHandle = m_view->GetNativeHandle();
            desc.bufferCount = HE_LENGTH_OF(m_render.frames);
            desc.format = m_render.preferredSwapChainFormat;
            desc.size = m_view->GetSize();
            desc.enableVSync = true;

            Result r = m_render.device->CreateSwapChain(desc, m_render.swapChain);
            if (!r)
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Failed to create scribe testbed swap chain."),
                    HE_KV(result, r));
                return false;
            }
        }

        return true;
    }

    void ScribeTestApp::TerminateRenderState()
    {
        if (!m_render.device)
        {
            if (m_render.instance)
            {
                rhi::Instance::Destroy(m_render.instance);
            }
            m_render = {};
            return;
        }

        m_render.device->GetRenderCmdQueue().WaitForFlush();
        m_render.device->SafeDestroy(m_render.swapChain);
        m_render.device->SafeDestroy(m_render.cmdList);

        for (RenderFrameData& frame : m_render.frames)
        {
            m_render.device->SafeDestroy(frame.fence);
            m_render.device->SafeDestroy(frame.cmdAlloc);
        }

        m_render.instance->DestroyDevice(m_render.device);
        rhi::Instance::Destroy(m_render.instance);
        m_render = {};
    }

    bool ScribeTestApp::LoadDemoFonts()
    {
        m_fonts.Clear();
        m_fonts.Resize(3);

        if (!LoadDemoFont(m_fonts[0], "NotoSans-Regular.ttf")
            || !LoadDemoFont(m_fonts[1], "materialdesignicons.ttf"))
        {
            return false;
        }

        static const char* RtlFontCandidates[] =
        {
            "C:/Windows/Fonts/segoeui.ttf",
            "C:/Windows/Fonts/arial.ttf",
            "C:/Windows/Fonts/tahoma.ttf",
        };

        if (!LoadOptionalDemoFont(m_fonts[2], RtlFontCandidates))
        {
            m_fonts.Resize(2);
        }

        return true;
    }

    bool ScribeTestApp::LoadDemoFont(LoadedDemoFont& out, const char* fileName)
    {
        out = {};
        out.name = fileName;
        return BuildLoadedFontFaceFromFile(fileName, out.blobWords, out.blob);
    }

    bool ScribeTestApp::LoadOptionalDemoFont(LoadedDemoFont& out, Span<const char*> fileNames)
    {
        out = {};

        for (const char* fileName : fileNames)
        {
            if (LoadDemoFont(out, fileName))
            {
                return true;
            }
        }

        return false;
    }

    bool ScribeTestApp::RebuildLayouts()
    {
        if (!m_view || m_fonts.IsEmpty())
        {
            return false;
        }

        UpdateSceneTitle();

        scribe::LoadedFontFaceBlob faces[8]{};
        uint32_t faceCount = 0;
        for (const LoadedDemoFont& font : m_fonts)
        {
            if (font.blob.root.IsValid())
            {
                faces[faceCount++] = font.blob;
            }
        }

        if (faceCount == 0)
        {
            return false;
        }

        const Span<const scribe::LoadedFontFaceBlob> faceSpan(faces, faceCount);
        const Span<const scribe::LoadedFontFaceBlob> primaryFace(faces, 1);
        const Vec2i viewSize = m_view->GetSize();
        const float dpiScale = Max(m_view->GetDpiScale(), 1.0f);
        const float margin = 40.0f * dpiScale;
        const float titleFontSize = 34.0f * dpiScale;
        const float bodyFontSize = 24.0f * dpiScale;
        const float footerFontSize = 16.0f * dpiScale;
        const float bodyWidth = Max(static_cast<float>(viewSize.x) - (margin * 2.0f), 128.0f);

        scribe::LayoutOptions titleOptions{};
        titleOptions.fontSize = titleFontSize;
        titleOptions.wrap = false;
        titleOptions.maxWidth = bodyWidth;
        titleOptions.direction = scribe::TextDirection::LeftToRight;

        if (!m_layoutEngine.LayoutText(m_titleLayout, primaryFace, m_titleText, titleOptions))
        {
            return false;
        }

        scribe::LayoutOptions bodyOptions{};
        bodyOptions.fontSize = bodyFontSize;
        bodyOptions.wrap = true;
        bodyOptions.maxWidth = bodyWidth;
        bodyOptions.direction = m_scene == DemoScene::RightToLeft
            ? scribe::TextDirection::RightToLeft
            : scribe::TextDirection::Auto;

        if (!m_layoutEngine.LayoutText(m_bodyLayout, faceSpan, m_bodyText, bodyOptions))
        {
            return false;
        }

        m_footerText.Clear();
        FormatTo(
            m_footerText,
            "Scene {}/{}  Left/Right: switch demos  Click: caret hit test  Esc: quit  Missing={}  Fallback={}",
            static_cast<uint32_t>(m_scene) + 1,
            static_cast<uint32_t>(DemoScene::_Count),
            m_bodyLayout.missingGlyphCount,
            m_bodyLayout.fallbackGlyphCount);

        scribe::LayoutOptions footerOptions{};
        footerOptions.fontSize = footerFontSize;
        footerOptions.wrap = true;
        footerOptions.maxWidth = bodyWidth;
        footerOptions.direction = scribe::TextDirection::LeftToRight;

        if (!m_layoutEngine.LayoutText(m_footerLayout, primaryFace, m_footerText, footerOptions))
        {
            return false;
        }

        m_titleOrigin = { margin, margin };
        m_bodyOrigin = { margin, margin + m_titleLayout.height + (22.0f * dpiScale) };

        float footerY = static_cast<float>(viewSize.y) - margin - m_footerLayout.height;
        footerY = Max(footerY, m_bodyOrigin.y + m_bodyLayout.height + (24.0f * dpiScale));
        m_footerOrigin = { margin, footerY };

        if (m_hasPointerPos)
        {
            UpdateCaretFromPointer();
        }

        if (!PrimeGlyphCache())
        {
            return false;
        }

        m_layoutDirty = false;
        return true;
    }

    bool ScribeTestApp::PrimeLayoutGlyphs(const scribe::LayoutResult& layout)
    {
        for (const scribe::ShapedGlyph& glyph : layout.glyphs)
        {
            if (glyph.fontFaceIndex >= m_fonts.Size())
            {
                continue;
            }

            const scribe::GlyphResource* glyphResource = nullptr;
            if (!EnsureGlyphResource(glyph.fontFaceIndex, glyph.glyphIndex, glyphResource))
            {
                continue;
            }
        }

        return true;
    }

    bool ScribeTestApp::PrimeGlyphCache()
    {
        return PrimeLayoutGlyphs(m_titleLayout)
            && PrimeLayoutGlyphs(m_bodyLayout)
            && PrimeLayoutGlyphs(m_footerLayout);
    }

    bool ScribeTestApp::EnsureGlyphResource(uint32_t fontFaceIndex, uint32_t glyphIndex, const scribe::GlyphResource*& out)
    {
        out = nullptr;

        for (CachedGlyph& cached : m_cachedGlyphs)
        {
            if ((cached.fontFaceIndex == fontFaceIndex) && (cached.glyphIndex == glyphIndex))
            {
                out = &cached.resource;
                return true;
            }
        }

        if (fontFaceIndex >= m_fonts.Size())
        {
            return false;
        }

        CachedGlyph& cached = m_cachedGlyphs.EmplaceBack();
        cached.fontFaceIndex = fontFaceIndex;
        cached.glyphIndex = glyphIndex;
        if (!m_renderer.CreateCompiledGlyphResource(cached.resource, m_fonts[fontFaceIndex].blob, glyphIndex))
        {
            m_cachedGlyphs.PopBack();
            return false;
        }

        out = &cached.resource;
        return true;
    }

    void ScribeTestApp::QueueLayout(const scribe::LayoutResult& layout, const Vec2f& origin, float fontSize)
    {
        for (const scribe::ShapedGlyph& glyph : layout.glyphs)
        {
            const scribe::GlyphResource* glyphResource = nullptr;
            if (!EnsureGlyphResource(glyph.fontFaceIndex, glyph.glyphIndex, glyphResource))
            {
                continue;
            }

            if (glyph.fontFaceIndex >= m_fonts.Size())
            {
                continue;
            }

            const uint32_t unitsPerEm = Max(m_fonts[glyph.fontFaceIndex].blob.metadata.GetMetrics().GetUnitsPerEm(), 1u);
            const float scale = fontSize / static_cast<float>(unitsPerEm);

            scribe::DrawGlyphDesc desc{};
            desc.glyph = glyphResource;
            desc.position = {
                origin.x + glyph.position.x,
                origin.y + glyph.position.y
            };
            desc.size = { scale, scale };
            m_renderer.QueueDraw(desc);
        }
    }

    void ScribeTestApp::QueueCaret()
    {
        if (!m_hasCaret || !m_caretGlyph.vertexBuffer || (m_caretHit.lineIndex >= m_bodyLayout.lines.Size()))
        {
            return;
        }

        const scribe::TextLine& line = m_bodyLayout.lines[m_caretHit.lineIndex];

        scribe::DrawGlyphDesc desc{};
        desc.glyph = &m_caretGlyph;
        desc.position = {
            m_bodyOrigin.x + m_caretHit.caretX,
            m_bodyOrigin.y + line.baselineY - line.ascent
        };
        desc.size = { 2.0f, Max(line.height, 1.0f) };
        m_renderer.QueueDraw(desc);
    }

    void ScribeTestApp::UpdateSceneTitle()
    {
        switch (m_scene)
        {
            case DemoScene::LatinWrap:
                m_titleText = "Scribe Testbed: paragraph wrapping and compiled font rendering";
                m_bodyText =
                    "Scribe compiles the source TTF in memory at startup, shapes text with HarfBuzz, "
                    "and renders directly from the compiled curve and band payloads. This scene is "
                    "the baseline paragraph demo for layout width, line metrics, and dense glyph submission.";
                break;

            case DemoScene::CombiningAndFallback:
                m_titleText = "Scribe Testbed: combining marks and fallback";
                m_bodyText =
                    "Combining cluster: A\xcc\x81 cafe. "
                    "Fallback icon from materialdesignicons: ";
                m_bodyText += TestIconAccount;
                m_bodyText +=
                    "  The body run should stay on the primary face until a glyph is missing, then "
                    "switch to the fallback face for only the covered cluster.";
                break;

            case DemoScene::RightToLeft:
                if (HasRtlDemoFallbackFont())
                {
                    m_titleText = "Scribe Testbed: right-to-left paragraph flow";
                    m_bodyText = RtlSample;
                    m_bodyText += " ";
                    m_bodyText += RtlSample;
                    m_bodyText += " 12345";
                }
                else
                {
                    m_titleText = "Scribe Testbed: forced RTL layout stress case";
                    m_bodyText =
                        "This machine does not have the optional RTL fallback font that the testbed uses "
                        "for Arabic coverage, so this scene stays on repository fonts and exercises the "
                        "first-pass right-to-left layout path with the currently available glyph set.";
                }
                break;

            case DemoScene::_Count:
                break;
        }

        if (m_view)
        {
            String windowTitle;
            FormatTo(windowTitle, "{} [{}]", m_titleText, m_fonts.IsEmpty() ? "no fonts" : m_fonts[0].name);
            m_view->SetTitle(windowTitle.Data());
        }
    }

    void ScribeTestApp::AdvanceScene(int32_t delta)
    {
        const int32_t count = static_cast<int32_t>(DemoScene::_Count);
        int32_t index = static_cast<int32_t>(m_scene);
        index = (index + delta) % count;
        if (index < 0)
        {
            index += count;
        }

        m_scene = static_cast<DemoScene>(index);
        m_layoutDirty = true;
    }

    void ScribeTestApp::UpdateCaretFromPointer()
    {
        if (!m_hasPointerPos)
        {
            return;
        }

        const Vec2f localPoint{
            m_lastPointerPos.x - m_bodyOrigin.x,
            m_lastPointerPos.y - m_bodyOrigin.y
        };

        m_hasCaret = m_layoutEngine.HitTest(m_bodyLayout, localPoint, m_caretHit);
    }

    rhi::SwapChainFormat ScribeTestApp::FindPreferredSwapChainFormat() const
    {
        uint32_t count = 0;
        Result r = m_render.device->GetSwapChainFormats(m_view->GetNativeHandle(), nullptr, count);
        if (!r || (count == 0))
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Falling back to a default swap chain format for the scribe testbed."),
                HE_KV(result, r),
                HE_KV(count, count));
            return { rhi::Format::BGRA8Unorm_sRGB, rhi::ColorSpace::sRGB };
        }

        Vector<rhi::SwapChainFormat> formats{};
        formats.Resize(count);

        r = m_render.device->GetSwapChainFormats(m_view->GetNativeHandle(), formats.Data(), count);
        if (!r)
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Failed to enumerate swap chain formats for the scribe testbed."),
                HE_KV(result, r));
            return { rhi::Format::BGRA8Unorm_sRGB, rhi::ColorSpace::sRGB };
        }

        for (const rhi::SwapChainFormat& format : formats)
        {
            if (format.colorSpace == rhi::ColorSpace::sRGB)
            {
                return format;
            }
        }

        return formats[0];
    }

    bool ScribeTestApp::BeginFrame()
    {
        if (!m_render.swapChain || !m_render.device)
        {
            return false;
        }

        m_render.presentTarget = m_render.device->AcquirePresentTarget(m_render.swapChain);
        if (!m_render.presentTarget.renderTargetView)
        {
            return false;
        }

        m_render.frameIndex = (m_render.frameIndex + 1) % HE_LENGTH_OF(m_render.frames);
        RenderFrameData& frame = m_render.frames[m_render.frameIndex];

        const bool waitResult = m_render.device->WaitForFence(frame.fence);
        if (!waitResult)
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Failed to wait for the scribe testbed frame fence."),
                HE_KV(result, Result::FromLastError()));
            return false;
        }

        Result r = m_render.device->ResetCmdAllocator(frame.cmdAlloc);
        if (!r)
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Failed to reset scribe testbed frame allocator."),
                HE_KV(result, r));
            return false;
        }

        r = m_render.cmdList->Begin(frame.cmdAlloc);
        if (!r)
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Failed to begin the scribe testbed command list."),
                HE_KV(result, r));
            return false;
        }

        return true;
    }

    void ScribeTestApp::EndFrame()
    {
        if (!m_render.cmdList || !m_render.device || !m_render.swapChain)
        {
            return;
        }

        Result r = m_render.cmdList->End();
        if (!r)
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Failed to end the scribe testbed command list."),
                HE_KV(result, r));
            return;
        }

        rhi::RenderCmdQueue& cmdQueue = m_render.device->GetRenderCmdQueue();
        cmdQueue.Submit(m_render.cmdList);
        cmdQueue.Signal(m_render.frames[m_render.frameIndex].fence);
        r = cmdQueue.Present(m_render.swapChain);
        if (!r)
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Failed to present the scribe testbed swap chain."),
                HE_KV(result, r));
        }
    }

    bool ScribeTestApp::HasRtlDemoFallbackFont() const
    {
        return (m_fonts.Size() > 2) && m_fonts[2].blob.root.IsValid();
    }
}
