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
                "Weight, italic, and code spans are part of the long-term text model. They are not mapped to real font-style runs yet.",
                "Regular / italic / bold toggle not yet implemented.",
                "Regular italics bold {code} target sample.",
            },
            {
                "Stretch and skew",
                "Per-run stretch and skew are not exposed through the layout API yet, but the testbed keeps them listed as a future renderer target.",
                "Text stretched (not yet implemented).",
                "Text skewed (not yet implemented).",
            },
            {
                "Text decorations",
                "Underline, strike-through, and other text decorations still need explicit shaping and paint support.",
                "Text underline (not yet implemented).",
                "Text strike-through (not yet implemented).",
            },
            {
                "Tracking",
                "Run-level tracking and letter spacing are not exposed yet. The row stays here as a roadmap checkpoint.",
                "Tight tracking -0.05 (not yet implemented).",
                "Loose tracking +0.05 (not yet implemented).",
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
                "Kerning is shaped through HarfBuzz today, but there is no UI toggle to disable it for a direct side-by-side comparison.",
                "Toggle not exposed in the testbed.",
                "“Too Wavy.”",
            },
            {
                "Ligatures",
                "Ligatures are shaped through HarfBuzz when the active face supports them.",
                "Toggle not exposed in the testbed.",
                "office firefly craft",
            },
            {
                "Combining marks",
                "Combining marks and cluster preservation are already working in the shaping path.",
                "Slug decomposed cluster stress.",
                "Şl̈ūg̊",
            },
            {
                "Small caps",
                "Small-cap substitutions are not wired through feature selection yet.",
                "Small caps (not yet implemented).",
                "SMALL CAPS target sample.",
            },
            {
                "Stylistic variants",
                "Style sets and alternate glyph forms still need a feature-selection surface in layout.",
                "Default glyph form only.",
                "Style-set substitution target sample.",
            },
            {
                "Case-sensitive forms",
                "Case-sensitive punctuation substitutions are still pending OpenType feature selection.",
                "[(ALL-CAPS)] default punctuation.",
                "[(ALL-CAPS)] target substitution sample.",
            },
            {
                "Subscripts and superscripts",
                "Native OpenType substitutions are not implemented yet, but Unicode fallback characters can still be displayed.",
                "CH3CH2CH3 / Footnote5",
                "CH₃CH₂CH₃ / Footnote⁵",
            },
            {
                "Script transforms",
                "Nested script transforms belong in a future styled-run model, not the current plain-text test path.",
                "Text sub1 sub2 (not yet implemented).",
                "Text sup1 sup2 (not yet implemented).",
            },
            {
                "Ordinals and fractions",
                "Ordinal and fraction substitutions are still pending feature-selection support.",
                "1st 2nd 3rd 4th / 123/456",
                "1ˢᵗ 2ⁿᵈ 3ʳᵈ 4ᵗʰ / ½ ¾",
            },
            {
                "Figure styles",
                "Old-style, tabular, and proportional figures are not yet selectable from the testbed.",
                "0123456789 lining / proportional default.",
                "Old-style and tabular targets not yet implemented.",
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
                "Shadows and geometric outlines are not implemented in Scribe yet, but they belong on the renderer roadmap.",
                "Shadow / Outline / Both not yet implemented.",
                "Effect pipeline target sample.",
            },
            {
                "Per-glyph transforms",
                "Curved text and per-glyph transforms require a richer scene model than the current layout result exposes.",
                "Per-glyph transforms not yet implemented.",
                "Curved baseline target sample.",
            },
            {
                "Vector strokes and gradients",
                "Stroke caps, joins, dashing, and fill gradients belong to future SVG/vector milestones and are listed here as pending.",
                "Stroke dashing and gradients not yet implemented.",
                "Future vector paint and stroke target.",
            },
        };

        Vec4f ToVec4f(scribe::FontFacePaletteColor::Reader color)
        {
            return {
                color.GetRed(),
                color.GetGreen(),
                color.GetBlue(),
                color.GetAlpha()
            };
        }

        float ComputeCapAlignedFontSize(const scribe::LoadedFontFaceBlob& font, float capHeightPixels)
        {
            if (!font.metadata.IsValid())
            {
                return capHeightPixels;
            }

            const auto metrics = font.metadata.GetMetrics();
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
        const float bodyFontSize = m_bodyFontSize;
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

        QueueLayout(m_titleLayout, transformedTitleOrigin, titleFontSize, m_sceneZoom);
        if (!m_bodyText.IsEmpty())
        {
            QueueLayout(m_bodyLayout, transformedBodyOrigin, bodyFontSize, m_sceneZoom);
        }

        for (const SceneTextBlock& block : m_sceneBlocks)
        {
            QueueLayout(block.layout, GetSceneBlockRenderOrigin(block), block.fontSize, m_sceneZoom, block.color);
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
            || !LoadDemoFonts())
        {
            return false;
        }

        BuildFontGlyphCacheState();

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
            m_fontGlyphCache.Clear();

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
        m_sceneBlocks.Clear();
        m_sceneStatsLayout.Clear();
        m_renderStatsLayout.Clear();
        m_inputHintsLayout.Clear();
        m_initialized = false;
    }

    void ScribeTestApp::BuildFontGlyphCacheState()
    {
        m_fontGlyphCache.Resize(m_fonts.Size(), DefaultInit);

        for (uint32_t fontIndex = 0; fontIndex < m_fonts.Size(); ++fontIndex)
        {
            FontGlyphCacheState& cacheState = m_fontGlyphCache[fontIndex];
            cacheState.glyphResourceIndices.Clear();
            cacheState.colorGlyphRanges.Clear();
            cacheState.colorGlyphLayers.Clear();
            cacheState.selectedPaletteIndex = 0;
            cacheState.hasColorGlyphs = false;

            const LoadedDemoFont& font = m_fonts[fontIndex];
            if (!font.blob.metadata.IsValid() || !font.blob.paint.IsValid())
            {
                continue;
            }

            cacheState.glyphResourceIndices.Resize(font.blob.metadata.GetGlyphCount(), int32_t(-1));
            cacheState.hasColorGlyphs =
                font.blob.metadata.GetHasColorGlyphs()
                && font.blob.paint.IsValid()
                && !font.blob.paint.GetPalettes().IsEmpty();
            if (cacheState.hasColorGlyphs)
            {
                cacheState.selectedPaletteIndex = scribe::SelectCompiledFontPalette(font.blob, true);

                const auto colorGlyphs = font.blob.paint.GetColorGlyphs();
                const auto palette = font.blob.paint.GetPalettes()[cacheState.selectedPaletteIndex];
                const auto colors = palette.GetColors();
                const auto layers = font.blob.paint.GetLayers();

                cacheState.colorGlyphRanges.Resize(colorGlyphs.Size(), DefaultInit);
                for (uint32_t glyphIndex = 0; glyphIndex < colorGlyphs.Size(); ++glyphIndex)
                {
                    const scribe::FontFaceColorGlyph::Reader colorGlyph = colorGlyphs[glyphIndex];
                    const uint32_t layerCount = colorGlyph.GetLayerCount();
                    FontGlyphCacheState::ColorGlyphRange& range = cacheState.colorGlyphRanges[glyphIndex];
                    range.firstLayer = cacheState.colorGlyphLayers.Size();
                    range.layerCount = layerCount;

                    for (uint32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex)
                    {
                        const scribe::FontFaceColorGlyphLayer::Reader layer = layers[colorGlyph.GetFirstLayer() + layerIndex];
                        FontGlyphCacheState::CachedColorGlyphLayer& cachedLayer = cacheState.colorGlyphLayers.EmplaceBack();
                        cachedLayer.glyphIndex = layer.GetGlyphIndex();
                        cachedLayer.color = { 1.0f, 1.0f, 1.0f, Max(layer.GetAlphaScale(), 0.0f) };
                        cachedLayer.basisX = { layer.GetTransform00(), layer.GetTransform10() };
                        cachedLayer.basisY = { layer.GetTransform01(), layer.GetTransform11() };
                        cachedLayer.offset = { layer.GetTransformTx(), layer.GetTransformTy() };
                        cachedLayer.useForegroundColor = (layer.GetFlags() & scribe::CompiledFontColorLayerFlagUseForeground) != 0;

                        if (!cachedLayer.useForegroundColor)
                        {
                            const uint32_t paletteEntryIndex = layer.GetPaletteEntryIndex();
                            if (paletteEntryIndex < colors.Size())
                            {
                                cachedLayer.color = ToVec4f(colors[paletteEntryIndex]);
                                cachedLayer.color.w *= Max(layer.GetAlphaScale(), 0.0f);
                            }
                        }
                    }
                }
            }
        }
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
        float bodyFontSize = 24.0f * dpiScale;
        float contentWidth = Max(static_cast<float>(viewSize.x) - (margin * 2.0f), 128.0f);
        float bodyWidth = contentWidth;

        m_sceneBlocks.Clear();
        m_bodyLayout.Clear();
        m_bodyOrigin = { 0.0f, 0.0f };

        auto layoutText = [&](scribe::LayoutResult& out,
                              Span<const scribe::LoadedFontFaceBlob> blockFaces,
                              const String& text,
                              float fontSize,
                              float maxWidth,
                              scribe::TextDirection direction,
                              bool wrap = true) -> bool
        {
            scribe::LayoutOptions options{};
            options.fontSize = fontSize;
            options.wrap = wrap;
            options.maxWidth = maxWidth;
            options.direction = direction;
            return m_layoutEngine.LayoutText(out, blockFaces, text, options);
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
                                 bool pixelAlignCapHeight = false) -> SceneTextBlock*
        {
            SceneTextBlock& block = m_sceneBlocks.EmplaceBack();
            block.text = text;
            block.origin = origin;
            block.fontSize = fontSize;
            block.color = color;
            block.fontFaceIndex = fontFaceIndex;
            block.useAllFaces = useAllFaces;
            block.pixelAlignBaseline = pixelAlignBaseline;
            block.pixelAlignCapHeight = pixelAlignCapHeight;

            const Span<const scribe::LoadedFontFaceBlob> blockFaces = useAllFaces
                ? faceSpan
                : Span<const scribe::LoadedFontFaceBlob>(&faces[Min(fontFaceIndex, faceCount - 1)], 1);

            if (!layoutText(block.layout, blockFaces, block.text, fontSize, maxWidth, direction))
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

                if (!addSceneBlock("Feature", { 0.0f, 0.0f }, sectionLabelSize, leftWidth, false, scribe::TextDirection::LeftToRight, mutedColor)
                    || !addSceneBlock("Disabled / current gap", { disabledX, 0.0f }, sectionLabelSize, sampleWidth, false, scribe::TextDirection::LeftToRight, mutedColor)
                    || !addSceneBlock("Enabled / target", { enabledX, 0.0f }, sectionLabelSize, sampleWidth, false, scribe::TextDirection::LeftToRight, mutedColor))
                {
                    return false;
                }

                float rowY = 46.0f * dpiScale;
                for (const FeatureRowSpec& row : FeatureRows)
                {
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
                    SceneTextBlock* disabledBlock = addSceneBlock(
                        row.disabledSample,
                        { disabledX, rowY + (4.0f * dpiScale) },
                        sampleSize,
                        sampleWidth,
                        true,
                        row.disabledDirection);
                    SceneTextBlock* enabledBlock = addSceneBlock(
                        row.enabledSample,
                        { enabledX, rowY + (4.0f * dpiScale) },
                        sampleSize,
                        sampleWidth,
                        true,
                        row.enabledDirection);
                    if (!descriptionBlock || !disabledBlock || !enabledBlock)
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
                        "Rows marked as not yet implemented stay visible here so the feature scene doubles as a roadmap.",
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

        m_hasCaret = false;
        m_bodyFontSize = bodyFontSize;

        if (!PrimeGlyphCache())
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
            GetSceneMissingGlyphCount(),
            GetSceneFallbackGlyphCount());

        m_renderStatsText.Clear();
        FormatTo(
            m_renderStatsText,
            "FPS: {:.1f}\n"
            "GPU text: {:.2f} ms\n"
            "Draws: {}\n"
            "Resolution: {} x {}",
            fps,
            m_lastGpuFrameMs,
            m_lastDrawCount,
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
        uint32_t sceneStatsVertexCount = 0;
        uint32_t renderStatsVertexCount = 0;
        uint32_t inputHintsVertexCount = 0;
        if (!PrimeLayoutGlyphs(m_sceneStatsLayout, sceneStatsVertexCount)
            || !PrimeLayoutGlyphs(m_renderStatsLayout, renderStatsVertexCount)
            || !PrimeLayoutGlyphs(m_inputHintsLayout, inputHintsVertexCount))
        {
            return false;
        }

        m_overlayVertexEstimate = sceneStatsVertexCount + renderStatsVertexCount + inputHintsVertexCount;
        return true;
    }

    bool ScribeTestApp::PrimeLayoutGlyphs(const scribe::LayoutResult& layout, uint32_t& outVertexCount)
    {
        outVertexCount = 0;

        for (const scribe::ShapedGlyph& glyph : layout.glyphs)
        {
            if (glyph.fontFaceIndex >= m_fonts.Size())
            {
                continue;
            }

            const FontGlyphCacheState& cacheState = m_fontGlyphCache[glyph.fontFaceIndex];
            if (cacheState.hasColorGlyphs && (glyph.glyphIndex < cacheState.colorGlyphRanges.Size()))
            {
                const FontGlyphCacheState::ColorGlyphRange range = cacheState.colorGlyphRanges[glyph.glyphIndex];
                if (range.layerCount > 0)
                {
                    for (uint32_t layerIndex = 0; layerIndex < range.layerCount; ++layerIndex)
                    {
                        const FontGlyphCacheState::CachedColorGlyphLayer& layer = cacheState.colorGlyphLayers[range.firstLayer + layerIndex];
                        const scribe::GlyphResource* glyphResource = nullptr;
                        if (!EnsureGlyphResource(glyph.fontFaceIndex, layer.glyphIndex, glyphResource))
                        {
                            continue;
                        }

                        outVertexCount += glyphResource->vertexCount;
                    }
                    continue;
                }
            }

            const scribe::GlyphResource* glyphResource = nullptr;
            if (!EnsureGlyphResource(glyph.fontFaceIndex, glyph.glyphIndex, glyphResource))
            {
                continue;
            }

            outVertexCount += glyphResource->vertexCount;
        }

        return true;
    }

    bool ScribeTestApp::PrimeGlyphCache()
    {
        uint32_t titleVertexCount = 0;
        uint32_t bodyVertexCount = 0;
        uint32_t sceneBlockVertexCount = 0;
        if (!PrimeLayoutGlyphs(m_titleLayout, titleVertexCount)
            || !PrimeLayoutGlyphs(m_bodyLayout, bodyVertexCount)
            || !PrimeSceneBlocks(sceneBlockVertexCount))
        {
            return false;
        }

        m_sceneVertexEstimate = titleVertexCount + bodyVertexCount + sceneBlockVertexCount;
        return true;
    }

    bool ScribeTestApp::PrimeSceneBlocks(uint32_t& outVertexCount)
    {
        outVertexCount = 0;

        for (const SceneTextBlock& block : m_sceneBlocks)
        {
            uint32_t blockVertexCount = 0;
            if (!PrimeLayoutGlyphs(block.layout, blockVertexCount))
            {
                return false;
            }

            outVertexCount += blockVertexCount;
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

        if ((!block.pixelAlignBaseline && !block.pixelAlignCapHeight)
            || (Abs(m_sceneZoom - 1.0f) > 0.001f)
            || block.layout.lines.IsEmpty()
            || (block.fontFaceIndex >= m_fonts.Size())
            || !m_fonts[block.fontFaceIndex].blob.metadata.IsValid())
        {
            return origin;
        }

        const auto metrics = m_fonts[block.fontFaceIndex].blob.metadata.GetMetrics();
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

        if (fontFaceIndex >= m_fonts.Size())
        {
            return false;
        }

        if (fontFaceIndex < m_fontGlyphCache.Size())
        {
            FontGlyphCacheState& cacheState = m_fontGlyphCache[fontFaceIndex];
            if (glyphIndex < cacheState.glyphResourceIndices.Size())
            {
                const int32_t cachedIndex = cacheState.glyphResourceIndices[glyphIndex];
                if ((cachedIndex >= 0) && (static_cast<uint32_t>(cachedIndex) < m_cachedGlyphs.Size()))
                {
                    out = &m_cachedGlyphs[static_cast<uint32_t>(cachedIndex)].resource;
                    return true;
                }
            }
        }

        const uint32_t cacheIndex = m_cachedGlyphs.Size();
        CachedGlyph& cached = m_cachedGlyphs.EmplaceBack();
        cached.fontFaceIndex = fontFaceIndex;
        cached.glyphIndex = glyphIndex;
        if (!m_renderer.CreateCompiledGlyphResource(cached.resource, m_fonts[fontFaceIndex].blob, glyphIndex))
        {
            m_cachedGlyphs.PopBack();
            return false;
        }

        if (fontFaceIndex < m_fontGlyphCache.Size())
        {
            FontGlyphCacheState& cacheState = m_fontGlyphCache[fontFaceIndex];
            if (glyphIndex >= cacheState.glyphResourceIndices.Size())
            {
                cacheState.glyphResourceIndices.Resize(glyphIndex + 1, int32_t(-1));
            }

            cacheState.glyphResourceIndices[glyphIndex] = static_cast<int32_t>(cacheIndex);
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

    void ScribeTestApp::QueueDraw(const scribe::DrawGlyphDesc& desc)
    {
        m_renderer.QueueDraw(desc);
    }

    void ScribeTestApp::QueueLayout(
        const scribe::LayoutResult& layout,
        const Vec2f& origin,
        float fontSize,
        float layoutScale,
        const Vec4f& foregroundColor)
    {
        struct FontQueueState
        {
            float scale{ 0.0f };
            bool hasColorGlyphs{ false };
        };

        Vector<FontQueueState> fontQueueStates{};
        fontQueueStates.Resize(m_fonts.Size(), DefaultInit);
        for (uint32_t fontIndex = 0; fontIndex < m_fonts.Size(); ++fontIndex)
        {
            FontQueueState& fontQueueState = fontQueueStates[fontIndex];
            if (fontIndex >= m_fontGlyphCache.Size())
            {
                continue;
            }

            const LoadedDemoFont& font = m_fonts[fontIndex];
            const uint32_t unitsPerEm = Max(font.blob.metadata.GetMetrics().GetUnitsPerEm(), 1u);
            fontQueueState.scale = (fontSize / static_cast<float>(unitsPerEm)) * layoutScale;
            fontQueueState.hasColorGlyphs = m_fontGlyphCache[fontIndex].hasColorGlyphs;
        }

        for (const scribe::ShapedGlyph& glyph : layout.glyphs)
        {
            if (glyph.fontFaceIndex >= m_fonts.Size())
            {
                continue;
            }

            const FontQueueState& fontQueueState = fontQueueStates[glyph.fontFaceIndex];
            const FontGlyphCacheState& cacheState = m_fontGlyphCache[glyph.fontFaceIndex];
            const float scale = fontQueueState.scale;
            const Vec2f position{
                origin.x + (glyph.position.x * layoutScale),
                origin.y + (glyph.position.y * layoutScale)
            };

            if (fontQueueState.hasColorGlyphs && (glyph.glyphIndex < cacheState.colorGlyphRanges.Size()))
            {
                const FontGlyphCacheState::ColorGlyphRange range = cacheState.colorGlyphRanges[glyph.glyphIndex];
                if (range.layerCount > 0)
                {
                    for (uint32_t layerIndex = 0; layerIndex < range.layerCount; ++layerIndex)
                    {
                        const FontGlyphCacheState::CachedColorGlyphLayer& layer = cacheState.colorGlyphLayers[range.firstLayer + layerIndex];
                        const scribe::GlyphResource* glyphResource = nullptr;
                        if (!EnsureGlyphResource(glyph.fontFaceIndex, layer.glyphIndex, glyphResource))
                        {
                            continue;
                        }

                        scribe::DrawGlyphDesc desc{};
                        desc.glyph = glyphResource;
                        desc.position = position;
                        desc.size = { scale, scale };
                        desc.color = layer.useForegroundColor ? foregroundColor : layer.color;
                        desc.color.w *= layer.useForegroundColor ? layer.color.w : 1.0f;
                        desc.basisX = layer.basisX;
                        desc.basisY = layer.basisY;
                        desc.offset = layer.offset;
                        QueueDraw(desc);
                    }
                    continue;
                }
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
            QueueDraw(desc);
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
            desc.color = { 1.0f, 1.0f, 1.0f, 1.0f };
            QueueDraw(desc);
        }
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
        QueueDraw(desc);
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
