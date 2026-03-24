// Copyright Chad Engler

#include "scribe_test_app.h"

#include "font_compile_geometry.h"
#include "font_import_utils.h"
#include "image_compile_geometry.h"
#include "resource_build_utils.h"

#include "he/scribe/compiled_font.h"
#include "he/scribe/compiled_vector_image.h"
#include "he/scribe/schema_types.h"

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

#include <cstring>

namespace he
{
    namespace
    {
        constexpr uint32_t GpuGraphTickCount = 5;
        constexpr float GpuGraphMaxMs = 2.0f;
        constexpr const char* TestIconAccount = "\xf3\xb0\x80\x84";
        constexpr const char* RtlSample = "\xd9\x85\xd8\xb1\xd8\xad\xd8\xa8\xd8\xa7 \xd8\xa8\xd8\xa7\xd9\x84\xd8\xb9\xd8\xa7\xd9\x84\xd9\x85";

#include "emoji_page_data.inl"

        struct FeatureRowSpec
        {
            const char* title{ nullptr };
            const char* description{ nullptr };
            const char* disabledSample{ nullptr };
            const char* enabledSample{ nullptr };
            scribe::TextDirection disabledDirection{ scribe::TextDirection::Auto };
            scribe::TextDirection enabledDirection{ scribe::TextDirection::Auto };
        };

        constexpr FeatureRowSpec FeatureRows[] =
        {
            {
                "Font styles",
                "Styled runs can switch faces per span, which lets the testbed compare plain text against italic, bold, and monospace code spans.",
                "Regular italic bold {code}",
                "Regular italic bold {code}",
            },
            {
                "Stretch and skew",
                "Retained text applies per-run stretch and skew after shaping so the same layout can be rendered with character-level transforms.",
                "Stretched text\nSkewed text",
                "Stretched text\nSkewed text",
            },
            {
                "Text decorations",
                "Underline and strike-through decorations are emitted as retained quads so they follow the shaped cluster geometry.",
                "Underline\nStrike-through",
                "Underline\nStrike-through",
            },
            {
                "Tracking",
                "Run-level tracking adjusts cluster advances after shaping, which makes loose and tight spacing easy to compare side by side.",
                "Tight tracking\nLoose tracking",
                "Tight tracking\nLoose tracking",
            },
            {
                "Multicolor emoji",
                "COLR/CPAL glyph layers already render through the compiled font and runtime path.",
                "Toggle-off sample not exposed in the testbed.",
                "🙂 😀 🎨 🌈 ✨",
            },
            {
                "Skin tone modifiers",
                "Emoji modifier sequences should shape and layer correctly when the color emoji fallback face is available.",
                "👋 🧑‍⚕️ fallback dependent.",
                "👋🏻 👋🏽 👋🏿 🧑🏽‍⚕️",
            },
            {
                "Kerning",
                "OpenType feature spans can disable kerning for a comparison against the default shaped result.",
                "AVATAR WAVE",
                "AVATAR WAVE",
            },
            {
                "Ligatures",
                "Ligature features are routed through HarfBuzz so the testbed can compare default shaping against ligatures disabled.",
                "office official afflict",
                "office official afflict",
            },
            {
                "Combining marks",
                "Combining marks and cluster preservation are already working in the shaping path.",
                "Slug decomposed cluster stress.",
                "Şl̈ūg̊",
            },
            {
                "Small caps",
                "Small-cap substitutions are enabled through OpenType feature spans when the chosen demo font supports them.",
                "Harvest Engine",
                "Harvest Engine",
            },
            {
                "Stylistic variants",
                "Style-set alternates are exposed as explicit OpenType feature toggles so the row can compare default and ss01 forms.",
                "arial lqty69",
                "arial lqty69",
            },
            {
                "Case-sensitive forms",
                "Case-sensitive punctuation substitutions are applied through OpenType feature spans on the enabled sample.",
                "[(ALL-CAPS)]",
                "[(ALL-CAPS)]",
            },
            {
                "Subscripts and superscripts",
                "Native subscript and superscript substitutions are enabled per run where the demo font supports them.",
                "CH3CH2CH3\nFootnote5",
                "CH3CH2CH3\nFootnote5",
            },
            {
                "Script transforms",
                "Per-run baseline shifts and glyph scaling provide script-style transforms even when the font does not supply dedicated OpenType substitutions.",
                "TextSub1 Sub2\nTextSup1 Sup2",
                "TextSub1 Sub2\nTextSup1 Sup2",
            },
            {
                "Ordinals and fractions",
                "Ordinal and fraction substitutions are now driven through OpenType feature spans on the enabled sample.",
                "1st 2nd 3rd 4th\n123/456",
                "1st 2nd 3rd 4th\n123/456",
            },
            {
                "Figure styles",
                "Old-style, lining, proportional, and tabular figures are now selectable through OpenType feature spans.",
                "0123456789\n0123456789",
                "0123456789\n0123456789",
            },
            {
                "Unicode support",
                "The runtime already handles mixed-script shaping and fallback across a broad set of Unicode text.",
                "Σύνθετη απόδοση γραμματοσειράς και διάταξη κειμένου",
                "Улучшенный отрисовщик шрифтов и макет текста / 高级字体渲染和文本布局",
            },
            {
                "Right-to-left languages",
                "The first-pass RTL layout path is working, but full bidi and line-breaking behavior is still a later milestone.",
                "LTR stress sample only.",
                "مرحبا بالعالم / עיבוד גופן מתקדם",
                scribe::TextDirection::LeftToRight,
                scribe::TextDirection::RightToLeft,
            },
            {
                "Cursive joining",
                "Arabic joining behavior depends on shaping and fallback-font coverage. The row stays visible as a shaping checkpoint.",
                "Isolated forms only if fallback is missing.",
                "تقديم الخط",
                scribe::TextDirection::RightToLeft,
                scribe::TextDirection::RightToLeft,
            },
            {
                "Bidirectional layout",
                "Mixed-direction runs are partially supported today and need broader bidi coverage later.",
                "Bezier نقاط التحكم with limited bidi.",
                "GPU text مع English 123 layout stress.",
                scribe::TextDirection::Auto,
                scribe::TextDirection::Auto,
            },
            {
                "Glyph effects",
                "Retained text can emit shadow and outline draws per glyph, using the same compiled glyph resources as the base fill.",
                "Shadow Outline Both",
                "Shadow Outline Both",
            },
            {
                "Per-glyph transforms",
                "Style spans can target individual glyphs, which lets the testbed bend a line onto a curved baseline after shaping.",
                "Text laid out on a curve",
                "Text laid out on a curve",
            },
            {
                "Vector strokes and gradients",
                "These are retained vector features rather than text features. They remain listed here as a bridge to the SVG scene.",
                "See SVG scene",
                "See SVG scene",
            },
        };

        bool TryGetRenderedLineMinX(
            float& outMinX,
            const scribe::RetainedTextModel& text,
            const scribe::LayoutResult& layout,
            uint32_t lineIndex)
        {
            outMinX = Limits<float>::Max;

            const Span<const scribe::RetainedTextDraw> draws = text.GetDraws();
            if (draws.IsEmpty() || layout.glyphs.IsEmpty())
            {
                return false;
            }

            const uint32_t count = Min(draws.Size(), layout.glyphs.Size());
            for (uint32_t glyphLayoutIndex = 0; glyphLayoutIndex < count; ++glyphLayoutIndex)
            {
                const scribe::ShapedGlyph& glyph = layout.glyphs[glyphLayoutIndex];
                if (glyph.lineIndex != lineIndex)
                {
                    continue;
                }

                const scribe::RetainedTextDraw& draw = draws[glyphLayoutIndex];
                const scribe::FontFaceResourceReader* fontFace = text.GetFontFace(draw.fontFaceIndex);
                if (!fontFace)
                {
                    continue;
                }

                const scribe::FontFaceRenderData::Reader render = fontFace->GetRender();
                if (!render.IsValid())
                {
                    continue;
                }

                const schema::List<scribe::FontFaceGlyphRenderData>::Reader glyphs = render.GetGlyphs();
                if (draw.glyphIndex >= glyphs.Size())
                {
                    continue;
                }

                const scribe::FontFaceGlyphRenderData::Reader compiledGlyph = glyphs[draw.glyphIndex];
                if (!compiledGlyph.IsValid() || !compiledGlyph.GetHasGeometry())
                {
                    outMinX = Min(outMinX, draw.position.x);
                    continue;
                }

                const float minX = compiledGlyph.GetBoundsMinX();
                const float maxX = Max(compiledGlyph.GetBoundsMaxX(), minX + 1.0f);
                const float minY = compiledGlyph.GetBoundsMinY();
                const float maxY = Max(compiledGlyph.GetBoundsMaxY(), minY + 1.0f);
                const float objectMinY = -maxY;
                const float objectMaxY = -minY;

                const float a00 = draw.size.x * draw.basisX.x;
                const float a01 = draw.size.x * draw.basisY.x;
                const float offsetX = draw.position.x + (draw.size.x * draw.offset.x);

                auto transformedX = [&](float x, float y)
                {
                    return offsetX + (a00 * x) + (a01 * y);
                };

                outMinX = Min(
                    outMinX,
                    transformedX(minX, objectMinY),
                    transformedX(maxX, objectMinY),
                    transformedX(maxX, objectMaxY),
                    transformedX(minX, objectMaxY));
            }

            return outMinX != Limits<float>::Max;
        }

        float ComputeCapAlignedFontSize(const scribe::FontFaceResourceReader& font, float capHeightPixels)
        {
            const scribe::FontFaceImportMetadata::Reader metadata = font.GetMetadata();
            if (!metadata.IsValid())
            {
                return capHeightPixels;
            }

            const scribe::FontFaceMetrics::Reader metrics = metadata.GetMetrics();
            const float unitsPerEm = static_cast<float>(Max(metrics.GetUnitsPerEm(), 1u));
            const float capHeightUnits = static_cast<float>(Max(Abs(metrics.GetCapHeight()), 1));
            return capHeightPixels * (unitsPerEm / capHeightUnits);
        }

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

        bool FindSubstringRange(
            StringView text,
            StringView needle,
            uint32_t occurrence,
            uint32_t& outStart,
            uint32_t& outEnd)
        {
            if (needle.IsEmpty() || (needle.Size() > text.Size()))
            {
                return false;
            }

            const char* search = text.Data();
            const char* searchEnd = text.Data() + text.Size();
            for (uint32_t i = 0; i <= occurrence; ++i)
            {
                const char* match = std::strstr(search, needle.Data());
                if (!match || (match >= searchEnd))
                {
                    return false;
                }

                if (i == occurrence)
                {
                    outStart = static_cast<uint32_t>(match - text.Data());
                    outEnd = outStart + needle.Size();
                    return true;
                }

                search = match + 1;
            }

            return false;
        }

        bool AddStyleSpanForSubstring(
            Vector<scribe::TextStyleSpan>& spans,
            StringView text,
            StringView needle,
            uint32_t styleIndex,
            uint32_t occurrence = 0)
        {
            uint32_t start = 0;
            uint32_t end = 0;
            if (!FindSubstringRange(text, needle, occurrence, start, end))
            {
                return false;
            }

            scribe::TextStyleSpan& span = spans.EmplaceBack();
            span.textByteStart = start;
            span.textByteEnd = end;
            span.styleIndex = styleIndex;
            return true;
        }

        bool BuildLoadedFontFaceFromFile(const char* fileName, Vector<schema::Word>& storage, scribe::FontFaceResourceReader& out)
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

            schema::Builder rootBuilder;
            scribe::FontFaceResource::Builder root = rootBuilder.AddStruct<scribe::FontFaceResource>();
            scribe::FontFaceShapingData::Builder shaping = root.InitShaping();
            shaping.SetFaceIndex(faceInfo.faceIndex);
            shaping.SetSourceFormat(faceInfo.sourceFormat);
            shaping.SetSourceBytes(rootBuilder.AddBlob(Span<const uint8_t>(fontBytes)));

            scribe::FontFaceImportMetadata::Builder metadata = root.InitMetadata();
            scribe::editor::FillFontFaceImportMetadata(metadata, faceInfo);

            scribe::editor::FillFontFaceResourceRenderData(root.InitRender(), renderData);
            scribe::editor::FillFontFaceResourcePaintData(root.InitPaint(), renderData.paint);
            root.SetCurveData(rootBuilder.AddBlob(Span<const scribe::PackedCurveTexel>(renderData.curveTexels.Data(), renderData.curveTexels.Size()).AsBytes()));
            root.SetBandData(rootBuilder.AddBlob(Span<const scribe::PackedBandTexel>(renderData.bandTexels.Data(), renderData.bandTexels.Size()).AsBytes()));
            rootBuilder.SetRoot(root);

            storage = Span<const schema::Word>(rootBuilder);
            out = schema::ReadRoot<scribe::FontFaceResource>(storage.Data());
            return out.IsValid();
        }

        bool BuildLoadedVectorImageFromFile(const char* fileName, Vector<schema::Word>& storage, scribe::VectorImageResourceReader& out)
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

            schema::Builder rootBuilder;
            scribe::VectorImageResource::Builder root = rootBuilder.AddStruct<scribe::VectorImageResource>();
            scribe::editor::FillVectorImageResourceMetadata(root.InitMetadata(), imageData);
            scribe::editor::FillVectorImageResourceRenderData(root.InitRender(), imageData);
            scribe::editor::FillVectorImageResourcePaintData(root.InitPaint(), imageData);
            root.SetCurveData(rootBuilder.AddBlob(Span<const scribe::PackedCurveTexel>(imageData.curveTexels.Data(), imageData.curveTexels.Size()).AsBytes()));
            root.SetBandData(rootBuilder.AddBlob(Span<const scribe::PackedBandTexel>(imageData.bandTexels.Data(), imageData.bandTexels.Size()).AsBytes()));
            rootBuilder.SetRoot(root);

            storage = Span<const schema::Word>(rootBuilder);
            out = schema::ReadRoot<scribe::VectorImageResource>(storage.Data());
            return out.IsValid();
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
                const window::ViewResizedEvent& evt = static_cast<const window::ViewResizedEvent&>(ev);
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
                const window::PointerMoveEvent& evt = static_cast<const window::PointerMoveEvent&>(ev);
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
                const window::PointerDownEvent& evt = static_cast<const window::PointerDownEvent&>(ev);
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
                const window::PointerUpEvent& evt = static_cast<const window::PointerUpEvent&>(ev);
                if (evt.button == window::PointerButton::Primary)
                {
                    m_isPanning = false;
                }
                break;
            }

            case window::EventKind::PointerWheel:
            {
                const window::PointerWheelEvent& evt = static_cast<const window::PointerWheelEvent&>(ev);
                const float wheelDelta = evt.delta.y;
                if (Abs(wheelDelta) > 0.0f)
                {
                    const float zoomFactor = Pow(1.1f, wheelDelta);
                    const float oldZoom = m_sceneZoom;
                    const float newZoom = Clamp(oldZoom * zoomFactor, 0.1f, 500.0f);
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
                const window::KeyDownEvent& evt = static_cast<const window::KeyDownEvent&>(ev);
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

                    case window::Key::B:
                        m_glyphBatchingEnabled = !m_glyphBatchingEnabled;
                        m_renderer.SetGlyphBatchingEnabled(m_glyphBatchingEnabled);
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

        const auto ApplySceneTransform = [this](const Vec2f& point) -> Vec2f
        {
            return {
                (point.x * m_sceneZoom) + m_scenePan.x,
                (point.y * m_sceneZoom) + m_scenePan.y
            };
        };
        Vec2f transformedTitleOrigin = ApplySceneTransform(m_titleOrigin);
        Vec2f transformedBodyOrigin = ApplySceneTransform(m_bodyOrigin);
        auto AlignTextOriginLeftEdgeX = [this](Vec2f& origin, const scribe::RetainedTextModel& text, const scribe::LayoutResult& layout)
        {
            float leftEdgeX = Limits<float>::Max;
            float renderedMinX = 0.0f;
            if (TryGetRenderedLineMinX(renderedMinX, text, layout, 0))
            {
                leftEdgeX = origin.x + (renderedMinX * m_sceneZoom);
            }
            else
            {
                for (const scribe::TextCluster& cluster : layout.clusters)
                {
                    if (cluster.isWhitespace || (cluster.lineIndex != 0))
                    {
                        continue;
                    }

                    leftEdgeX = Min(leftEdgeX, origin.x + (cluster.x0 * m_sceneZoom));
                }
            }

            if (leftEdgeX != Limits<float>::Max)
            {
                origin.x += Round(leftEdgeX) - leftEdgeX;
            }
            else
            {
                origin.x = Round(origin.x);
            }
        };
        AlignTextOriginLeftEdgeX(transformedTitleOrigin, m_retainedTitleText, m_titleLayout);
        AlignTextOriginLeftEdgeX(transformedBodyOrigin, m_retainedBodyText, m_bodyLayout);

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
        frameDesc.gpuTimer.querySet = m_render.frames[m_render.frameIndex].gpuTimerQueries;
        frameDesc.gpuTimer.resolveBuffer = m_render.frames[m_render.frameIndex].gpuTimerReadback;

        if (!m_renderer.BeginFrame(frameDesc))
        {
            EndFrame();
            return;
        }

        const uint32_t reservedVertexCount = m_sceneVertexEstimate
            + m_overlayVertexEstimate
            + (m_hasCaret ? m_caretGlyph.vertexCount : 0);
        m_renderer.ReserveQueuedVertexCapacity(reservedVertexCount);

        if (!m_retainedTitleText.IsEmpty())
        {
            scribe::RetainedTextInstanceDesc instance{};
            instance.origin = transformedTitleOrigin;
            instance.scale = m_sceneZoom;
            instance.foregroundColor = { 0.0f, 0.0f, 0.0f, 1.0f };
            m_renderer.QueueRetainedText(m_retainedTitleText, instance);
        }

        if (!m_retainedBodyText.IsEmpty())
        {
            scribe::RetainedTextInstanceDesc instance{};
            instance.origin = transformedBodyOrigin;
            instance.scale = m_sceneZoom;
            instance.foregroundColor = { 0.0f, 0.0f, 0.0f, 1.0f };
            m_renderer.QueueRetainedText(m_retainedBodyText, instance);
        }

        for (const SceneTextBlock& block : m_sceneBlocks)
        {
            if (block.retainedText.IsEmpty())
            {
                continue;
            }

            scribe::RetainedTextInstanceDesc instance{};
            instance.origin = GetSceneBlockRenderOrigin(block);
            instance.scale = m_sceneZoom;
            instance.foregroundColor = block.color;
            m_renderer.QueueRetainedText(block.retainedText, instance);
        }

        for (const SceneVectorImageBlock& block : m_sceneImages)
        {
            if (block.retainedImage.IsEmpty())
            {
                continue;
            }

            scribe::RetainedVectorImageInstanceDesc instance{};
            instance.origin = GetSceneImageRenderOrigin(block);
            instance.scale = block.scale * m_sceneZoom;
            m_renderer.QueueRetainedVectorImage(block.retainedImage, instance);
        }

        if (!m_retainedSceneStatsText.IsEmpty())
        {
            scribe::RetainedTextInstanceDesc instance{};
            instance.origin = m_sceneStatsOrigin;
            instance.foregroundColor = { 0.0f, 0.0f, 0.0f, 1.0f };
            m_renderer.QueueRetainedText(m_retainedSceneStatsText, instance);
        }

        if (!m_retainedRenderStatsText.IsEmpty())
        {
            scribe::RetainedTextInstanceDesc instance{};
            instance.origin = m_renderStatsOrigin;
            instance.foregroundColor = { 0.0f, 0.0f, 0.0f, 1.0f };
            m_renderer.QueueRetainedText(m_retainedRenderStatsText, instance);
        }

        if (!m_retainedInputHintsText.IsEmpty())
        {
            scribe::RetainedTextInstanceDesc instance{};
            instance.origin = m_inputHintsOrigin;
            instance.foregroundColor = { 0.0f, 0.0f, 0.0f, 1.0f };
            m_renderer.QueueRetainedText(m_retainedInputHintsText, instance);
        }

        for (uint32_t i = 0; i < GpuGraphTickCount; ++i)
        {
            if (!m_retainedGpuGraphLabels[i].IsEmpty())
            {
                scribe::RetainedTextInstanceDesc instance{};
                instance.origin = m_gpuGraphLabelOrigins[i];
                instance.foregroundColor = { 0.35f, 0.35f, 0.35f, 1.0f };
                m_renderer.QueueRetainedText(m_retainedGpuGraphLabels[i], instance);
            }
        }

        if ((m_gpuGraphSize.x > 0.0f) && (m_gpuGraphSize.y > 0.0f) && !m_gpuFrameHistory.IsEmpty())
        {
            auto queueOverlayQuad = [this](float x, float y, float width, float height, const Vec4f& color)
            {
                scribe::DrawQuadDesc desc{};
                desc.position = { x, y };
                desc.size = { width, height };
                desc.color = color;
                m_renderer.QueueQuad(desc);
            };

            const float graphX = m_gpuGraphOrigin.x;
            const float graphY = m_gpuGraphOrigin.y;
            const float graphW = m_gpuGraphSize.x;
            const float graphH = m_gpuGraphSize.y;
            const float border = 1.0f;

            queueOverlayQuad(graphX, graphY, graphW, graphH, { 0.97f, 0.97f, 0.97f, 1.0f });
            queueOverlayQuad(graphX, graphY, graphW, border, { 0.78f, 0.78f, 0.78f, 1.0f });
            queueOverlayQuad(graphX, graphY + graphH - border, graphW, border, { 0.78f, 0.78f, 0.78f, 1.0f });
            queueOverlayQuad(graphX, graphY, border, graphH, { 0.78f, 0.78f, 0.78f, 1.0f });
            queueOverlayQuad(graphX + graphW - border, graphY, border, graphH, { 0.78f, 0.78f, 0.78f, 1.0f });

            for (uint32_t i = 1; i < GpuGraphTickCount - 1; ++i)
            {
                const float t = static_cast<float>(i) / static_cast<float>(GpuGraphTickCount - 1);
                const float y = graphY + (graphH * t);
                queueOverlayQuad(graphX + border, y, graphW - (border * 2.0f), 1.0f, { 0.88f, 0.88f, 0.88f, 1.0f });
            }

            const uint32_t sampleCount = m_gpuFrameHistoryFilled ? m_gpuFrameHistory.Size() : m_gpuFrameHistoryHead;

            if (sampleCount >= 2)
            {
                const float innerX = graphX + border + 1.0f;
                const float innerY = graphY + border + 1.0f;
                const float innerW = Max(graphW - ((border + 1.0f) * 2.0f), 1.0f);
                const float innerH = Max(graphH - ((border + 1.0f) * 2.0f), 1.0f);
                const uint32_t historySize = m_gpuFrameHistory.Size();
                const uint32_t startIndex = m_gpuFrameHistoryFilled ? m_gpuFrameHistoryHead : 0u;
                const float thickness = 2.0f;

                auto getSample = [&](uint32_t sampleIndex)
                {
                    return m_gpuFrameHistory[(startIndex + sampleIndex) % historySize];
                };

                auto sampleY = [&](float sampleMs)
                {
                    return innerY + innerH - (innerH * Clamp(sampleMs / GpuGraphMaxMs, 0.0f, 1.0f));
                };

                for (uint32_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
                {
                    const float x = innerX + (innerW * (static_cast<float>(sampleIndex) / static_cast<float>(sampleCount - 1)));
                    const float y = sampleY(getSample(sampleIndex));
                    queueOverlayQuad(x - 1.0f, y - 1.0f, 2.0f, 2.0f, { 0.10f, 0.45f, 0.90f, 1.0f });
                }

                for (uint32_t sampleIndex = 1; sampleIndex < sampleCount; ++sampleIndex)
                {
                    const float x0 = innerX + (innerW * (static_cast<float>(sampleIndex - 1) / static_cast<float>(sampleCount - 1)));
                    const float x1 = innerX + (innerW * (static_cast<float>(sampleIndex) / static_cast<float>(sampleCount - 1)));
                    const float y0 = sampleY(getSample(sampleIndex - 1));
                    const float y1 = sampleY(getSample(sampleIndex));
                    const float dx = x1 - x0;
                    const float dy = y1 - y0;
                    const float len = Sqrt((dx * dx) + (dy * dy));
                    if (len <= 1.0e-4f)
                    {
                        continue;
                    }

                    const float invLen = 1.0f / len;
                    const float nx = -dy * invLen;
                    const float ny = dx * invLen;

                    scribe::DrawQuadDesc desc{};
                    desc.position = { x0 - (nx * (thickness * 0.5f)), y0 - (ny * (thickness * 0.5f)) };
                    desc.size = { len, thickness };
                    desc.color = { 0.10f, 0.45f, 0.90f, 1.0f };
                    desc.basisX = { dx * invLen, dy * invLen };
                    desc.basisY = { nx, ny };
                    m_renderer.QueueQuad(desc);
                }
            }
        }
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

        m_renderer.SetGlyphBatchingEnabled(m_glyphBatchingEnabled);
        m_gpuFrameHistory.Resize(120, 0.0f);
        m_gpuFrameHistoryHead = 0;
        m_gpuFrameHistoryFilled = false;
        m_smoothedGpuFrameMs = 0.0f;

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
        m_retainedTitleText.Clear();
        m_retainedBodyText.Clear();
        m_sceneBlocks.Clear();
        m_sceneImages.Clear();
        m_sceneStatsLayout.Clear();
        m_renderStatsLayout.Clear();
        m_inputHintsLayout.Clear();
        for (uint32_t i = 0; i < GpuGraphTickCount; ++i)
        {
            m_gpuGraphLabelLayouts[i].Clear();
            m_retainedGpuGraphLabels[i].Clear();
        }
        m_retainedSceneStatsText.Clear();
        m_retainedRenderStatsText.Clear();
        m_retainedInputHintsText.Clear();
        m_svgLoadErrors.Clear();
        m_gpuFrameHistory.Clear();
        m_gpuFrameHistoryHead = 0;
        m_gpuFrameHistoryFilled = false;
        m_smoothedGpuFrameMs = 0.0f;
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
            desc.enableDebugCpu = false;
            desc.enableDebugGpu = false;
            desc.enableDebugBreakOnError = false;
            desc.enableDebugBreakOnWarning = false;

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
        m_render.renderQueueTimestampFrequency = cmdQueue.GetTimestampFrequency();
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

            rhi::TimestampQuerySetDesc queryDesc{};
            queryDesc.type = rhi::CmdListType::Render;
            queryDesc.count = 2;
            HE_RHI_SET_NAME(queryDesc, "Scribe Testbed GPU Timer Queries");

            r = m_render.device->CreateTimestampQuerySet(queryDesc, frame.gpuTimerQueries);
            if (!r)
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Failed to create scribe testbed GPU timer queries."),
                    HE_KV(result, r));
                return false;
            }

            rhi::BufferDesc readbackDesc{};
            readbackDesc.heapType = rhi::HeapType::Readback;
            readbackDesc.usage = rhi::BufferUsage::CopyDst;
            readbackDesc.size = sizeof(uint64_t) * 2;
            readbackDesc.stride = sizeof(uint64_t);
            HE_RHI_SET_NAME(readbackDesc, "Scribe Testbed GPU Timer Readback");

            r = m_render.device->CreateBuffer(readbackDesc, frame.gpuTimerReadback);
            if (!r)
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Failed to create scribe testbed GPU timer readback buffer."),
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
            m_render.device->SafeDestroy(frame.gpuTimerReadback);
            m_render.device->SafeDestroy(frame.gpuTimerQueries);
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
        m_uiFontIndex = 0;
        m_iconFontIndex = 1;
        m_rtlFontIndex = InvalidIndex;
        m_colorFontIndex = InvalidIndex;
        m_sansRegularFontIndex = InvalidIndex;
        m_sansBoldFontIndex = InvalidIndex;
        m_sansItalicFontIndex = InvalidIndex;
        m_sansBoldItalicFontIndex = InvalidIndex;
        m_monoFontIndex = InvalidIndex;
        m_serifRegularFontIndex = InvalidIndex;
        m_serifBoldFontIndex = InvalidIndex;
        m_serifItalicFontIndex = InvalidIndex;
        m_serifBoldItalicFontIndex = InvalidIndex;
        m_featureFontIndex = InvalidIndex;
        m_symbolFontIndex = InvalidIndex;

        auto loadRequiredFont = [&](uint32_t& outIndex, const char* fileName) -> bool
        {
            outIndex = m_fonts.Size();
            LoadedDemoFont& font = m_fonts.EmplaceBack();
            if (!LoadDemoFont(font, fileName))
            {
                m_fonts.Resize(outIndex);
                outIndex = InvalidIndex;
                return false;
            }

            return true;
        };

        auto loadOptionalFont = [&](uint32_t& outIndex, Span<const char*> candidates) -> bool
        {
            outIndex = m_fonts.Size();
            LoadedDemoFont& font = m_fonts.EmplaceBack();
            if (!LoadOptionalDemoFont(font, candidates))
            {
                m_fonts.Resize(outIndex);
                outIndex = InvalidIndex;
                return false;
            }

            return true;
        };

        if (!loadRequiredFont(m_uiFontIndex, "NotoSans-Regular.ttf")
            || !loadRequiredFont(m_iconFontIndex, "materialdesignicons.ttf"))
        {
            return false;
        }

        static const char* SansRegularCandidates[] =
        {
            "C:/Windows/Fonts/segoeui.ttf",
            "C:/Windows/Fonts/calibri.ttf",
            "C:/Windows/Fonts/Candara.ttf",
        };
        static const char* SansBoldCandidates[] =
        {
            "C:/Windows/Fonts/segoeuib.ttf",
            "C:/Windows/Fonts/calibrib.ttf",
            "C:/Windows/Fonts/Candarab.ttf",
        };
        static const char* SansItalicCandidates[] =
        {
            "C:/Windows/Fonts/segoeuii.ttf",
            "C:/Windows/Fonts/calibrii.ttf",
            "C:/Windows/Fonts/Candarai.ttf",
        };
        static const char* SansBoldItalicCandidates[] =
        {
            "C:/Windows/Fonts/segoeuiz.ttf",
            "C:/Windows/Fonts/calibriz.ttf",
            "C:/Windows/Fonts/Candaraz.ttf",
        };
        static const char* MonoCandidates[] =
        {
            "plugins/editor/src/fonts/NotoMono-Regular.ttf",
            "C:/Windows/Fonts/consola.ttf",
        };
        static const char* SerifRegularCandidates[] =
        {
            "C:/Windows/Fonts/cambria.ttc",
            "C:/Windows/Fonts/times.ttf",
            "C:/Windows/Fonts/georgia.ttf",
            "C:/Windows/Fonts/calibri.ttf",
        };
        static const char* SerifBoldCandidates[] =
        {
            "C:/Windows/Fonts/timesbd.ttf",
            "C:/Windows/Fonts/georgiab.ttf",
            "C:/Windows/Fonts/calibrib.ttf",
        };
        static const char* SerifItalicCandidates[] =
        {
            "C:/Windows/Fonts/timesi.ttf",
            "C:/Windows/Fonts/georgiai.ttf",
            "C:/Windows/Fonts/calibrii.ttf",
        };
        static const char* SerifBoldItalicCandidates[] =
        {
            "C:/Windows/Fonts/timesbi.ttf",
            "C:/Windows/Fonts/georgiaz.ttf",
            "C:/Windows/Fonts/calibriz.ttf",
        };
        static const char* FeatureFontCandidates[] =
        {
            "C:/Windows/Fonts/cambria.ttc",
            "C:/Windows/Fonts/calibri.ttf",
            "C:/Windows/Fonts/Candara.ttf",
            "C:/Windows/Fonts/SitkaVF.ttf",
            "C:/Windows/Fonts/Gabriola.ttf",
        };
        static const char* RtlFontCandidates[] =
        {
            "C:/Windows/Fonts/segoeui.ttf",
            "C:/Windows/Fonts/arial.ttf",
            "C:/Windows/Fonts/tahoma.ttf",
        };
        static const char* ColorFontCandidates[] =
        {
            "C:/Windows/Fonts/seguiemj.ttf",
        };
        static const char* SymbolFontCandidates[] =
        {
            "C:/Windows/Fonts/seguisym.ttf",
            "C:/Windows/Fonts/SegoeIcons.ttf",
        };

        loadOptionalFont(m_sansRegularFontIndex, SansRegularCandidates);
        loadOptionalFont(m_sansBoldFontIndex, SansBoldCandidates);
        loadOptionalFont(m_sansItalicFontIndex, SansItalicCandidates);
        loadOptionalFont(m_sansBoldItalicFontIndex, SansBoldItalicCandidates);
        loadOptionalFont(m_monoFontIndex, MonoCandidates);
        loadOptionalFont(m_serifRegularFontIndex, SerifRegularCandidates);
        loadOptionalFont(m_serifBoldFontIndex, SerifBoldCandidates);
        loadOptionalFont(m_serifItalicFontIndex, SerifItalicCandidates);
        loadOptionalFont(m_serifBoldItalicFontIndex, SerifBoldItalicCandidates);
        loadOptionalFont(m_featureFontIndex, FeatureFontCandidates);
        loadOptionalFont(m_rtlFontIndex, RtlFontCandidates);
        loadOptionalFont(m_colorFontIndex, ColorFontCandidates);
        loadOptionalFont(m_symbolFontIndex, SymbolFontCandidates);

        return true;
    }

    bool ScribeTestApp::LoadDemoImages()
    {
        m_images.Clear();
        m_svgLoadErrors.Clear();
        static const char* SvgFiles[] =
        {
            "C:/Users/engle/Downloads/svg_tests/slug_algorithm.svg",
            "C:/Users/engle/Downloads/svg_tests/tiger.svg",
            "C:/Users/engle/Downloads/svg_tests/Complex_arcsin_abs_01_Pengo.svg",
        };

        for (uint32_t imageIndex = 0; imageIndex < HE_LENGTH_OF(SvgFiles); ++imageIndex)
        {
            LoadedDemoImage image{};
            if (LoadDemoImage(image, SvgFiles[imageIndex]))
            {
                m_images.EmplaceBack(Move(image));
                continue;
            }

            m_svgLoadErrors.EmplaceBack(SvgFiles[imageIndex]);
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

        Vector<scribe::FontFaceResourceReader> faces{};
        for (const LoadedDemoFont& font : m_fonts)
        {
            if (font.blob.IsValid())
            {
                faces.EmplaceBack(font.blob);
            }
        }

        if (faces.IsEmpty())
        {
            return false;
        }

        const Span<const scribe::FontFaceResourceReader> faceSpan(faces.Data(), faces.Size());
        const Span<const scribe::FontFaceResourceReader> primaryFace(faces.Data(), 1);
        const Vec2i viewSize = m_view->GetSize();
        const float dpiScale = Max(m_view->GetDpiScale(), 1.0f);
        const float margin = 40.0f * dpiScale;
        const float titleFontSize = 34.0f * dpiScale;
        float bodyFontSize = 24.0f * dpiScale;
        float contentWidth = Max(static_cast<float>(viewSize.x) - (margin * 2.0f), 128.0f);
        float bodyWidth = contentWidth;

        m_sceneBlocks.Clear();
        m_sceneImages.Clear();
        m_bodyLayout.Clear();
        m_retainedTitleText.Clear();
        m_retainedBodyText.Clear();
        m_bodyOrigin = { 0.0f, 0.0f };

        auto layoutText = [&](scribe::LayoutResult& out,
                              Span<const scribe::FontFaceResourceReader> blockFaces,
                              const String& text,
                              float fontSize,
                              float maxWidth,
                              scribe::TextDirection direction,
                              bool wrap = true,
                              Span<const scribe::TextStyle> styles = {},
                              Span<const scribe::TextStyleSpan> styleSpans = {},
                              Span<const scribe::TextFeatureSetting> features = {}) -> bool
        {
            scribe::StyledTextLayoutDesc desc{};
            desc.fontFaces = blockFaces;
            desc.text = text;
            desc.options.fontSize = fontSize;
            desc.options.wrap = wrap;
            desc.options.maxWidth = maxWidth;
            desc.options.direction = direction;
            desc.styles = styles;
            desc.styleSpans = styleSpans;
            desc.features = features;
            return m_layoutEngine.LayoutStyledText(out, desc);
        };

        auto buildRetainedText = [&](scribe::RetainedTextModel& out,
                                     Span<const scribe::FontFaceResourceReader> blockFaces,
                                     const scribe::LayoutResult& layout,
                                     float fontSize,
                                     Span<const scribe::TextStyle> styles = {}) -> bool
        {
            out.Clear();

            scribe::RetainedTextBuildDesc retainedDesc{};
            retainedDesc.fontFaces = blockFaces;
            retainedDesc.layout = &layout;
            retainedDesc.fontSize = fontSize;
            retainedDesc.darkBackgroundPreferred = true;
            retainedDesc.styles = styles;
            return out.Build(retainedDesc) && m_renderer.PrepareRetainedText(out);
        };

        auto buildRetainedImage = [&](scribe::RetainedVectorImageModel& out, const LoadedDemoImage& image) -> bool
        {
            out.Clear();

            scribe::RetainedVectorImageBuildDesc retainedDesc{};
            retainedDesc.image = &image.blob;
            return out.Build(retainedDesc) && m_renderer.PrepareRetainedVectorImage(out);
        };

        auto resolveBlockFaces = [&](const SceneTextBlock& block, Vector<scribe::FontFaceResourceReader>& out) -> bool
        {
            out.Clear();

            if (!block.faceIndices.IsEmpty())
            {
                out.Reserve(block.faceIndices.Size());
                for (uint32_t fontIndex : block.faceIndices)
                {
            if ((fontIndex < m_fonts.Size()) && m_fonts[fontIndex].blob.IsValid())
                    {
                        out.EmplaceBack(m_fonts[fontIndex].blob);
                    }
                }
            }
            else if (block.useAllFaces)
            {
                out = faceSpan;
            }
            else if ((block.fontFaceIndex < m_fonts.Size()) && m_fonts[block.fontFaceIndex].blob.IsValid())
            {
                out.EmplaceBack(m_fonts[block.fontFaceIndex].blob);
            }

            return !out.IsEmpty();
        };

        auto finalizeSceneBlock = [&](SceneTextBlock& block,
                                      float maxWidth,
                                      scribe::TextDirection direction,
                                      bool wrap = true) -> bool
        {
            Vector<scribe::FontFaceResourceReader> blockFacesStorage{};
            if (!resolveBlockFaces(block, blockFacesStorage))
            {
                return false;
            }

            const Span<const scribe::FontFaceResourceReader> blockFaces(blockFacesStorage.Data(), blockFacesStorage.Size());
            if (!layoutText(
                    block.layout,
                    blockFaces,
                    block.text,
                    block.fontSize,
                    maxWidth,
                    direction,
                    wrap,
                    block.styles,
                    block.styleSpans,
                    block.features))
            {
                return false;
            }

            return buildRetainedText(block.retainedText, blockFaces, block.layout, block.fontSize, block.styles);
        };

        auto addSceneBlock = [&](const char* text,
                                 const Vec2f& origin,
                                 float fontSize,
                                 float maxWidth,
                                 bool useAllFaces,
                                 scribe::TextDirection direction,
                                 const Vec4f& color = { 0.0f, 0.0f, 0.0f, 1.0f },
                                 uint32_t fontFaceIndex = 0,
                                 bool pixelAlignBaseline = false,
                                 bool pixelAlignCapHeight = false,
                                 bool wrap = true) -> SceneTextBlock*
        {
            const uint32_t blockIndex = m_sceneBlocks.Size();
            SceneTextBlock& block = m_sceneBlocks.EmplaceBack();
            block.text = text;
            block.origin = origin;
            block.fontSize = fontSize;
            block.color = color;
            block.fontFaceIndex = fontFaceIndex;
            block.useAllFaces = useAllFaces;
            block.pixelAlignBaseline = pixelAlignBaseline;
            block.pixelAlignCapHeight = pixelAlignCapHeight;

            if (!finalizeSceneBlock(block, maxWidth, direction, wrap))
            {
                m_sceneBlocks.Resize(blockIndex);
                return nullptr;
            }

            return &block;
        };

        auto addPreparedSceneBlock = [&](SceneTextBlock&& prototype,
                                         float maxWidth,
                                         scribe::TextDirection direction,
                                         bool wrap = true) -> SceneTextBlock*
        {
            const uint32_t blockIndex = m_sceneBlocks.Size();
            SceneTextBlock& block = m_sceneBlocks.EmplaceBack(Move(prototype));
            if (!finalizeSceneBlock(block, maxWidth, direction, wrap))
            {
                m_sceneBlocks.Resize(blockIndex);
                return nullptr;
            }

            return &block;
        };

        auto addSceneImage = [&](const LoadedDemoImage& image, const Vec2f& origin, float scale) -> SceneVectorImageBlock*
        {
            SceneVectorImageBlock& block = m_sceneImages.EmplaceBack();
            block.name = image.name;
            block.origin = origin;
            block.scale = scale;
            if (!buildRetainedImage(block.retainedImage, image))
            {
                return nullptr;
            }

            return &block;
        };

        scribe::LayoutOptions titleOptions{};
        titleOptions.fontSize = titleFontSize;
        titleOptions.wrap = true;
        titleOptions.maxWidth = contentWidth;
        titleOptions.direction = scribe::TextDirection::LeftToRight;
        if (!m_layoutEngine.LayoutText(m_titleLayout, primaryFace, m_titleText, titleOptions))
        {
            return false;
        }

        if (!buildRetainedText(m_retainedTitleText, primaryFace, m_titleLayout, titleFontSize))
        {
            return false;
        }

        switch (m_scene)
        {
            case DemoScene::FeatureOverview:
            {
                const float sectionLabelSize = 16.0f * dpiScale;
                const float featureTitleSize = 28.0f * dpiScale;
                const float featureDescSize = 17.0f * dpiScale;
                const float sampleSize = 24.0f * dpiScale;
                const float columnGap = 36.0f * dpiScale;
                const float leftWidth = contentWidth * 0.32f;
                const float sampleWidth = Max((contentWidth - leftWidth - (columnGap * 2.0f)) * 0.5f, 200.0f * dpiScale);
                const float disabledX = leftWidth + columnGap;
                const float enabledX = disabledX + sampleWidth + columnGap;
                const Vec4f mutedColor{ 0.35f, 0.35f, 0.35f, 1.0f };
                const Vec4f noteColor{ 0.15f, 0.45f, 0.90f, 1.0f };
                const uint32_t sansRegularIndex = (m_sansRegularFontIndex != InvalidIndex) ? m_sansRegularFontIndex : m_uiFontIndex;
                const uint32_t sansBoldIndex = (m_sansBoldFontIndex != InvalidIndex) ? m_sansBoldFontIndex : sansRegularIndex;
                const uint32_t sansItalicIndex = (m_sansItalicFontIndex != InvalidIndex) ? m_sansItalicFontIndex : sansRegularIndex;
                const uint32_t sansBoldItalicIndex = (m_sansBoldItalicFontIndex != InvalidIndex) ? m_sansBoldItalicFontIndex : sansBoldIndex;
                const uint32_t monoIndex = (m_monoFontIndex != InvalidIndex) ? m_monoFontIndex : sansRegularIndex;
                const uint32_t serifRegularIndex = (m_serifRegularFontIndex != InvalidIndex) ? m_serifRegularFontIndex : sansRegularIndex;
                const uint32_t featureFontIndex = (m_featureFontIndex != InvalidIndex) ? m_featureFontIndex : serifRegularIndex;
                const uint32_t rtlFontIndex = (m_rtlFontIndex != InvalidIndex) ? m_rtlFontIndex : sansRegularIndex;
                const uint32_t colorFontIndex = (m_colorFontIndex != InvalidIndex) ? m_colorFontIndex : sansRegularIndex;
                const uint32_t symbolFontIndex = (m_symbolFontIndex != InvalidIndex) ? m_symbolFontIndex : sansRegularIndex;

                auto addFaceIndices = [](SceneTextBlock& block, Span<const uint32_t> indices)
                {
                    for (uint32_t fontIndex : indices)
                    {
                        block.faceIndices.PushBack(fontIndex);
                    }
                };

                auto addCustomBlock = [&](SceneTextBlock&& prototype,
                                          float x,
                                          float y,
                                          float fontSize,
                                          float maxWidth,
                                          scribe::TextDirection direction,
                                          bool useAllFaces,
                                          uint32_t fontFaceIndex,
                                          const Vec4f& color = { 0.0f, 0.0f, 0.0f, 1.0f },
                                          bool wrap = true) -> SceneTextBlock*
                {
                    prototype.origin = { x, y };
                    prototype.fontSize = fontSize;
                    prototype.color = color;
                    prototype.fontFaceIndex = fontFaceIndex;
                    prototype.useAllFaces = useAllFaces;
                    return addPreparedSceneBlock(Move(prototype), maxWidth, direction, wrap);
                };

                if (!addSceneBlock("Feature", { 0.0f, 0.0f }, sectionLabelSize, leftWidth, false, scribe::TextDirection::LeftToRight, mutedColor)
                    || !addSceneBlock("Disabled / current gap", { disabledX, 0.0f }, sectionLabelSize, sampleWidth, false, scribe::TextDirection::LeftToRight, mutedColor)
                    || !addSceneBlock("Enabled / target", { enabledX, 0.0f }, sectionLabelSize, sampleWidth, false, scribe::TextDirection::LeftToRight, mutedColor))
                {
                    return false;
                }

                float rowY = 46.0f * dpiScale;
                for (uint32_t rowIndex = 0; rowIndex < HE_LENGTH_OF(FeatureRows); ++rowIndex)
                {
                    const FeatureRowSpec& row = FeatureRows[rowIndex];
                    SceneTextBlock* titleBlock = addSceneBlock(
                        row.title,
                        { 0.0f, rowY },
                        featureTitleSize,
                        leftWidth,
                        false,
                        scribe::TextDirection::LeftToRight);
                    if (!titleBlock)
                    {
                        return false;
                    }

                    SceneTextBlock* descriptionBlock = addSceneBlock(
                        row.description,
                        { 0.0f, rowY + titleBlock->layout.height + (6.0f * dpiScale) },
                        featureDescSize,
                        leftWidth,
                        false,
                        scribe::TextDirection::LeftToRight,
                        mutedColor);
                    if (!descriptionBlock)
                    {
                        return false;
                    }

                    SceneTextBlock* disabledBlock = nullptr;
                    SceneTextBlock* enabledBlock = nullptr;
                    switch (rowIndex)
                    {
                        case 0:
                        {
                            disabledBlock = addSceneBlock("Regular italic bold {code}", { disabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, false, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, sansRegularIndex, false, false, false);

                            SceneTextBlock enabled{};
                            enabled.text = "Regular italic bold {code}";
                            const uint32_t styleFaces[] = { sansRegularIndex, sansBoldIndex, sansItalicIndex, sansBoldItalicIndex, monoIndex };
                            addFaceIndices(enabled, styleFaces);
                            enabled.styles.Resize(5, DefaultInit);
                            enabled.styles[1].fontFaceIndex = 2;
                            enabled.styles[2].fontFaceIndex = 1;
                            enabled.styles[3].fontFaceIndex = 4;
                            AddStyleSpanForSubstring(enabled.styleSpans, enabled.text, "italic", 1);
                            AddStyleSpanForSubstring(enabled.styleSpans, enabled.text, "bold", 2);
                            AddStyleSpanForSubstring(enabled.styleSpans, enabled.text, "{code}", 3);
                            enabledBlock = addCustomBlock(Move(enabled), enabledX, rowY + (4.0f * dpiScale), sampleSize, sampleWidth, scribe::TextDirection::LeftToRight, false, sansRegularIndex, { 0.0f, 0.0f, 0.0f, 1.0f }, false);
                            break;
                        }

                        case 1:
                        {
                            disabledBlock = addSceneBlock("Stretched text\nSkewed text", { disabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, false, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, sansRegularIndex, false, false, false);

                            SceneTextBlock enabled{};
                            enabled.text = "Stretched text\nSkewed text";
                            const uint32_t faceIndices[] = { sansRegularIndex };
                            addFaceIndices(enabled, faceIndices);
                            enabled.styles.Resize(3, DefaultInit);
                            enabled.styles[1].stretchX = 1.35f;
                            enabled.styles[2].skewX = -0.35f;
                            AddStyleSpanForSubstring(enabled.styleSpans, enabled.text, "Stretched text", 1);
                            AddStyleSpanForSubstring(enabled.styleSpans, enabled.text, "Skewed text", 2);
                            enabledBlock = addCustomBlock(Move(enabled), enabledX, rowY + (4.0f * dpiScale), sampleSize, sampleWidth, scribe::TextDirection::LeftToRight, false, sansRegularIndex, { 0.0f, 0.0f, 0.0f, 1.0f }, false);
                            break;
                        }

                        case 2:
                        {
                            disabledBlock = addSceneBlock("Underline\nStrike-through", { disabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, false, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, sansRegularIndex, false, false, false);

                            SceneTextBlock enabled{};
                            enabled.text = "Underline\nStrike-through";
                            const uint32_t faceIndices[] = { sansRegularIndex };
                            addFaceIndices(enabled, faceIndices);
                            enabled.styles.Resize(3, DefaultInit);
                            enabled.styles[1].decorations = scribe::TextDecorationFlags::Underline;
                            enabled.styles[1].decorationColor = { 0.0f, 0.0f, 0.0f, 1.0f };
                            enabled.styles[2].decorations = scribe::TextDecorationFlags::Strikethrough;
                            enabled.styles[2].decorationColor = { 0.0f, 0.0f, 0.0f, 1.0f };
                            AddStyleSpanForSubstring(enabled.styleSpans, enabled.text, "Underline", 1);
                            AddStyleSpanForSubstring(enabled.styleSpans, enabled.text, "Strike-through", 2);
                            enabledBlock = addCustomBlock(Move(enabled), enabledX, rowY + (4.0f * dpiScale), sampleSize, sampleWidth, scribe::TextDirection::LeftToRight, false, sansRegularIndex, { 0.0f, 0.0f, 0.0f, 1.0f }, false);
                            break;
                        }

                        case 3:
                        {
                            disabledBlock = addSceneBlock("Tight tracking\nLoose tracking", { disabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, false, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, sansRegularIndex, false, false, false);

                            SceneTextBlock enabled{};
                            enabled.text = "Tight tracking\nLoose tracking";
                            const uint32_t faceIndices[] = { sansRegularIndex };
                            addFaceIndices(enabled, faceIndices);
                            enabled.styles.Resize(3, DefaultInit);
                            enabled.styles[1].trackingEm = -0.05f;
                            enabled.styles[2].trackingEm = 0.05f;
                            AddStyleSpanForSubstring(enabled.styleSpans, enabled.text, "Tight tracking", 1);
                            AddStyleSpanForSubstring(enabled.styleSpans, enabled.text, "Loose tracking", 2);
                            enabledBlock = addCustomBlock(Move(enabled), enabledX, rowY + (4.0f * dpiScale), sampleSize, sampleWidth, scribe::TextDirection::LeftToRight, false, sansRegularIndex, { 0.0f, 0.0f, 0.0f, 1.0f }, false);
                            break;
                        }

                        case 4:
                        {
                            disabledBlock = addSceneBlock("🙂 😀 🎨 🌈 ✨", { disabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, false, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, symbolFontIndex, false, false, false);
                            enabledBlock = addSceneBlock("🙂 😀 🎨 🌈 ✨", { enabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, true, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, colorFontIndex, false, false, false);
                            break;
                        }

                        case 5:
                        {
                            disabledBlock = addSceneBlock("👋 👋 👋", { disabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, false, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, colorFontIndex, false, false, false);
                            enabledBlock = addSceneBlock("👋🏻 👋🏽 👋🏿 🧑🏽‍⚕️", { enabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, true, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, colorFontIndex, false, false, false);
                            break;
                        }

                        case 6:
                        {
                            SceneTextBlock kernOff{};
                            kernOff.text = "AVATAR WAVE";
                            const uint32_t faceIndices[] = { featureFontIndex };
                            addFaceIndices(kernOff, faceIndices);
                            kernOff.styles.Resize(2, DefaultInit);
                            kernOff.features.EmplaceBack(scribe::TextFeatureSetting{ scribe::MakeOpenTypeFeatureTag('k', 'e', 'r', 'n'), 0 });
                            kernOff.styles[1].firstFeature = 0;
                            kernOff.styles[1].featureCount = 1;
                            kernOff.styleSpans.EmplaceBack(scribe::TextStyleSpan{ 0, static_cast<uint32_t>(kernOff.text.Size()), 1 });
                            disabledBlock = addCustomBlock(Move(kernOff), disabledX, rowY + (4.0f * dpiScale), sampleSize, sampleWidth, scribe::TextDirection::LeftToRight, false, featureFontIndex, { 0.0f, 0.0f, 0.0f, 1.0f }, false);
                            enabledBlock = addSceneBlock("AVATAR WAVE", { enabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, false, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, featureFontIndex, false, false, false);
                            break;
                        }

                        case 7:
                        {
                            SceneTextBlock ligaOff{};
                            ligaOff.text = "office official afflict";
                            const uint32_t faceIndices[] = { featureFontIndex };
                            addFaceIndices(ligaOff, faceIndices);
                            ligaOff.styles.Resize(2, DefaultInit);
                            ligaOff.features.EmplaceBack(scribe::TextFeatureSetting{ scribe::MakeOpenTypeFeatureTag('l', 'i', 'g', 'a'), 0 });
                            ligaOff.features.EmplaceBack(scribe::TextFeatureSetting{ scribe::MakeOpenTypeFeatureTag('c', 'l', 'i', 'g'), 0 });
                            ligaOff.styles[1].firstFeature = 0;
                            ligaOff.styles[1].featureCount = 2;
                            ligaOff.styleSpans.EmplaceBack(scribe::TextStyleSpan{ 0, static_cast<uint32_t>(ligaOff.text.Size()), 1 });
                            disabledBlock = addCustomBlock(Move(ligaOff), disabledX, rowY + (4.0f * dpiScale), sampleSize, sampleWidth, scribe::TextDirection::LeftToRight, false, featureFontIndex, { 0.0f, 0.0f, 0.0f, 1.0f }, false);
                            enabledBlock = addSceneBlock("office official afflict", { enabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, false, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, featureFontIndex, false, false, false);
                            break;
                        }

                        case 8:
                        {
                            disabledBlock = addSceneBlock("S ̧ l ̈ u ̄ g ̊", { disabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, false, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, serifRegularIndex, false, false, false);
                            enabledBlock = addSceneBlock("Şl̈ūg̊", { enabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, false, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, serifRegularIndex, false, false, false);
                            break;
                        }

                        case 9:
                        {
                            disabledBlock = addSceneBlock("Harvest Engine", { disabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, false, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, featureFontIndex, false, false, false);

                            SceneTextBlock enabled{};
                            enabled.text = "Harvest Engine";
                            const uint32_t faceIndices[] = { featureFontIndex };
                            addFaceIndices(enabled, faceIndices);
                            enabled.styles.Resize(2, DefaultInit);
                            enabled.features.EmplaceBack(scribe::TextFeatureSetting{ scribe::MakeOpenTypeFeatureTag('s', 'm', 'c', 'p'), 1 });
                            enabled.styles[1].firstFeature = 0;
                            enabled.styles[1].featureCount = 1;
                            enabled.styleSpans.EmplaceBack(scribe::TextStyleSpan{ 0, static_cast<uint32_t>(enabled.text.Size()), 1 });
                            enabledBlock = addCustomBlock(Move(enabled), enabledX, rowY + (4.0f * dpiScale), sampleSize, sampleWidth, scribe::TextDirection::LeftToRight, false, featureFontIndex, { 0.0f, 0.0f, 0.0f, 1.0f }, false);
                            break;
                        }

                        case 10:
                        {
                            disabledBlock = addSceneBlock("arial lqty69", { disabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, false, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, featureFontIndex, false, false, false);

                            SceneTextBlock enabled{};
                            enabled.text = "arial lqty69";
                            const uint32_t faceIndices[] = { featureFontIndex };
                            addFaceIndices(enabled, faceIndices);
                            enabled.styles.Resize(2, DefaultInit);
                            enabled.features.EmplaceBack(scribe::TextFeatureSetting{ scribe::MakeOpenTypeFeatureTag('s', 's', '0', '1'), 1 });
                            enabled.styles[1].firstFeature = 0;
                            enabled.styles[1].featureCount = 1;
                            enabled.styleSpans.EmplaceBack(scribe::TextStyleSpan{ 0, static_cast<uint32_t>(enabled.text.Size()), 1 });
                            enabledBlock = addCustomBlock(Move(enabled), enabledX, rowY + (4.0f * dpiScale), sampleSize, sampleWidth, scribe::TextDirection::LeftToRight, false, featureFontIndex, { 0.0f, 0.0f, 0.0f, 1.0f }, false);
                            break;
                        }

                        case 11:
                        {
                            disabledBlock = addSceneBlock("[(ALL-CAPS)]", { disabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, false, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, featureFontIndex, false, false, false);

                            SceneTextBlock enabled{};
                            enabled.text = "[(ALL-CAPS)]";
                            const uint32_t faceIndices[] = { featureFontIndex };
                            addFaceIndices(enabled, faceIndices);
                            enabled.styles.Resize(2, DefaultInit);
                            enabled.features.EmplaceBack(scribe::TextFeatureSetting{ scribe::MakeOpenTypeFeatureTag('c', 'a', 's', 'e'), 1 });
                            enabled.styles[1].firstFeature = 0;
                            enabled.styles[1].featureCount = 1;
                            enabled.styleSpans.EmplaceBack(scribe::TextStyleSpan{ 0, static_cast<uint32_t>(enabled.text.Size()), 1 });
                            enabledBlock = addCustomBlock(Move(enabled), enabledX, rowY + (4.0f * dpiScale), sampleSize, sampleWidth, scribe::TextDirection::LeftToRight, false, featureFontIndex, { 0.0f, 0.0f, 0.0f, 1.0f }, false);
                            break;
                        }

                        case 12:
                        {
                            disabledBlock = addSceneBlock("CH3CH2CH3\nFootnote5", { disabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, false, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, featureFontIndex, false, false, false);

                            SceneTextBlock enabled{};
                            enabled.text = "CH3CH2CH3\nFootnote5";
                            const uint32_t faceIndices[] = { featureFontIndex };
                            addFaceIndices(enabled, faceIndices);
                            enabled.styles.Resize(3, DefaultInit);
                            enabled.features.EmplaceBack(scribe::TextFeatureSetting{ scribe::MakeOpenTypeFeatureTag('s', 'u', 'b', 's'), 1 });
                            enabled.features.EmplaceBack(scribe::TextFeatureSetting{ scribe::MakeOpenTypeFeatureTag('s', 'u', 'p', 's'), 1 });
                            enabled.styles[1].firstFeature = 0;
                            enabled.styles[1].featureCount = 1;
                            enabled.styles[2].firstFeature = 1;
                            enabled.styles[2].featureCount = 1;
                            AddStyleSpanForSubstring(enabled.styleSpans, enabled.text, "3CH2CH3", 1);
                            AddStyleSpanForSubstring(enabled.styleSpans, enabled.text, "5", 2);
                            enabledBlock = addCustomBlock(Move(enabled), enabledX, rowY + (4.0f * dpiScale), sampleSize, sampleWidth, scribe::TextDirection::LeftToRight, false, featureFontIndex, { 0.0f, 0.0f, 0.0f, 1.0f }, false);
                            break;
                        }

                        case 13:
                        {
                            disabledBlock = addSceneBlock("TextSub1 Sub2\nTextSup1 Sup2", { disabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, false, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, sansRegularIndex, false, false, false);

                            SceneTextBlock enabled{};
                            enabled.text = "TextSub1 Sub2\nTextSup1 Sup2";
                            const uint32_t faceIndices[] = { sansRegularIndex };
                            addFaceIndices(enabled, faceIndices);
                            enabled.styles.Resize(5, DefaultInit);
                            enabled.styles[1].baselineShiftEm = 0.20f;
                            enabled.styles[1].glyphScale = 0.72f;
                            enabled.styles[2].baselineShiftEm = 0.20f;
                            enabled.styles[2].glyphScale = 0.60f;
                            enabled.styles[3].baselineShiftEm = -0.35f;
                            enabled.styles[3].glyphScale = 0.72f;
                            enabled.styles[4].baselineShiftEm = -0.35f;
                            enabled.styles[4].glyphScale = 0.60f;
                            AddStyleSpanForSubstring(enabled.styleSpans, enabled.text, "Sub1", 1);
                            AddStyleSpanForSubstring(enabled.styleSpans, enabled.text, "Sub2", 2);
                            AddStyleSpanForSubstring(enabled.styleSpans, enabled.text, "Sup1", 3);
                            AddStyleSpanForSubstring(enabled.styleSpans, enabled.text, "Sup2", 4);
                            enabledBlock = addCustomBlock(Move(enabled), enabledX, rowY + (4.0f * dpiScale), sampleSize, sampleWidth, scribe::TextDirection::LeftToRight, false, sansRegularIndex, { 0.0f, 0.0f, 0.0f, 1.0f }, false);
                            break;
                        }

                        case 14:
                        {
                            disabledBlock = addSceneBlock("1st 2nd 3rd 4th\n123/456", { disabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, false, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, featureFontIndex, false, false, false);

                            SceneTextBlock enabled{};
                            enabled.text = "1st 2nd 3rd 4th\n123/456";
                            const uint32_t faceIndices[] = { featureFontIndex };
                            addFaceIndices(enabled, faceIndices);
                            enabled.styles.Resize(3, DefaultInit);
                            enabled.features.EmplaceBack(scribe::TextFeatureSetting{ scribe::MakeOpenTypeFeatureTag('o', 'r', 'd', 'n'), 1 });
                            enabled.features.EmplaceBack(scribe::TextFeatureSetting{ scribe::MakeOpenTypeFeatureTag('f', 'r', 'a', 'c'), 1 });
                            enabled.styles[1].firstFeature = 0;
                            enabled.styles[1].featureCount = 1;
                            enabled.styles[2].firstFeature = 1;
                            enabled.styles[2].featureCount = 1;
                            AddStyleSpanForSubstring(enabled.styleSpans, enabled.text, "1st 2nd 3rd 4th", 1);
                            AddStyleSpanForSubstring(enabled.styleSpans, enabled.text, "123/456", 2);
                            enabledBlock = addCustomBlock(Move(enabled), enabledX, rowY + (4.0f * dpiScale), sampleSize, sampleWidth, scribe::TextDirection::LeftToRight, false, featureFontIndex, { 0.0f, 0.0f, 0.0f, 1.0f }, false);
                            break;
                        }

                        case 15:
                        {
                            disabledBlock = addSceneBlock("0123456789\n0123456789", { disabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, false, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, featureFontIndex, false, false, false);

                            SceneTextBlock enabled{};
                            enabled.text = "0123456789\n0123456789";
                            const uint32_t faceIndices[] = { featureFontIndex };
                            addFaceIndices(enabled, faceIndices);
                            enabled.styles.Resize(3, DefaultInit);
                            enabled.features.EmplaceBack(scribe::TextFeatureSetting{ scribe::MakeOpenTypeFeatureTag('o', 'n', 'u', 'm'), 1 });
                            enabled.features.EmplaceBack(scribe::TextFeatureSetting{ scribe::MakeOpenTypeFeatureTag('p', 'n', 'u', 'm'), 1 });
                            enabled.features.EmplaceBack(scribe::TextFeatureSetting{ scribe::MakeOpenTypeFeatureTag('l', 'n', 'u', 'm'), 1 });
                            enabled.features.EmplaceBack(scribe::TextFeatureSetting{ scribe::MakeOpenTypeFeatureTag('t', 'n', 'u', 'm'), 1 });
                            enabled.styles[1].firstFeature = 0;
                            enabled.styles[1].featureCount = 2;
                            enabled.styles[2].firstFeature = 2;
                            enabled.styles[2].featureCount = 2;
                            AddStyleSpanForSubstring(enabled.styleSpans, enabled.text, "0123456789", 1, 0);
                            AddStyleSpanForSubstring(enabled.styleSpans, enabled.text, "0123456789", 2, 1);
                            enabledBlock = addCustomBlock(Move(enabled), enabledX, rowY + (4.0f * dpiScale), sampleSize, sampleWidth, scribe::TextDirection::LeftToRight, false, featureFontIndex, { 0.0f, 0.0f, 0.0f, 1.0f }, false);
                            break;
                        }

                        case 16:
                        {
                            const char* unicodeSample = "Σύνθετη απόδοση γραμματοσειράς\nУлучшенный отрисовщик шрифтов\n高级字体渲染和文本布局";
                            disabledBlock = addSceneBlock(unicodeSample, { disabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, false, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, featureFontIndex);
                            enabledBlock = addSceneBlock(unicodeSample, { enabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, true, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, sansRegularIndex);
                            break;
                        }

                        case 17:
                        {
                            const char* rtlSample = "مرحبا بالعالم\nעיבוד גופן מתקדם";
                            disabledBlock = addSceneBlock(rtlSample, { disabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, false, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, rtlFontIndex, false, false, true);
                            enabledBlock = addSceneBlock(rtlSample, { enabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, true, scribe::TextDirection::RightToLeft, { 0.0f, 0.0f, 0.0f, 1.0f }, rtlFontIndex, false, false, true);
                            break;
                        }

                        case 18:
                        {
                            disabledBlock = addSceneBlock("ت\u200Cق\u200Cد\u200Cي\u200Cم\u200C الخط", { disabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, true, scribe::TextDirection::RightToLeft, { 0.0f, 0.0f, 0.0f, 1.0f }, rtlFontIndex, false, false, true);
                            enabledBlock = addSceneBlock("تقديم الخط", { enabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, true, scribe::TextDirection::RightToLeft, { 0.0f, 0.0f, 0.0f, 1.0f }, rtlFontIndex, false, false, true);
                            break;
                        }

                        case 19:
                        {
                            const char* bidiSample = "GPU text مع English 123";
                            disabledBlock = addSceneBlock(bidiSample, { disabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, true, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, rtlFontIndex, false, false, true);
                            enabledBlock = addSceneBlock(bidiSample, { enabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, true, scribe::TextDirection::Auto, { 0.0f, 0.0f, 0.0f, 1.0f }, rtlFontIndex, false, false, true);
                            break;
                        }

                        case 20:
                        {
                            disabledBlock = addSceneBlock("Shadow Outline Both", { disabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, false, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, sansBoldIndex, false, false, false);

                            SceneTextBlock enabled{};
                            enabled.text = "Shadow Outline Both";
                            const uint32_t faceIndices[] = { sansBoldIndex };
                            addFaceIndices(enabled, faceIndices);
                            enabled.styles.Resize(4, DefaultInit);
                            enabled.styles[1].effects = scribe::TextEffectFlags::Shadow;
                            enabled.styles[1].color = { 0.0f, 0.55f, 0.75f, 1.0f };
                            enabled.styles[1].shadowColor = { 0.0f, 0.35f, 0.55f, 0.45f };
                            enabled.styles[1].shadowOffsetEm = { 0.06f, 0.08f };
                            enabled.styles[2].effects = scribe::TextEffectFlags::Outline;
                            enabled.styles[2].color = { 1.0f, 0.85f, 0.15f, 1.0f };
                            enabled.styles[2].outlineColor = { 0.85f, 0.10f, 0.10f, 1.0f };
                            enabled.styles[2].outlineWidthEm = 0.05f;
                            enabled.styles[3].effects = scribe::TextEffectFlags::Shadow | scribe::TextEffectFlags::Outline;
                            enabled.styles[3].color = { 0.85f, 0.10f, 0.10f, 1.0f };
                            enabled.styles[3].shadowColor = { 0.0f, 0.0f, 0.0f, 0.35f };
                            enabled.styles[3].shadowOffsetEm = { 0.06f, 0.08f };
                            enabled.styles[3].outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
                            enabled.styles[3].outlineWidthEm = 0.05f;
                            AddStyleSpanForSubstring(enabled.styleSpans, enabled.text, "Shadow", 1);
                            AddStyleSpanForSubstring(enabled.styleSpans, enabled.text, "Outline", 2);
                            AddStyleSpanForSubstring(enabled.styleSpans, enabled.text, "Both", 3);
                            enabledBlock = addCustomBlock(Move(enabled), enabledX, rowY + (4.0f * dpiScale), sampleSize, sampleWidth, scribe::TextDirection::LeftToRight, false, sansBoldIndex, { 0.0f, 0.0f, 0.0f, 1.0f }, false);
                            break;
                        }

                        case 21:
                        {
                            disabledBlock = addSceneBlock("Text laid out on a curve", { disabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, false, scribe::TextDirection::LeftToRight, { 0.0f, 0.0f, 0.0f, 1.0f }, serifRegularIndex, false, false, false);

                            SceneTextBlock enabled{};
                            enabled.text = "Text laid out on a curve";
                            const uint32_t faceIndices[] = { serifRegularIndex };
                            addFaceIndices(enabled, faceIndices);
                            enabled.styles.Resize(enabled.text.Size() + 1u, DefaultInit);

                            uint32_t nonSpaceCount = 0;
                            for (uint32_t i = 0; i < enabled.text.Size(); ++i)
                            {
                                if (enabled.text.Data()[i] != ' ')
                                {
                                    ++nonSpaceCount;
                                }
                            }

                            uint32_t styleIndex = 1;
                            uint32_t nonSpaceIndex = 0;
                            for (uint32_t i = 0; i < enabled.text.Size(); ++i)
                            {
                                if (enabled.text.Data()[i] == ' ')
                                {
                                    continue;
                                }

                                const float t = (nonSpaceCount > 1u)
                                    ? (static_cast<float>(nonSpaceIndex) / static_cast<float>(nonSpaceCount - 1u))
                                    : 0.5f;
                                const float centered = (t * 2.0f) - 1.0f;
                                enabled.styles[styleIndex].baselineShiftEm = 0.55f * (1.0f - (centered * centered));
                                enabled.styles[styleIndex].rotationRadians = centered * -0.55f;
                                enabled.styleSpans.EmplaceBack(scribe::TextStyleSpan{ i, i + 1u, styleIndex });
                                ++styleIndex;
                                ++nonSpaceIndex;
                            }

                            enabledBlock = addCustomBlock(Move(enabled), enabledX, rowY + (4.0f * dpiScale), sampleSize, sampleWidth, scribe::TextDirection::LeftToRight, false, serifRegularIndex, { 0.0f, 0.0f, 0.0f, 1.0f }, false);
                            break;
                        }

                        default:
                        {
                            disabledBlock = addSceneBlock(row.disabledSample, { disabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, true, row.disabledDirection);
                            enabledBlock = addSceneBlock(row.enabledSample, { enabledX, rowY + (4.0f * dpiScale) }, sampleSize, sampleWidth, true, row.enabledDirection);
                            break;
                        }
                    }

                    if (!disabledBlock || !enabledBlock)
                    {
                        return false;
                    }

                    const float leftHeight = (descriptionBlock->origin.y + descriptionBlock->layout.height) - rowY;
                    const float centerHeight = (disabledBlock->origin.y + disabledBlock->layout.height) - rowY;
                    const float rightHeight = (enabledBlock->origin.y + enabledBlock->layout.height) - rowY;
                    const float rowHeight = Max(leftHeight, Max(centerHeight, rightHeight));
                    rowY += rowHeight + (26.0f * dpiScale);
                }

                if (!addSceneBlock(
                        "Scene 1 is the active feature roadmap: text rows now render real current/target comparisons, while vector paint and stroke features remain tracked for the SVG scene.",
                        { 0.0f, rowY + (6.0f * dpiScale) },
                        featureDescSize,
                        contentWidth,
                        false,
                        scribe::TextDirection::LeftToRight,
                        noteColor))
                {
                    return false;
                }
                break;
            }

            case DemoScene::RichParagraphs:
            {
                bodyWidth = Min(contentWidth, 1100.0f * dpiScale);
                bodyFontSize = 26.0f * dpiScale;
                if (!layoutText(m_bodyLayout, faceSpan, m_bodyText, bodyFontSize, bodyWidth, scribe::TextDirection::Auto))
                {
                    return false;
                }

                if (!buildRetainedText(m_retainedBodyText, faceSpan, m_bodyLayout, bodyFontSize))
                {
                    return false;
                }
                break;
            }

            case DemoScene::EmojiPage:
            {
                bodyWidth = Max(contentWidth, 2200.0f * dpiScale);
                bodyFontSize = 44.0f * dpiScale;
                if (!layoutText(
                        m_bodyLayout,
                        faceSpan,
                        m_bodyText,
                        bodyFontSize,
                        bodyWidth,
                        scribe::TextDirection::LeftToRight,
                        false))
                {
                    return false;
                }

                if (!buildRetainedText(m_retainedBodyText, faceSpan, m_bodyLayout, bodyFontSize))
                {
                    return false;
                }
                break;
            }

            case DemoScene::SvgGallery:
            {
                const float noteFontSize = 18.0f * dpiScale;
                const float labelFontSize = 16.0f * dpiScale;
                const float columnGap = 36.0f * dpiScale;
                const float rowGap = 48.0f * dpiScale;
                const float cellWidth = Max((contentWidth - columnGap) * 0.5f, 240.0f * dpiScale);
                const float cellHeight = 420.0f * dpiScale;
                const Vec4f mutedColor{ 0.35f, 0.35f, 0.35f, 1.0f };
                String noteText =
                    "These SVGs are compiled at startup into Scribe runtime blobs and then rendered through the retained vector-image path. "
                    "Pan and zoom the page to inspect edge quality and fill-rule behavior.";
                if (m_images.IsEmpty())
                {
                    noteText += " No SVGs loaded successfully from the test directory.";
                }
                else if (!m_svgLoadErrors.IsEmpty())
                {
                    noteText += " Skipped SVGs:";
                    for (const String& failedName : m_svgLoadErrors)
                    {
                        noteText += "\n- ";
                        noteText += failedName;
                    }
                }

                SceneTextBlock* noteBlock = addSceneBlock(
                    noteText.Data(),
                    { 0.0f, 0.0f },
                    noteFontSize,
                    contentWidth,
                    false,
                    scribe::TextDirection::LeftToRight,
                    mutedColor);
                if (!noteBlock)
                {
                    return false;
                }

                float rowY = noteBlock->origin.y + noteBlock->layout.height + (28.0f * dpiScale);
                for (uint32_t imageIndex = 0; imageIndex < m_images.Size(); ++imageIndex)
                {
                    const uint32_t columnIndex = imageIndex % 2;
                    const uint32_t rowIndex = imageIndex / 2;
                    const float x = static_cast<float>(columnIndex) * (cellWidth + columnGap);
                    const float y = rowY + (static_cast<float>(rowIndex) * (cellHeight + rowGap));
                    const Vec2f imageOrigin{ x, y };

                    const float viewBoxWidth = Max(m_images[imageIndex].blob.GetMetadata().GetSourceViewBoxWidth(), 1.0f);
                    const float viewBoxHeight = Max(m_images[imageIndex].blob.GetMetadata().GetSourceViewBoxHeight(), 1.0f);
                    const float scale = Min(cellWidth / viewBoxWidth, (cellHeight - (28.0f * dpiScale)) / viewBoxHeight);

                    if (!addSceneImage(m_images[imageIndex], imageOrigin, scale))
                    {
                        return false;
                    }

                    if (!addSceneBlock(
                            m_images[imageIndex].name.Data(),
                            { x, y + (viewBoxHeight * scale) + (10.0f * dpiScale) },
                            labelFontSize,
                            cellWidth,
                            false,
                            scribe::TextDirection::LeftToRight,
                            mutedColor))
                    {
                        return false;
                    }
                }
                break;
            }

            case DemoScene::SmallTextAlignment:
            {
                const float noteFontSize = 18.0f * dpiScale;
                const float lineWidth = Min(contentWidth, 1500.0f * dpiScale);
                const Vec4f mutedColor{ 0.35f, 0.35f, 0.35f, 1.0f };
                float rowY = 0.0f;

                SceneTextBlock* noteBlock = addSceneBlock(
                    "These rows use font sizes chosen from the compiled cap-height metric so the cap line and baseline land on whole pixels at the default zoom.",
                    { 0.0f, rowY },
                    noteFontSize,
                    lineWidth,
                    false,
                    scribe::TextDirection::LeftToRight,
                    mutedColor);
                if (!noteBlock)
                {
                    return false;
                }
                rowY += noteBlock->layout.height + (18.0f * dpiScale);

                constexpr float CapHeights[] = { 13.0f, 12.0f, 11.0f, 10.0f, 9.0f, 8.0f };
                for (float capHeightPixels : CapHeights)
                {
                    String lineText;
                    FormatTo(
                        lineText,
                        "Cap {:>2.0f}px  ABCDEFGHIJKLMNOPQRSTUVWXYZ  abcdefghijklmnopqrstuvwxyz  0123456789  Cafe\u0301  Ångström  WY/WY/III",
                        capHeightPixels);

                    const float alignedFontSize = ComputeCapAlignedFontSize(m_fonts[0].blob, capHeightPixels) * dpiScale;
                    SceneTextBlock* rowBlock = addSceneBlock(
                        lineText.Data(),
                        { 0.0f, rowY },
                        alignedFontSize,
                        lineWidth,
                        false,
                        scribe::TextDirection::LeftToRight,
                        { 0.0f, 0.0f, 0.0f, 1.0f },
                        0,
                        true,
                        true);
                    if (!rowBlock)
                    {
                        return false;
                    }

                    rowY += rowBlock->layout.height + (10.0f * dpiScale);
                }
                break;
            }

            case DemoScene::_Count:
                break;
        }

        if (!UpdateOverlayLayout())
        {
            return false;
        }

        const float overlayHeight = Max(m_sceneStatsLayout.height, Max(m_renderStatsLayout.height, m_inputHintsLayout.height));
        const float sceneTop = margin + overlayHeight + (20.0f * dpiScale);
        m_titleOrigin = {
            Max((static_cast<float>(viewSize.x) - m_titleLayout.width) * 0.5f, margin),
            sceneTop
        };
        m_bodyOrigin = {
            Max((static_cast<float>(viewSize.x) - bodyWidth) * 0.5f, margin),
            sceneTop + m_titleLayout.height + (28.0f * dpiScale)
        };

        const Vec2f blockOffset{ margin, sceneTop + m_titleLayout.height + (30.0f * dpiScale) };
        for (SceneTextBlock& block : m_sceneBlocks)
        {
            block.origin = {
                block.origin.x + blockOffset.x,
                block.origin.y + blockOffset.y
            };
        }
        for (SceneVectorImageBlock& block : m_sceneImages)
        {
            block.origin = {
                block.origin.x + blockOffset.x,
                block.origin.y + blockOffset.y
            };
        }

        m_hasCaret = false;
        m_bodyFontSize = bodyFontSize;
        m_sceneVertexEstimate = m_retainedTitleText.GetEstimatedVertexCount() + m_retainedBodyText.GetEstimatedVertexCount();
        for (const SceneTextBlock& block : m_sceneBlocks)
        {
            m_sceneVertexEstimate += block.retainedText.GetEstimatedVertexCount();
        }
        for (const SceneVectorImageBlock& block : m_sceneImages)
        {
            m_sceneVertexEstimate += block.retainedImage.GetEstimatedVertexCount();
        }

        m_layoutDirty = false;
        return true;
    }

    bool ScribeTestApp::UpdateOverlayLayout()
    {
        if (m_fonts.IsEmpty() || !m_fonts[0].blob.IsValid())
        {
            return false;
        }

        const Vec2i viewSize = m_view->GetSize();
        const float dpiScale = Max(m_view->GetDpiScale(), 1.0f);
        const float margin = 40.0f * dpiScale;
        const float overlayFontSize = 16.0f * dpiScale;
        const float graphLabelFontSize = 11.0f * dpiScale;
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
            GetSceneMissingGlyphCount(),
            GetSceneFallbackGlyphCount());

        m_renderStatsText.Clear();
        FormatTo(
            m_renderStatsText,
            "FPS: {:.1f}\n"
            "GPU: {:.2f} ms\n"
            "Batching: {}\n"
            "Draws: {}\n"
            "Resolution: {} x {}",
            fps,
            m_smoothedGpuFrameMs,
            m_glyphBatchingEnabled ? "on" : "off",
            m_lastDrawCount,
            viewSize.x,
            viewSize.y);

        m_inputHintsText =
            "Left drag: pan\n"
            "Mouse wheel: zoom\n"
            "B: toggle batching\n"
            "R: reset view\n"
            "Left/Right: switch demos\n"
            "Esc: quit";

        scribe::LayoutOptions overlayOptions{};
        overlayOptions.fontSize = overlayFontSize;
        overlayOptions.wrap = true;
        overlayOptions.maxWidth = columnWidth;
        overlayOptions.direction = scribe::TextDirection::LeftToRight;

        const scribe::FontFaceResourceReader primaryFace[] = { m_fonts[0].blob };
        const Span<const scribe::FontFaceResourceReader> faceSpan(primaryFace, 1);
        if (!m_layoutEngine.LayoutText(m_sceneStatsLayout, faceSpan, m_sceneStatsText, overlayOptions)
            || !m_layoutEngine.LayoutText(m_renderStatsLayout, faceSpan, m_renderStatsText, overlayOptions)
            || !m_layoutEngine.LayoutText(m_inputHintsLayout, faceSpan, m_inputHintsText, overlayOptions))
        {
            return false;
        }

        auto buildRetainedText = [&](scribe::RetainedTextModel& out, const scribe::LayoutResult& layout, float fontSize) -> bool
        {
            out.Clear();

            scribe::RetainedTextBuildDesc retainedDesc{};
            retainedDesc.fontFaces = faceSpan;
            retainedDesc.layout = &layout;
            retainedDesc.fontSize = fontSize;
            retainedDesc.darkBackgroundPreferred = true;
            return out.Build(retainedDesc) && m_renderer.PrepareRetainedText(out);
        };

        if (!buildRetainedText(m_retainedSceneStatsText, m_sceneStatsLayout, overlayFontSize)
            || !buildRetainedText(m_retainedRenderStatsText, m_renderStatsLayout, overlayFontSize)
            || !buildRetainedText(m_retainedInputHintsText, m_inputHintsLayout, overlayFontSize))
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
        const float graphGap = 16.0f * dpiScale;
        const float graphLabelWidth = 48.0f * dpiScale;
        const float graphX = m_renderStatsOrigin.x + m_renderStatsLayout.width + graphGap + graphLabelWidth;
        const float graphRight = inputX - graphGap;
        const float graphWidth = Max(graphRight - graphX, 0.0f);
        if (graphWidth >= (120.0f * dpiScale))
        {
            m_gpuGraphOrigin = { graphX, margin + (2.0f * dpiScale) };
            m_gpuGraphSize = { Min(graphWidth, 220.0f * dpiScale), 92.0f * dpiScale };

            scribe::LayoutOptions labelOptions{};
            labelOptions.fontSize = graphLabelFontSize;
            labelOptions.wrap = false;
            labelOptions.maxWidth = graphLabelWidth;
            labelOptions.direction = scribe::TextDirection::LeftToRight;

            for (uint32_t i = 0; i < GpuGraphTickCount; ++i)
            {
                const float valueMs = GpuGraphMaxMs * (1.0f - (static_cast<float>(i) / static_cast<float>(GpuGraphTickCount - 1)));
                String labelText{};
                FormatTo(labelText, "{:.1f}ms", valueMs);
                if (!m_layoutEngine.LayoutText(m_gpuGraphLabelLayouts[i], faceSpan, labelText, labelOptions)
                    || !buildRetainedText(m_retainedGpuGraphLabels[i], m_gpuGraphLabelLayouts[i], graphLabelFontSize))
                {
                    return false;
                }

                const float tickY = m_gpuGraphOrigin.y + (m_gpuGraphSize.y * (static_cast<float>(i) / static_cast<float>(GpuGraphTickCount - 1)));
                m_gpuGraphLabelOrigins[i] = {
                    m_gpuGraphOrigin.x - (6.0f * dpiScale) - m_gpuGraphLabelLayouts[i].width,
                    tickY - (m_gpuGraphLabelLayouts[i].height * 0.5f)
                };
            }
        }
        else
        {
            m_gpuGraphOrigin = { 0.0f, 0.0f };
            m_gpuGraphSize = { 0.0f, 0.0f };
            for (uint32_t i = 0; i < GpuGraphTickCount; ++i)
            {
                m_gpuGraphLabelLayouts[i].Clear();
                m_retainedGpuGraphLabels[i].Clear();
                m_gpuGraphLabelOrigins[i] = { 0.0f, 0.0f };
            }
        }
        m_overlayVertexEstimate = m_retainedSceneStatsText.GetEstimatedVertexCount()
            + m_retainedRenderStatsText.GetEstimatedVertexCount()
            + m_retainedInputHintsText.GetEstimatedVertexCount();
        for (uint32_t i = 0; i < GpuGraphTickCount; ++i)
        {
            m_overlayVertexEstimate += m_retainedGpuGraphLabels[i].GetEstimatedVertexCount();
        }
        return true;
    }

    uint32_t ScribeTestApp::GetSceneMissingGlyphCount() const
    {
        uint32_t count = m_bodyLayout.missingGlyphCount;
        for (const SceneTextBlock& block : m_sceneBlocks)
        {
            count += block.layout.missingGlyphCount;
        }
        return count;
    }

    uint32_t ScribeTestApp::GetSceneFallbackGlyphCount() const
    {
        uint32_t count = m_bodyLayout.fallbackGlyphCount;
        for (const SceneTextBlock& block : m_sceneBlocks)
        {
            count += block.layout.fallbackGlyphCount;
        }
        return count;
    }

    Vec2f ScribeTestApp::GetSceneBlockRenderOrigin(const SceneTextBlock& block) const
    {
        Vec2f origin{
            (block.origin.x * m_sceneZoom) + m_scenePan.x,
            (block.origin.y * m_sceneZoom) + m_scenePan.y
        };
        float leftEdgeX = Limits<float>::Max;
        float renderedMinX = 0.0f;
        if (TryGetRenderedLineMinX(renderedMinX, block.retainedText, block.layout, 0))
        {
            leftEdgeX = origin.x + (renderedMinX * m_sceneZoom);
        }
        else
        {
            for (const scribe::TextCluster& cluster : block.layout.clusters)
            {
                if (cluster.isWhitespace || (cluster.lineIndex != 0))
                {
                    continue;
                }

                leftEdgeX = Min(leftEdgeX, origin.x + (cluster.x0 * m_sceneZoom));
            }

            if (leftEdgeX == Limits<float>::Max)
            {
                origin.x = Round(origin.x);
            }
        }

        if (leftEdgeX != Limits<float>::Max)
        {
            origin.x += Round(leftEdgeX) - leftEdgeX;
        }

        if ((!block.pixelAlignBaseline && !block.pixelAlignCapHeight)
            || (Abs(m_sceneZoom - 1.0f) > 0.001f)
            || block.layout.lines.IsEmpty()
            || (block.fontFaceIndex >= m_fonts.Size())
            || !m_fonts[block.fontFaceIndex].blob.GetMetadata().IsValid())
        {
            return origin;
        }

        const scribe::FontFaceMetrics::Reader metrics = m_fonts[block.fontFaceIndex].blob.GetMetadata().GetMetrics();
        const float unitsPerEm = static_cast<float>(Max(metrics.GetUnitsPerEm(), 1u));
        const float scale = block.fontSize / unitsPerEm;
        const float baselineY = origin.y + block.layout.lines[0].baselineY;

        float deltaY = 0.0f;
        if (block.pixelAlignBaseline)
        {
            deltaY += Round(baselineY) - baselineY;
        }

        if (block.pixelAlignCapHeight)
        {
            const float alignedBaselineY = baselineY + deltaY;
            const float capHeight = static_cast<float>(Max(Abs(metrics.GetCapHeight()), 1)) * scale;
            const float capTopY = alignedBaselineY - capHeight;
            deltaY += Round(capTopY) - capTopY;
        }

        origin.y += deltaY;
        return origin;
    }

    Vec2f ScribeTestApp::GetSceneImageRenderOrigin(const SceneVectorImageBlock& block) const
    {
        return {
            (block.origin.x * m_sceneZoom) + m_scenePan.x,
            (block.origin.y * m_sceneZoom) + m_scenePan.y
        };
    }

    void ScribeTestApp::QueueCaret()
    {
        if (m_scene != DemoScene::RichParagraphs)
        {
            return;
        }

        if (!m_hasCaret || !m_caretGlyph.atlas || (m_caretGlyph.vertexCount == 0) || (m_caretHit.lineIndex >= m_bodyLayout.lines.Size()))
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
            case DemoScene::FeatureOverview:
                m_titleText = "Scribe Testbed: renderer and typography feature overview";
                m_bodyText.Clear();
                break;

            case DemoScene::RichParagraphs:
                m_titleText = "Scribe Testbed: rich text stress page";
                m_bodyText =
                    "Scribe compiles source fonts in memory, shapes text with HarfBuzz, and renders "
                    "directly from compiled curve and band payloads. This page keeps the content dense "
                    "so wrapping, fallback selection, cluster preservation, and color-glyph submission "
                    "can all be inspected in a single view.\n\n"
                    "Inline emoji fallback stays active here: 🙂 😀 🎨 🌈 ✨. Combining marks should remain "
                    "clustered in words like Café, Ångström, and Ślüg. Icon fallback from the repository "
                    "font should also remain isolated to the missing glyph itself: ";
                m_bodyText += TestIconAccount;
                m_bodyText +=
                    ".\n\n"
                    "Mixed-script content exercises the current shaping and fallback path across scripts: "
                    "Σύνθετη απόδοση γραμματοσειράς και διάταξη κειμένου. Улучшенный отрисовщик шрифтов "
                    "и макет текста. 高级字体渲染和文本布局.\n\n"
                    "Right-to-left and bidi behavior are still a milestone in progress, but the current "
                    "testbed should keep these runs stable enough to inspect: مرحبا بالعالم 12345, "
                    "עיבוד גופן מתקדם 67890, and GPU text مع English words mixed into the same paragraph.\n\n"
                    "Text decorations, run-level style switches, tracking, and OpenType feature toggles "
                    "are not exposed yet, so this page calls them out in prose instead of pretending they "
                    "already exist. The goal of this scene is to make the current strengths and gaps visible "
                    "in one long page of text.";
                break;

            case DemoScene::EmojiPage:
                m_titleText = "Scribe Testbed: color emoji page";
                m_bodyText = c_emoji_page_text;
                break;

            case DemoScene::SvgGallery:
                m_titleText = "Scribe Testbed: retained SVG gallery";
                m_bodyText.Clear();
                break;

            case DemoScene::SmallTextAlignment:
                m_titleText = "Scribe Testbed: small-text cap-height alignment";
                m_bodyText.Clear();
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
            if (frame.gpuTimerReadback && (m_render.renderQueueTimestampFrequency > 0))
            {
                const uint64_t* timestamps = static_cast<const uint64_t*>(
                    m_render.device->Map(frame.gpuTimerReadback, 0, sizeof(uint64_t) * 2));

                if (timestamps)
                {
                    const uint64_t beginTicks = timestamps[0];
                    const uint64_t endTicks = timestamps[1];
                    const uint64_t elapsedTicks = (endTicks >= beginTicks) ? (endTicks - beginTicks) : 0;
                    m_lastGpuFrameMs = static_cast<float>(
                        (static_cast<double>(elapsedTicks) * 1000.0)
                        / static_cast<double>(m_render.renderQueueTimestampFrequency));
                    m_render.device->Unmap(frame.gpuTimerReadback);
                }
                else
                {
                    m_lastGpuFrameMs = 0.0f;
                }
            }
            else
            {
                m_lastGpuFrameMs = 0.0f;
            }

            if (m_smoothedGpuFrameMs <= 0.0f)
            {
                m_smoothedGpuFrameMs = m_lastGpuFrameMs;
            }
            else
            {
                m_smoothedGpuFrameMs = Lerp(m_smoothedGpuFrameMs, m_lastGpuFrameMs, 0.12f);
            }

            if (!m_gpuFrameHistory.IsEmpty())
            {
                m_gpuFrameHistory[m_gpuFrameHistoryHead] = m_smoothedGpuFrameMs;
                m_gpuFrameHistoryHead = (m_gpuFrameHistoryHead + 1u) % m_gpuFrameHistory.Size();
                m_gpuFrameHistoryFilled |= m_gpuFrameHistoryHead == 0u;
            }

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
        m_render.frames[m_render.frameIndex].hasSubmittedWork = true;
        m_lastDrawCount = m_renderer.GetLastSubmittedDrawCount();
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
        return (m_fonts.Size() > 2) && m_fonts[2].blob.IsValid();
    }

    bool ScribeTestApp::HasColorDemoFont() const
    {
        for (const LoadedDemoFont& font : m_fonts)
        {
            if (font.blob.GetMetadata().IsValid()
                && font.blob.GetMetadata().GetHasColorGlyphs()
                && font.blob.GetPaint().IsValid()
                && !font.blob.GetPaint().GetPalettes().IsEmpty())
            {
                return true;
            }
        }

        return false;
    }
}
