// Copyright Chad Engler

#include "scribe_test_app.h"

#include "font_compile_geometry.h"
#include "font_import_utils.h"
#include "image_compile_geometry.h"

#include "he/scribe/compiled_font.h"
#include "he/scribe/compiled_vector_image.h"
#include "he/scribe/schema/runtime_blob.hsc.h"

#include "he/core/assert.h"
#include "he/core/clock.h"
#include "he/core/file.h"
#include "he/core/log.h"
#include "he/core/macros.h"
#include "he/core/math.h"
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

        bool ResolveImagePath(String& out, const char* fileName)
        {
            if (File::Exists(fileName))
            {
                out = fileName;
                return true;
            }

            static const char* Candidates[] =
            {
                "plugins/scribe/test_app/assets/",
                "../../../plugins/scribe/test_app/assets/",
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

            schema::Builder paintBuilder;
            scribe::FontFacePaintData::Builder paint = paintBuilder.AddStruct<scribe::FontFacePaintData>();
            paint.SetDefaultPaletteIndex(renderData.paint.defaultPaletteIndex);

            auto palettes = paint.InitPalettes(renderData.paint.palettes.Size());
            for (uint32_t paletteIndex = 0; paletteIndex < renderData.paint.palettes.Size(); ++paletteIndex)
            {
                const scribe::editor::CompiledFontPalette& srcPalette = renderData.paint.palettes[paletteIndex];
                scribe::FontFacePalette::Builder dstPalette = palettes[paletteIndex];
                dstPalette.SetFlags(srcPalette.flags);

                auto colors = dstPalette.InitColors(srcPalette.colors.Size());
                for (uint32_t colorIndex = 0; colorIndex < srcPalette.colors.Size(); ++colorIndex)
                {
                    const scribe::editor::CompiledFontPaletteColor& srcColor = srcPalette.colors[colorIndex];
                    scribe::FontFacePaletteColor::Builder dstColor = colors[colorIndex];
                    dstColor.SetRed(srcColor.red);
                    dstColor.SetGreen(srcColor.green);
                    dstColor.SetBlue(srcColor.blue);
                    dstColor.SetAlpha(srcColor.alpha);
                }
            }

            auto colorGlyphs = paint.InitColorGlyphs(renderData.paint.colorGlyphs.Size());
            for (uint32_t glyphIndex = 0; glyphIndex < renderData.paint.colorGlyphs.Size(); ++glyphIndex)
            {
                const scribe::editor::CompiledColorGlyphEntry& srcColorGlyph = renderData.paint.colorGlyphs[glyphIndex];
                scribe::FontFaceColorGlyph::Builder dstColorGlyph = colorGlyphs[glyphIndex];
                dstColorGlyph.SetFirstLayer(srcColorGlyph.firstLayer);
                dstColorGlyph.SetLayerCount(srcColorGlyph.layerCount);
            }

            auto layers = paint.InitLayers(renderData.paint.layers.Size());
            for (uint32_t layerIndex = 0; layerIndex < renderData.paint.layers.Size(); ++layerIndex)
            {
                const scribe::editor::CompiledColorGlyphLayerEntry& srcLayer = renderData.paint.layers[layerIndex];
                scribe::FontFaceColorGlyphLayer::Builder dstLayer = layers[layerIndex];
                dstLayer.SetGlyphIndex(srcLayer.glyphIndex);
                dstLayer.SetPaletteEntryIndex(srcLayer.paletteEntryIndex);
                dstLayer.SetFlags(srcLayer.flags);
                dstLayer.SetAlphaScale(srcLayer.alphaScale);
                dstLayer.SetTransform00(srcLayer.transform00);
                dstLayer.SetTransform01(srcLayer.transform01);
                dstLayer.SetTransform10(srcLayer.transform10);
                dstLayer.SetTransform11(srcLayer.transform11);
                dstLayer.SetTransformTx(srcLayer.transformTx);
                dstLayer.SetTransformTy(srcLayer.transformTy);
            }
            paintBuilder.SetRoot(paint);

            schema::Builder rootBuilder;
            scribe::CompiledFontFaceBlob::Builder root = rootBuilder.AddStruct<scribe::CompiledFontFaceBlob>();
            scribe::RuntimeBlobHeader::Builder header = root.InitHeader();
            header.SetFormatVersion(scribe::RuntimeBlobFormatVersion);
            header.SetKind(scribe::RuntimeBlobKind::FontFace);
            header.SetFlags(0);
            root.SetShapingData(rootBuilder.AddBlob(Span<const schema::Word>(shapingBuilder).AsBytes()));
            root.SetCurveData(rootBuilder.AddBlob(Span<const scribe::PackedCurveTexel>(renderData.curveTexels.Data(), renderData.curveTexels.Size()).AsBytes()));
            root.SetBandData(rootBuilder.AddBlob(Span<const scribe::PackedBandTexel>(renderData.bandTexels.Data(), renderData.bandTexels.Size()).AsBytes()));
            root.SetPaintData(rootBuilder.AddBlob(Span<const schema::Word>(paintBuilder).AsBytes()));
            root.SetMetadataData(rootBuilder.AddBlob(Span<const schema::Word>(metadataBuilder).AsBytes()));
            root.SetRenderData(rootBuilder.AddBlob(Span<const schema::Word>(renderBuilder).AsBytes()));
            rootBuilder.SetRoot(root);

            storage = Span<const schema::Word>(rootBuilder);
            return scribe::LoadCompiledFontFaceBlob(out, storage);
        }

        bool BuildLoadedVectorImageFromFile(const char* fileName, Vector<schema::Word>& storage, scribe::LoadedVectorImageBlob& out)
        {
            String path;
            if (!ResolveImagePath(path, fileName))
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Failed to resolve demo SVG path."),
                    HE_KV(file_name, fileName));
                return false;
            }

            Vector<uint8_t> imageBytes;
            Result readResult = File::ReadAll(imageBytes, path.Data());
            if (!readResult)
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Failed to read demo SVG source."),
                    HE_KV(path, path),
                    HE_KV(result, readResult));
                return false;
            }

            scribe::editor::CompiledVectorImageData imageData{};
            if (!scribe::editor::BuildCompiledVectorImageData(imageData, imageBytes, 0.25f))
            {
                return false;
            }

            schema::Builder metadataBuilder;
            scribe::VectorImageRuntimeMetadata::Builder metadata = metadataBuilder.AddStruct<scribe::VectorImageRuntimeMetadata>();
            metadata.SetSourceViewBoxMinX(imageData.viewBoxMinX);
            metadata.SetSourceViewBoxMinY(imageData.viewBoxMinY);
            metadata.SetSourceViewBoxWidth(imageData.viewBoxWidth);
            metadata.SetSourceViewBoxHeight(imageData.viewBoxHeight);
            metadata.SetSourceBoundsMinX(imageData.boundsMinX);
            metadata.SetSourceBoundsMinY(imageData.boundsMinY);
            metadata.SetSourceBoundsMaxX(imageData.boundsMaxX);
            metadata.SetSourceBoundsMaxY(imageData.boundsMaxY);
            metadataBuilder.SetRoot(metadata);

            schema::Builder renderBuilder;
            scribe::VectorImageRenderData::Builder render = renderBuilder.AddStruct<scribe::VectorImageRenderData>();
            render.SetCurveTextureWidth(imageData.curveTextureWidth);
            render.SetCurveTextureHeight(imageData.curveTextureHeight);
            render.SetBandTextureWidth(imageData.bandTextureWidth);
            render.SetBandTextureHeight(imageData.bandTextureHeight);
            render.SetBandOverlapEpsilon(imageData.bandOverlapEpsilon);
            auto shapes = render.InitShapes(imageData.shapes.Size());
            for (uint32_t shapeIndex = 0; shapeIndex < imageData.shapes.Size(); ++shapeIndex)
            {
                const scribe::editor::CompiledVectorShapeRenderEntry& srcShape = imageData.shapes[shapeIndex];
                scribe::VectorImageShapeRenderData::Builder dstShape = shapes[shapeIndex];
                dstShape.SetBoundsMinX(srcShape.boundsMinX);
                dstShape.SetBoundsMinY(srcShape.boundsMinY);
                dstShape.SetBoundsMaxX(srcShape.boundsMaxX);
                dstShape.SetBoundsMaxY(srcShape.boundsMaxY);
                dstShape.SetBandScaleX(srcShape.bandScaleX);
                dstShape.SetBandScaleY(srcShape.bandScaleY);
                dstShape.SetBandOffsetX(srcShape.bandOffsetX);
                dstShape.SetBandOffsetY(srcShape.bandOffsetY);
                dstShape.SetGlyphBandLocX(srcShape.glyphBandLocX);
                dstShape.SetGlyphBandLocY(srcShape.glyphBandLocY);
                dstShape.SetBandMaxX(srcShape.bandMaxX);
                dstShape.SetBandMaxY(srcShape.bandMaxY);
                dstShape.SetFillRule(srcShape.fillRule);
                dstShape.SetFlags(srcShape.flags);
            }
            renderBuilder.SetRoot(render);

            schema::Builder paintBuilder;
            scribe::VectorImagePaintData::Builder paint = paintBuilder.AddStruct<scribe::VectorImagePaintData>();
            auto layers = paint.InitLayers(imageData.layers.Size());
            for (uint32_t layerIndex = 0; layerIndex < imageData.layers.Size(); ++layerIndex)
            {
                const scribe::editor::CompiledVectorImageLayerEntry& srcLayer = imageData.layers[layerIndex];
                scribe::VectorImageLayer::Builder dstLayer = layers[layerIndex];
                dstLayer.SetShapeIndex(srcLayer.shapeIndex);
                dstLayer.SetRed(srcLayer.red);
                dstLayer.SetGreen(srcLayer.green);
                dstLayer.SetBlue(srcLayer.blue);
                dstLayer.SetAlpha(srcLayer.alpha);
            }
            paintBuilder.SetRoot(paint);

            schema::Builder rootBuilder;
            scribe::CompiledVectorImageBlob::Builder root = rootBuilder.AddStruct<scribe::CompiledVectorImageBlob>();
            scribe::RuntimeBlobHeader::Builder header = root.InitHeader();
            header.SetFormatVersion(scribe::RuntimeBlobFormatVersion);
            header.SetKind(scribe::RuntimeBlobKind::VectorImage);
            header.SetFlags(0);
            root.SetCurveData(rootBuilder.AddBlob(Span<const scribe::PackedCurveTexel>(imageData.curveTexels.Data(), imageData.curveTexels.Size()).AsBytes()));
            root.SetBandData(rootBuilder.AddBlob(Span<const scribe::PackedBandTexel>(imageData.bandTexels.Data(), imageData.bandTexels.Size()).AsBytes()));
            root.SetPaintData(rootBuilder.AddBlob(Span<const schema::Word>(paintBuilder).AsBytes()));
            root.SetMetadataData(rootBuilder.AddBlob(Span<const schema::Word>(metadataBuilder).AsBytes()));
            root.SetRenderData(rootBuilder.AddBlob(Span<const schema::Word>(renderBuilder).AsBytes()));
            rootBuilder.SetRoot(root);

            storage = Span<const schema::Word>(rootBuilder);
            return scribe::LoadCompiledVectorImageBlob(out, storage);
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
                if (m_isPanning)
                {
                    if (evt.absolute)
                    {
                        m_scenePan.x += evt.pos.x - m_lastPointerPos.x;
                        m_scenePan.y += evt.pos.y - m_lastPointerPos.y;
                    }
                    else
                    {
                        m_scenePan.x += evt.pos.x;
                        m_scenePan.y += evt.pos.y;
                    }
                }

                if (evt.absolute)
                {
                    m_lastPointerPos = evt.pos;
                    m_hasPointerPos = true;
                }
                else if (m_windowDevice && m_view)
                {
                    m_lastPointerPos = m_windowDevice->GetCursorPos(m_view);
                    m_hasPointerPos = true;
                }
                break;
            }

            case window::EventKind::PointerDown:
            {
                const auto& evt = static_cast<const window::PointerDownEvent&>(ev);
                if ((evt.button == window::PointerButton::Primary) && m_windowDevice && m_view)
                {
                    m_lastPointerPos = m_windowDevice->GetCursorPos(m_view);
                    m_hasPointerPos = true;
                    m_isPanning = true;
                }
                m_hasCaret = false;
                break;
            }

            case window::EventKind::PointerUp:
            {
                const auto& evt = static_cast<const window::PointerUpEvent&>(ev);
                if (evt.button == window::PointerButton::Primary)
                {
                    m_isPanning = false;
                }
                break;
            }

            case window::EventKind::PointerWheel:
            {
                const auto& evt = static_cast<const window::PointerWheelEvent&>(ev);
                const float wheelDelta = evt.delta.y;
                if (Abs(wheelDelta) > 0.0f)
                {
                    const float zoomFactor = Pow(1.1f, wheelDelta);
                    const float oldZoom = m_sceneZoom;
                    const float newZoom = Clamp(oldZoom * zoomFactor, 0.25f, 12.0f);
                    if (Abs(newZoom - oldZoom) > 0.0001f)
                    {
                        Vec2f pivot = m_hasPointerPos
                            ? m_lastPointerPos
                            : Vec2f{
                                static_cast<float>(m_view ? m_view->GetSize().x : 0) * 0.5f,
                                static_cast<float>(m_view ? m_view->GetSize().y : 0) * 0.5f
                            };
                        const Vec2f scenePoint{
                            (pivot.x - m_scenePan.x) / oldZoom,
                            (pivot.y - m_scenePan.y) / oldZoom
                        };
                        m_sceneZoom = newZoom;
                        m_scenePan = {
                            pivot.x - (scenePoint.x * newZoom),
                            pivot.y - (scenePoint.y * newZoom)
                        };
                    }
                }
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

                    case window::Key::R:
                        ResetSceneView();
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

        const MonotonicTime frameNow = MonotonicClock::Now();
        if (m_hasFrameTime)
        {
            const float frameMs = ToPeriod<Seconds, float>(frameNow - m_lastFrameTime) * 1000.0f;
            if (m_smoothedFrameMs <= 0.0f)
            {
                m_smoothedFrameMs = frameMs;
            }
            else
            {
                m_smoothedFrameMs = Lerp(m_smoothedFrameMs, frameMs, 0.12f);
            }
        }
        m_lastFrameTime = frameNow;
        m_hasFrameTime = true;

        if (!UpdateOverlayLayout())
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
        const auto ApplySceneTransform = [this](const Vec2f& point) -> Vec2f
        {
            return {
                (point.x * m_sceneZoom) + m_scenePan.x,
                (point.y * m_sceneZoom) + m_scenePan.y
            };
        };
        const Vec2f transformedTitleOrigin = ApplySceneTransform(m_titleOrigin);
        const Vec2f transformedBodyOrigin = ApplySceneTransform(m_bodyOrigin);

        scribe::FrameDesc frameDesc{};
        frameDesc.cmdList = m_render.cmdList;
        frameDesc.targetView = m_render.presentTarget.renderTargetView;
        frameDesc.targetState = rhi::TextureState::Present;
        frameDesc.targetSize = {
            static_cast<uint32_t>(Max(m_view->GetSize().x, 0)),
            static_cast<uint32_t>(Max(m_view->GetSize().y, 0))
        };
        frameDesc.clearTarget = true;
        frameDesc.clearColor = { 1.0f, 1.0f, 1.0f, 1.0f };

        if (!m_renderer.BeginFrame(frameDesc))
        {
            EndFrame();
            return;
        }

        QueueLayout(m_titleLayout, transformedTitleOrigin, titleFontSize, m_sceneZoom);
        QueueLayout(m_bodyLayout, transformedBodyOrigin, bodyFontSize, m_sceneZoom);
        if ((m_scene == DemoScene::SvgVectorImages) && (m_images.Size() >= 2))
        {
            const float imageScale = 2.4f * dpiScale * m_sceneZoom;
            QueueImage(
                m_images[0],
                0,
                ApplySceneTransform({ m_bodyOrigin.x, m_bodyOrigin.y + (150.0f * dpiScale) }),
                imageScale);
            QueueImage(
                m_images[1],
                1,
                ApplySceneTransform({ m_bodyOrigin.x + (620.0f * dpiScale), m_bodyOrigin.y + (150.0f * dpiScale) }),
                imageScale * 0.85f);
        }
        QueueLayout(m_sceneStatsLayout, m_sceneStatsOrigin, footerFontSize);
        QueueLayout(m_renderStatsLayout, m_renderStatsOrigin, footerFontSize);
        QueueLayout(m_inputHintsLayout, m_inputHintsOrigin, footerFontSize);
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
            || !LoadDemoFonts()
            || !LoadDemoImages())
        {
            return false;
        }

        UpdateSceneTitle();
        m_lastFrameTime = MonotonicClock::Now();
        m_hasFrameTime = false;
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

            for (CachedImageShape& shape : m_cachedImageShapes)
            {
                m_renderer.DestroyGlyphResource(shape.resource);
            }
            m_cachedImageShapes.Clear();

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
        m_images.Clear();
        m_titleLayout.Clear();
        m_bodyLayout.Clear();
        m_sceneStatsLayout.Clear();
        m_renderStatsLayout.Clear();
        m_inputHintsLayout.Clear();
        m_initialized = false;
    }

    bool ScribeTestApp::InitializeView()
    {
        window::ViewDesc desc{};
        desc.title = "Harvest Scribe Testbed";
        desc.size = { 1800, 1200 };

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
        m_fonts.Resize(2);

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

        m_fonts.Resize(3);
        if (!LoadOptionalDemoFont(m_fonts[2], RtlFontCandidates))
        {
            m_fonts.Resize(2);
        }

        static const char* ColorFontCandidates[] =
        {
            "C:/Windows/Fonts/seguiemj.ttf",
        };

        const uint32_t colorFontIndex = m_fonts.Size();
        m_fonts.Resize(colorFontIndex + 1);
        if (!LoadOptionalDemoFont(m_fonts[colorFontIndex], ColorFontCandidates))
        {
            m_fonts.Resize(colorFontIndex);
        }

        return true;
    }

    bool ScribeTestApp::LoadDemoImages()
    {
        m_images.Clear();
        m_images.Resize(2);
        return LoadDemoImage(m_images[0], "vector_layers.svg")
            && LoadDemoImage(m_images[1], "vector_evenodd.svg");
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

    bool ScribeTestApp::LoadDemoImage(LoadedDemoImage& out, const char* fileName)
    {
        out = {};
        out.name = fileName;
        return BuildLoadedVectorImageFromFile(fileName, out.blobWords, out.blob);
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
        const float bodyWidth = Max(static_cast<float>(viewSize.x) - (margin * 2.0f), 128.0f);

        scribe::LayoutOptions titleOptions{};
        titleOptions.fontSize = titleFontSize;
        titleOptions.wrap = true;
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

        if (!UpdateOverlayLayout())
        {
            return false;
        }

        const float overlayHeight = Max(m_sceneStatsLayout.height, Max(m_renderStatsLayout.height, m_inputHintsLayout.height));
        const float sceneTop = margin + overlayHeight + (20.0f * dpiScale);
        m_titleOrigin = { margin, sceneTop };
        m_bodyOrigin = { margin, sceneTop + m_titleLayout.height + (22.0f * dpiScale) };
        if (m_scene == DemoScene::SvgVectorImages)
        {
            m_bodyOrigin = { margin, sceneTop + m_titleLayout.height + (18.0f * dpiScale) };
        }

        m_hasCaret = false;

        if (!PrimeGlyphCache())
        {
            return false;
        }

        if ((m_scene == DemoScene::SvgVectorImages) && !PrimeImageCache())
        {
            return false;
        }

        m_layoutDirty = false;
        return true;
    }

    bool ScribeTestApp::UpdateOverlayLayout()
    {
        if (m_fonts.IsEmpty() || !m_fonts[0].blob.root.IsValid())
        {
            return false;
        }

        const Vec2i viewSize = m_view->GetSize();
        const float dpiScale = Max(m_view->GetDpiScale(), 1.0f);
        const float margin = 40.0f * dpiScale;
        const float overlayFontSize = 16.0f * dpiScale;
        const float overlayWidth = Max(static_cast<float>(viewSize.x) - (margin * 2.0f), 128.0f);
        const float columnGap = 48.0f * dpiScale;
        const float columnWidth = Max((overlayWidth - (columnGap * 2.0f)) / 3.0f, 128.0f);
        const float fps = m_smoothedFrameMs > 0.0f ? (1000.0f / m_smoothedFrameMs) : 0.0f;

        m_sceneStatsText.Clear();
        FormatTo(
            m_sceneStatsText,
            "Scene: {}/{}\n"
            "Missing: {}\n"
            "Fallback: {}",
            static_cast<uint32_t>(m_scene) + 1,
            static_cast<uint32_t>(DemoScene::_Count),
            m_bodyLayout.missingGlyphCount,
            m_bodyLayout.fallbackGlyphCount);

        m_renderStatsText.Clear();
        FormatTo(
            m_renderStatsText,
            "FPS: {:.1f}\n"
            "GPU: {:.2f} ms\n"
            "Resolution: {} x {}",
            fps,
            m_lastGpuFrameMs,
            viewSize.x,
            viewSize.y);

        m_inputHintsText =
            "Left drag: pan\n"
            "Mouse wheel: zoom\n"
            "R: reset view\n"
            "Left/Right: switch demos\n"
            "Esc: quit";

        scribe::LayoutOptions overlayOptions{};
        overlayOptions.fontSize = overlayFontSize;
        overlayOptions.wrap = true;
        overlayOptions.maxWidth = columnWidth;
        overlayOptions.direction = scribe::TextDirection::LeftToRight;

        const scribe::LoadedFontFaceBlob primaryFace[] = { m_fonts[0].blob };
        const Span<const scribe::LoadedFontFaceBlob> faceSpan(primaryFace, 1);
        if (!m_layoutEngine.LayoutText(m_sceneStatsLayout, faceSpan, m_sceneStatsText, overlayOptions)
            || !m_layoutEngine.LayoutText(m_renderStatsLayout, faceSpan, m_renderStatsText, overlayOptions)
            || !m_layoutEngine.LayoutText(m_inputHintsLayout, faceSpan, m_inputHintsText, overlayOptions))
        {
            return false;
        }

        m_sceneStatsOrigin = { margin, margin };
        const float inputX = Max(static_cast<float>(viewSize.x) - margin - m_inputHintsLayout.width, margin);
        const float minRenderX = m_sceneStatsOrigin.x + m_sceneStatsLayout.width + columnGap;
        const float maxRenderX = inputX - columnGap - m_renderStatsLayout.width;
        float renderX = (static_cast<float>(viewSize.x) - m_renderStatsLayout.width) * 0.5f;
        if (maxRenderX < minRenderX)
        {
            renderX = minRenderX;
        }
        else
        {
            renderX = Clamp(renderX, minRenderX, maxRenderX);
        }

        m_renderStatsOrigin = { renderX, margin };
        m_inputHintsOrigin = { inputX, margin };
        return PrimeLayoutGlyphs(m_sceneStatsLayout)
            && PrimeLayoutGlyphs(m_renderStatsLayout)
            && PrimeLayoutGlyphs(m_inputHintsLayout);
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
            && PrimeLayoutGlyphs(m_sceneStatsLayout)
            && PrimeLayoutGlyphs(m_renderStatsLayout)
            && PrimeLayoutGlyphs(m_inputHintsLayout);
    }

    bool ScribeTestApp::PrimeImageCache()
    {
        for (uint32_t imageIndex = 0; imageIndex < m_images.Size(); ++imageIndex)
        {
            const LoadedDemoImage& image = m_images[imageIndex];
            if (!image.blob.render.IsValid() || !image.blob.paint.IsValid())
            {
                continue;
            }

            const auto layers = image.blob.paint.GetLayers();
            for (uint32_t layerIndex = 0; layerIndex < layers.Size(); ++layerIndex)
            {
                const scribe::GlyphResource* shapeResource = nullptr;
                if (!EnsureImageShapeResource(imageIndex, layers[layerIndex].GetShapeIndex(), shapeResource))
                {
                    return false;
                }
            }
        }

        return true;
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

    bool ScribeTestApp::EnsureImageShapeResource(uint32_t imageIndex, uint32_t shapeIndex, const scribe::GlyphResource*& out)
    {
        out = nullptr;

        for (CachedImageShape& cached : m_cachedImageShapes)
        {
            if ((cached.imageIndex == imageIndex) && (cached.shapeIndex == shapeIndex))
            {
                out = &cached.resource;
                return true;
            }
        }

        if (imageIndex >= m_images.Size())
        {
            return false;
        }

        CachedImageShape& cached = m_cachedImageShapes.EmplaceBack();
        cached.imageIndex = imageIndex;
        cached.shapeIndex = shapeIndex;
        if (!m_renderer.CreateCompiledVectorShapeResource(cached.resource, m_images[imageIndex].blob, shapeIndex))
        {
            m_cachedImageShapes.PopBack();
            return false;
        }

        out = &cached.resource;
        return true;
    }

    void ScribeTestApp::QueueLayout(const scribe::LayoutResult& layout, const Vec2f& origin, float fontSize, float layoutScale)
    {
        Vector<scribe::CompiledColorGlyphLayer> colorLayers{};
        const Vec4f foregroundColor{ 0.0f, 0.0f, 0.0f, 1.0f };

        for (const scribe::ShapedGlyph& glyph : layout.glyphs)
        {
            if (glyph.fontFaceIndex >= m_fonts.Size())
            {
                continue;
            }

            const LoadedDemoFont& font = m_fonts[glyph.fontFaceIndex];
            const uint32_t unitsPerEm = Max(font.blob.metadata.GetMetrics().GetUnitsPerEm(), 1u);
            const float scale = (fontSize / static_cast<float>(unitsPerEm)) * layoutScale;
            const Vec2f position{
                origin.x + (glyph.position.x * layoutScale),
                origin.y + (glyph.position.y * layoutScale)
            };
            const uint32_t paletteIndex = scribe::SelectCompiledFontPalette(font.blob, true);
            const bool hasResolvedLayers = scribe::GetCompiledColorGlyphLayers(
                colorLayers,
                font.blob,
                glyph.glyphIndex,
                paletteIndex,
                foregroundColor);

            if (hasResolvedLayers && !colorLayers.IsEmpty())
            {
                for (const scribe::CompiledColorGlyphLayer& layer : colorLayers)
                {
                    const scribe::GlyphResource* glyphResource = nullptr;
                    if (!EnsureGlyphResource(glyph.fontFaceIndex, layer.glyphIndex, glyphResource))
                    {
                        continue;
                    }

                    scribe::DrawGlyphDesc desc{};
                    desc.glyph = glyphResource;
                    desc.position = position;
                    desc.size = { scale, scale };
                    desc.color = layer.color;
                    desc.basisX = layer.basisX;
                    desc.basisY = layer.basisY;
                    desc.offset = layer.offset;
                    m_renderer.QueueDraw(desc);
                }
                continue;
            }

            const scribe::GlyphResource* glyphResource = nullptr;
            if (!EnsureGlyphResource(glyph.fontFaceIndex, glyph.glyphIndex, glyphResource))
            {
                continue;
            }

            scribe::DrawGlyphDesc desc{};
            desc.glyph = glyphResource;
            desc.position = position;
            desc.size = { scale, scale };
            desc.color = foregroundColor;
            m_renderer.QueueDraw(desc);
        }
    }

    void ScribeTestApp::QueueImage(const LoadedDemoImage& image, uint32_t imageIndex, const Vec2f& position, float scale)
    {
        Vector<scribe::CompiledVectorImageLayer> layers{};
        if (!scribe::GetCompiledVectorImageLayers(layers, image.blob))
        {
            return;
        }

        const float minX = image.blob.metadata.GetSourceViewBoxMinX();
        const float minY = image.blob.metadata.GetSourceViewBoxMinY();
        const Vec2f drawOffset{ -minX, minY };

        for (const scribe::CompiledVectorImageLayer& layer : layers)
        {
            const scribe::GlyphResource* shapeResource = nullptr;
            if (!EnsureImageShapeResource(imageIndex, layer.shapeIndex, shapeResource))
            {
                continue;
            }

            scribe::DrawGlyphDesc desc{};
            desc.glyph = shapeResource;
            desc.position = position;
            desc.size = { scale, scale };
            desc.offset = drawOffset;
            desc.color = layer.color;
            m_renderer.QueueDraw(desc);
        }
    }

    void ScribeTestApp::QueueCaret()
    {
        if (m_scene == DemoScene::SvgVectorImages)
        {
            return;
        }

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
        desc.color = { 0.0f, 0.0f, 0.0f, 1.0f };
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
                    "the baseline paragraph demo for layout width, line metrics, and dense glyph submission.\n\n"
                    "Artifact stress: W Y / WY/WY ///";
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

            case DemoScene::ColorGlyphLayers:
                if (HasColorDemoFont())
                {
                    m_titleText = "Scribe Testbed: COLR/CPAL layered color glyphs";
                    m_bodyText =
                        "Color font fallback should route these glyphs through explicit palette layers:\n\n"
                        "\xF0\x9F\x99\x82 \xF0\x9F\x98\x80 \xF0\x9F\x8E\xA8 \xF0\x9F\x8C\x88 \xE2\x9C\xA8";
                }
                else
                {
                    m_titleText = "Scribe Testbed: color glyph fallback unavailable";
                    m_bodyText =
                        "This machine does not have the optional Segoe UI Emoji font available at the "
                        "expected system path, so the COLR/CPAL demo scene cannot be exercised here.";
                }
                break;

            case DemoScene::SvgVectorImages:
                m_titleText = "Scribe Testbed: compiled SVG vector scenes";
                m_bodyText =
                    "These SVG files are loaded from source, compiled in memory into Scribe curve and band "
                    "payloads, and rendered through the same coverage path as text. Left image stresses layered "
                    "fills and transforms. Right image stresses even-odd filling.";
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

    void ScribeTestApp::ResetSceneView()
    {
        m_scenePan = { 0.0f, 0.0f };
        m_sceneZoom = 1.0f;
        m_isPanning = false;
        m_hasCaret = false;
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
        ResetSceneView();
        m_layoutDirty = true;
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

        if (frame.hasSubmittedWork)
        {
            frame.lastGpuMs = ToPeriod<Seconds, float>(MonotonicClock::Now() - frame.submitTime) * 1000.0f;
            m_lastGpuFrameMs = frame.lastGpuMs;
            frame.hasSubmittedWork = false;
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
        m_render.frames[m_render.frameIndex].submitTime = MonotonicClock::Now();
        m_render.frames[m_render.frameIndex].hasSubmittedWork = true;
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

    bool ScribeTestApp::HasColorDemoFont() const
    {
        for (const LoadedDemoFont& font : m_fonts)
        {
            if (font.blob.metadata.IsValid()
                && font.blob.metadata.GetHasColorGlyphs()
                && font.blob.paint.IsValid()
                && !font.blob.paint.GetPalettes().IsEmpty())
            {
                return true;
            }
        }

        return false;
    }
}
