// Copyright Chad Engler

#include "he/scribe/retained_text.h"

#include "he/scribe/compiled_font.h"
#include "resource_key_utils.h"

#include "he/core/math.h"
#include "he/schema/layout.h"

namespace he::scribe
{
    namespace
    {
        FontFaceResourceReader CloneFontFaceResource(Vector<schema::Word>& storage, const FontFaceResourceReader& source)
        {
            schema::Builder builder{};
            FontFaceResource::Builder root = builder.AddStruct<FontFaceResource>();

            const FontFaceShapingData::Reader sourceShaping = source.GetShaping();
            FontFaceShapingData::Builder shaping = root.GetShaping();
            shaping.SetFaceIndex(sourceShaping.GetFaceIndex());
            const schema::Blob::Reader sourceBytes = sourceShaping.GetSourceBytes();
            shaping.SetSourceBytes(builder.AddBlob(Span<const uint8_t>(
                reinterpret_cast<const uint8_t*>(sourceBytes.Data()),
                sourceBytes.Size())));

            const schema::Blob::Reader curveData = source.GetCurveData();
            root.SetCurveData(builder.AddBlob(Span<const uint8_t>(
                reinterpret_cast<const uint8_t*>(curveData.Data()),
                curveData.Size())));

            const schema::Blob::Reader bandData = source.GetBandData();
            root.SetBandData(builder.AddBlob(Span<const uint8_t>(
                reinterpret_cast<const uint8_t*>(bandData.Data()),
                bandData.Size())));

            const FontFacePaintData::Reader sourcePaint = source.GetPaint();
            FontFacePaintData::Builder paint = root.GetPaint();
            paint.SetDefaultPaletteIndex(sourcePaint.GetDefaultPaletteIndex());

            const schema::List<FontFacePalette>::Reader sourcePalettes = sourcePaint.GetPalettes();
            schema::List<FontFacePalette>::Builder palettes = paint.InitPalettes(sourcePalettes.Size());
            for (uint32_t paletteIndex = 0; paletteIndex < sourcePalettes.Size(); ++paletteIndex)
            {
                const FontFacePalette::Reader sourcePalette = sourcePalettes[paletteIndex];
                FontFacePalette::Builder palette = palettes[paletteIndex];
                palette.SetBackground(sourcePalette.GetBackground());

                const schema::List<FontFacePaletteColor>::Reader sourceColors = sourcePalette.GetColors();
                schema::List<FontFacePaletteColor>::Builder colors = palette.InitColors(sourceColors.Size());
                for (uint32_t colorIndex = 0; colorIndex < sourceColors.Size(); ++colorIndex)
                {
                    const FontFacePaletteColor::Reader sourceColor = sourceColors[colorIndex];
                    FontFacePaletteColor::Builder color = colors[colorIndex];
                    color.SetRed(sourceColor.GetRed());
                    color.SetGreen(sourceColor.GetGreen());
                    color.SetBlue(sourceColor.GetBlue());
                    color.SetAlpha(sourceColor.GetAlpha());
                }
            }

            const schema::List<FontFaceColorGlyph>::Reader sourceColorGlyphs = sourcePaint.GetColorGlyphs();
            schema::List<FontFaceColorGlyph>::Builder colorGlyphs = paint.InitColorGlyphs(sourceColorGlyphs.Size());
            for (uint32_t glyphIndex = 0; glyphIndex < sourceColorGlyphs.Size(); ++glyphIndex)
            {
                const FontFaceColorGlyph::Reader sourceColorGlyph = sourceColorGlyphs[glyphIndex];
                FontFaceColorGlyph::Builder colorGlyph = colorGlyphs[glyphIndex];
                colorGlyph.SetFirstLayer(sourceColorGlyph.GetFirstLayer());
                colorGlyph.SetLayerCount(sourceColorGlyph.GetLayerCount());
            }

            const schema::List<FontFaceColorGlyphLayer>::Reader sourceLayers = sourcePaint.GetLayers();
            schema::List<FontFaceColorGlyphLayer>::Builder layers = paint.InitLayers(sourceLayers.Size());
            for (uint32_t layerIndex = 0; layerIndex < sourceLayers.Size(); ++layerIndex)
            {
                const FontFaceColorGlyphLayer::Reader sourceLayer = sourceLayers[layerIndex];
                FontFaceColorGlyphLayer::Builder layer = layers[layerIndex];
                layer.SetGlyphIndex(sourceLayer.GetGlyphIndex());
                layer.SetPaletteEntryIndex(sourceLayer.GetPaletteEntryIndex());
                layer.SetColorSource(sourceLayer.GetColorSource());
                layer.SetAlphaScale(sourceLayer.GetAlphaScale());
                layer.SetTransform00(sourceLayer.GetTransform00());
                layer.SetTransform01(sourceLayer.GetTransform01());
                layer.SetTransform10(sourceLayer.GetTransform10());
                layer.SetTransform11(sourceLayer.GetTransform11());
                layer.SetTransformTx(sourceLayer.GetTransformTx());
                layer.SetTransformTy(sourceLayer.GetTransformTy());
            }

            const FontFaceRuntimeMetadata::Reader sourceMetadata = source.GetMetadata();
            FontFaceRuntimeMetadata::Builder metadata = root.GetMetadata();
            metadata.SetGlyphCount(sourceMetadata.GetGlyphCount());
            metadata.SetUnitsPerEm(sourceMetadata.GetUnitsPerEm());
            metadata.SetAscender(sourceMetadata.GetAscender());
            metadata.SetDescender(sourceMetadata.GetDescender());
            metadata.SetLineHeight(sourceMetadata.GetLineHeight());
            metadata.SetCapHeight(sourceMetadata.GetCapHeight());
            metadata.SetHasColorGlyphs(sourceMetadata.GetHasColorGlyphs());

            const FontFaceRenderData::Reader sourceRender = source.GetRender();
            FontFaceRenderData::Builder render = root.GetRender();
            render.SetCurveTextureWidth(sourceRender.GetCurveTextureWidth());
            render.SetCurveTextureHeight(sourceRender.GetCurveTextureHeight());
            render.SetBandTextureWidth(sourceRender.GetBandTextureWidth());
            render.SetBandTextureHeight(sourceRender.GetBandTextureHeight());
            render.SetBandOverlapEpsilon(sourceRender.GetBandOverlapEpsilon());

            const schema::List<FontFaceGlyphRenderData>::Reader sourceGlyphs = sourceRender.GetGlyphs();
            schema::List<FontFaceGlyphRenderData>::Builder glyphs = render.InitGlyphs(sourceGlyphs.Size());
            for (uint32_t glyphIndex = 0; glyphIndex < sourceGlyphs.Size(); ++glyphIndex)
            {
                const FontFaceGlyphRenderData::Reader sourceGlyph = sourceGlyphs[glyphIndex];
                FontFaceGlyphRenderData::Builder glyph = glyphs[glyphIndex];
                glyph.SetAdvanceX(sourceGlyph.GetAdvanceX());
                glyph.SetAdvanceY(sourceGlyph.GetAdvanceY());
                glyph.SetBoundsMinX(sourceGlyph.GetBoundsMinX());
                glyph.SetBoundsMinY(sourceGlyph.GetBoundsMinY());
                glyph.SetBoundsMaxX(sourceGlyph.GetBoundsMaxX());
                glyph.SetBoundsMaxY(sourceGlyph.GetBoundsMaxY());
                glyph.SetBandScaleX(sourceGlyph.GetBandScaleX());
                glyph.SetBandScaleY(sourceGlyph.GetBandScaleY());
                glyph.SetBandOffsetX(sourceGlyph.GetBandOffsetX());
                glyph.SetBandOffsetY(sourceGlyph.GetBandOffsetY());
                glyph.SetGlyphBandLocX(sourceGlyph.GetGlyphBandLocX());
                glyph.SetGlyphBandLocY(sourceGlyph.GetGlyphBandLocY());
                glyph.SetBandMaxX(sourceGlyph.GetBandMaxX());
                glyph.SetBandMaxY(sourceGlyph.GetBandMaxY());
                glyph.SetFillRule(sourceGlyph.GetFillRule());
                glyph.SetHasGeometry(sourceGlyph.GetHasGeometry());
                glyph.SetHasColorLayers(sourceGlyph.GetHasColorLayers());
            }

            builder.SetRoot(root);
            storage = Span<const schema::Word>(builder);
            return schema::ReadRoot<FontFaceResource>(storage.Data());
        }

        Vec4f ToVec4f(FontFacePaletteColor::Reader color)
        {
            return {
                color.GetRed(),
                color.GetGreen(),
                color.GetBlue(),
                color.GetAlpha()
            };
        }

        Vec4f MultiplyColor(const Vec4f& a, const Vec4f& b)
        {
            return {
                a.x * b.x,
                a.y * b.y,
                a.z * b.z,
                a.w * b.w
            };
        }

        bool IsWhiteColor(const Vec4f& color)
        {
            constexpr float Epsilon = 1.0e-4f;
            return (Abs(color.x - 1.0f) <= Epsilon)
                && (Abs(color.y - 1.0f) <= Epsilon)
                && (Abs(color.z - 1.0f) <= Epsilon)
                && (Abs(color.w - 1.0f) <= Epsilon);
        }

        const TextStyle* GetStyle(Span<const TextStyle> styles, uint32_t styleIndex)
        {
            if ((styleIndex == InvalidTextStyleIndex) || (styleIndex >= styles.Size()))
            {
                return nullptr;
            }

            return &styles[styleIndex];
        }

        void ApplyStyleTransform(
            RetainedTextQuad& quad,
            const TextStyle* style,
            float fontSize)
        {
            if (!style)
            {
                return;
            }

            const float angle = style->rotationRadians;
            const float cosAngle = Cos(angle);
            const float sinAngle = Sin(angle);

            quad.position.y -= style->baselineShiftEm * fontSize;
            quad.size.x *= Max(style->glyphScale * style->stretchX, 0.001f);
            quad.size.y *= Max(style->glyphScale * style->stretchY, 0.001f);

            quad.basisX = { cosAngle, sinAngle };
            quad.basisY = {
                (cosAngle * style->skewX) - sinAngle,
                (sinAngle * style->skewX) + cosAngle
            };
        }

        void ApplyStyleTransform(
            RetainedTextDraw& draw,
            const TextStyle* style,
            float fontSize)
        {
            if (!style)
            {
                return;
            }

            const float angle = style->rotationRadians;
            const float cosAngle = Cos(angle);
            const float sinAngle = Sin(angle);

            draw.position.y -= style->baselineShiftEm * fontSize;
            draw.size.x *= Max(style->glyphScale * style->stretchX, 0.001f);
            draw.size.y *= Max(style->glyphScale * style->stretchY, 0.001f);

            draw.basisX = { cosAngle, sinAngle };
            draw.basisY = {
                (cosAngle * style->skewX) - sinAngle,
                (sinAngle * style->skewX) + cosAngle
            };
        }

        void AppendDecorationQuad(
            Vector<RetainedTextQuad>& out,
            uint32_t& estimatedVertexCount,
            const TextCluster& cluster,
            const TextLine& line,
            const TextStyle* style,
            float fontSize,
            bool underline)
        {
            if (!style || cluster.isWhitespace)
            {
                return;
            }

            RetainedTextQuad& quad = out.EmplaceBack();
            const float thickness = Max(style->decorationThicknessEm * fontSize, 1.0f);
            const float offset = (underline ? style->underlineOffsetEm : -style->strikethroughOffsetEm) * fontSize;
            quad.position = {
                cluster.x0,
                line.baselineY + offset - (thickness * 0.5f)
            };
            quad.size = {
                Max(cluster.x1 - cluster.x0, 0.0f),
                thickness
            };
            quad.color = style->decorationColor;
            ApplyStyleTransform(quad, style, fontSize);
            estimatedVertexCount += 6;
        }

        void AppendEffectDraws(
            Vector<RetainedTextDraw>& out,
            uint32_t& estimatedVertexCount,
            const RetainedTextDraw& source,
            const TextStyle* style,
            float fontSize)
        {
            if (!style)
            {
                return;
            }

            if (HasAnyFlags(style->effects, TextEffectFlags::Shadow) && (style->shadowColor.w > 0.0f))
            {
                RetainedTextDraw& shadow = out.EmplaceBack(source);
                shadow.flags = 0;
                shadow.color = style->shadowColor;
                shadow.position.x += style->shadowOffsetEm.x * fontSize;
                shadow.position.y += style->shadowOffsetEm.y * fontSize;
                estimatedVertexCount += ScribeGlyphVertexCount;
            }

            if (HasAnyFlags(style->effects, TextEffectFlags::Outline)
                && (style->outlineWidthEm > 0.0f)
                && (style->outlineColor.w > 0.0f))
            {
                const float outlineRadius = style->outlineWidthEm * fontSize;
                static const Vec2f Offsets[] =
                {
                    { -1.0f, 0.0f },
                    { 1.0f, 0.0f },
                    { 0.0f, -1.0f },
                    { 0.0f, 1.0f },
                    { -0.70710678f, -0.70710678f },
                    { 0.70710678f, -0.70710678f },
                    { -0.70710678f, 0.70710678f },
                    { 0.70710678f, 0.70710678f },
                };

                out.Reserve(out.Size() + HE_LENGTH_OF(Offsets));
                for (const Vec2f& offset : Offsets)
                {
                    RetainedTextDraw& outline = out.EmplaceBack(source);
                    outline.flags = 0;
                    outline.color = style->outlineColor;
                    outline.position.x += offset.x * outlineRadius;
                    outline.position.y += offset.y * outlineRadius;
                    estimatedVertexCount += ScribeGlyphVertexCount;
                }
            }
        }
    }

    bool RetainedTextModel::Build(const RetainedTextBuildDesc& desc)
    {
        Clear();

        if ((desc.layout == nullptr) || (desc.fontFaces.IsEmpty()) || (desc.fontSize <= 0.0f))
        {
            return false;
        }

        const TextStyle defaultStyle{};
        Span<const TextStyle> styles = desc.styles;
        if (styles.IsEmpty())
        {
            styles = Span<const TextStyle>(&defaultStyle, 1);
        }

        struct FontBuildState
        {
            float scale{ 0.0f };
            uint32_t paletteIndex{ 0 };
            bool hasColorGlyphs{ false };
        };

        Vector<FontBuildState> fontStates{};
        fontStates.Resize(desc.fontFaces.Size(), DefaultInit);
        m_fontFaceStorage.Resize(desc.fontFaces.Size(), DefaultInit);
        m_fontFaces.Resize(desc.fontFaces.Size(), DefaultInit);
        m_fontFaceHashes.Resize(desc.fontFaces.Size(), DefaultInit);
        for (uint32_t fontIndex = 0; fontIndex < desc.fontFaces.Size(); ++fontIndex)
        {
            const FontFaceResourceReader& fontFace = desc.fontFaces[fontIndex];
            m_fontFaces[fontIndex] = CloneFontFaceResource(m_fontFaceStorage[fontIndex], fontFace);
            m_fontFaceHashes[fontIndex] = HashFontFaceGeometryResource(m_fontFaces[fontIndex]);

            const FontFaceRuntimeMetadata::Reader metadata = m_fontFaces[fontIndex].GetMetadata();
            const FontFacePaintData::Reader paint = m_fontFaces[fontIndex].GetPaint();
            const uint32_t unitsPerEm = Max(metadata.GetUnitsPerEm(), 1u);
            FontBuildState& fontState = fontStates[fontIndex];
            fontState.scale = desc.fontSize / static_cast<float>(unitsPerEm);
            fontState.hasColorGlyphs =
                metadata.IsValid()
                && metadata.GetHasColorGlyphs()
                && paint.IsValid()
                && !paint.GetPalettes().IsEmpty();
            if (fontState.hasColorGlyphs)
            {
                fontState.paletteIndex = SelectCompiledFontPalette(m_fontFaces[fontIndex], desc.darkBackgroundPreferred);
            }
        }

        m_draws.Reserve(desc.layout->glyphs.Size());
        for (const ShapedGlyph& glyph : desc.layout->glyphs)
        {
            if (glyph.fontFaceIndex >= desc.fontFaces.Size())
            {
                continue;
            }

            const FontFaceResourceReader& fontFace = m_fontFaces[glyph.fontFaceIndex];
            const FontBuildState& fontState = fontStates[glyph.fontFaceIndex];
            const TextStyle* style = GetStyle(styles, glyph.styleIndex);
            if (fontState.hasColorGlyphs)
            {
                const FontFacePaintData::Reader paint = fontFace.GetPaint();
                const schema::List<FontFaceColorGlyph>::Reader colorGlyphs = paint.GetColorGlyphs();
                if (glyph.glyphIndex < colorGlyphs.Size())
                {
                    const FontFaceColorGlyph::Reader colorGlyph = colorGlyphs[glyph.glyphIndex];
                    const uint32_t layerCount = colorGlyph.GetLayerCount();
                    if (layerCount > 0)
                    {
                        const FontFacePalette::Reader palette = paint.GetPalettes()[fontState.paletteIndex];
                        const schema::List<FontFacePaletteColor>::Reader colors = palette.GetColors();
                        const schema::List<FontFaceColorGlyphLayer>::Reader layers = paint.GetLayers();

                        m_draws.Reserve(m_draws.Size() + layerCount);
                        for (uint32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex)
                        {
                            const FontFaceColorGlyphLayer::Reader layer = layers[colorGlyph.GetFirstLayer() + layerIndex];

                            RetainedTextDraw draw{};
                            draw.fontFaceIndex = glyph.fontFaceIndex;
                            draw.glyphIndex = layer.GetGlyphIndex();
                            draw.position = glyph.position;
                            draw.size = { fontState.scale, fontState.scale };
                            draw.basisX = { layer.GetTransform00(), layer.GetTransform10() };
                            draw.basisY = { layer.GetTransform01(), layer.GetTransform11() };
                            draw.offset = { layer.GetTransformTx(), layer.GetTransformTy() };
                            draw.flags = 0;
                            draw.color = { 1.0f, 1.0f, 1.0f, Max(layer.GetAlphaScale(), 0.0f) };

                            if (layer.GetColorSource() == FontFaceColorSource::Foreground)
                            {
                                draw.flags |= RetainedTextDrawFlagUseForegroundColor;
                            }
                            else
                            {
                                const uint32_t paletteEntryIndex = layer.GetPaletteEntryIndex();
                                if (paletteEntryIndex < colors.Size())
                                {
                                    draw.color = ToVec4f(colors[paletteEntryIndex]);
                                    draw.color.w *= Max(layer.GetAlphaScale(), 0.0f);
                                }
                            }

                            if (style)
                            {
                                if (layer.GetColorSource() == FontFaceColorSource::Foreground)
                                {
                                    draw.color = MultiplyColor(draw.color, style->color);
                                }

                                ApplyStyleTransform(draw, style, desc.fontSize);
                            }

                            AppendEffectDraws(m_draws, m_estimatedVertexCount, draw, style, desc.fontSize);
                            m_draws.PushBack(draw);
                            m_estimatedVertexCount += ScribeGlyphVertexCount;
                        }
                        continue;
                    }
                }
            }

            RetainedTextDraw draw{};
            draw.fontFaceIndex = glyph.fontFaceIndex;
            draw.glyphIndex = glyph.glyphIndex;
            draw.flags = (!style || IsWhiteColor(style->color)) ? RetainedTextDrawFlagUseForegroundColor : 0u;
            draw.position = glyph.position;
            draw.size = { fontState.scale, fontState.scale };
            draw.color = style ? style->color : Vec4f{ 1.0f, 1.0f, 1.0f, 1.0f };
            ApplyStyleTransform(draw, style, desc.fontSize);
            AppendEffectDraws(m_draws, m_estimatedVertexCount, draw, style, desc.fontSize);
            m_draws.PushBack(draw);
            m_estimatedVertexCount += ScribeGlyphVertexCount;
        }

        m_quads.Reserve(desc.layout->clusters.Size());
        for (const TextCluster& cluster : desc.layout->clusters)
        {
            const TextStyle* style = GetStyle(styles, cluster.styleIndex);
            if (!style || (cluster.lineIndex >= desc.layout->lines.Size()))
            {
                continue;
            }

            const TextLine& line = desc.layout->lines[cluster.lineIndex];
            if (HasAnyFlags(style->decorations, TextDecorationFlags::Underline))
            {
                AppendDecorationQuad(m_quads, m_estimatedVertexCount, cluster, line, style, desc.fontSize, true);
            }

            if (HasAnyFlags(style->decorations, TextDecorationFlags::Strikethrough))
            {
                AppendDecorationQuad(m_quads, m_estimatedVertexCount, cluster, line, style, desc.fontSize, false);
            }
        }

        return true;
    }

    void RetainedTextModel::Clear()
    {
        m_fontFaceStorage.Clear();
        m_fontFaces.Clear();
        m_fontFaceHashes.Clear();
        m_draws.Clear();
        m_quads.Clear();
        m_estimatedVertexCount = 0;
    }

    const FontFaceResourceReader* RetainedTextModel::GetFontFace(uint32_t fontFaceIndex) const
    {
        return fontFaceIndex < m_fontFaces.Size() ? &m_fontFaces[fontFaceIndex] : nullptr;
    }

    uint64_t RetainedTextModel::GetFontFaceHash(uint32_t fontFaceIndex) const
    {
        return fontFaceIndex < m_fontFaceHashes.Size() ? m_fontFaceHashes[fontFaceIndex] : 0;
    }
}
