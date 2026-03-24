// Copyright Chad Engler

#include "he/scribe/retained_text.h"

#include "he/scribe/compiled_font.h"

#include "he/core/math.h"

namespace he::scribe
{
    namespace
    {
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
        m_fontFaces.Resize(desc.fontFaces.Size(), DefaultInit);
        for (uint32_t fontIndex = 0; fontIndex < desc.fontFaces.Size(); ++fontIndex)
        {
            const FontFaceResourceReader& fontFace = desc.fontFaces[fontIndex];
            m_fontFaces[fontIndex] = fontFace;

            const FontFaceImportMetadata::Reader metadata = fontFace.GetMetadata();
            const FontFacePaintData::Reader paint = fontFace.GetPaint();
            const uint32_t unitsPerEm = Max(metadata.GetMetrics().GetUnitsPerEm(), 1u);
            FontBuildState& fontState = fontStates[fontIndex];
            fontState.scale = desc.fontSize / static_cast<float>(unitsPerEm);
            fontState.hasColorGlyphs =
                metadata.IsValid()
                && metadata.GetHasColorGlyphs()
                && paint.IsValid()
                && !paint.GetPalettes().IsEmpty();
            if (fontState.hasColorGlyphs)
            {
                fontState.paletteIndex = SelectCompiledFontPalette(fontFace, desc.darkBackgroundPreferred);
            }
        }

        m_draws.Reserve(desc.layout->glyphs.Size());
        for (const ShapedGlyph& glyph : desc.layout->glyphs)
        {
            if (glyph.fontFaceIndex >= desc.fontFaces.Size())
            {
                continue;
            }

            const FontFaceResourceReader& fontFace = desc.fontFaces[glyph.fontFaceIndex];
            const FontBuildState& fontState = fontStates[glyph.fontFaceIndex];
            const TextStyle* style = GetStyle(styles, glyph.styleIndex);
            if (fontState.hasColorGlyphs)
            {
                const FontFacePaintData::Reader paint = fontFace.GetPaint();
                const auto colorGlyphs = paint.GetColorGlyphs();
                if (glyph.glyphIndex < colorGlyphs.Size())
                {
                    const FontFaceColorGlyph::Reader colorGlyph = colorGlyphs[glyph.glyphIndex];
                    const uint32_t layerCount = colorGlyph.GetLayerCount();
                    if (layerCount > 0)
                    {
                        const auto palette = paint.GetPalettes()[fontState.paletteIndex];
                        const auto colors = palette.GetColors();
                        const auto layers = paint.GetLayers();

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
        m_fontFaces.Clear();
        m_draws.Clear();
        m_quads.Clear();
        m_estimatedVertexCount = 0;
    }

    const FontFaceResourceReader* RetainedTextModel::GetFontFace(uint32_t fontFaceIndex) const
    {
        return fontFaceIndex < m_fontFaces.Size() ? &m_fontFaces[fontFaceIndex] : nullptr;
    }
}
