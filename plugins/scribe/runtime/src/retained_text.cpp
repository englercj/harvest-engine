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
    }

    bool RetainedTextModel::Build(const RetainedTextBuildDesc& desc)
    {
        Clear();

        if ((desc.layout == nullptr) || (desc.fontFaces.IsEmpty()) || (desc.fontSize <= 0.0f))
        {
            return false;
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
            const LoadedFontFaceBlob& fontFace = desc.fontFaces[fontIndex];
            m_fontFaces[fontIndex] = fontFace;

            const uint32_t unitsPerEm = Max(fontFace.metadata.GetMetrics().GetUnitsPerEm(), 1u);
            FontBuildState& fontState = fontStates[fontIndex];
            fontState.scale = desc.fontSize / static_cast<float>(unitsPerEm);
            fontState.hasColorGlyphs =
                fontFace.metadata.IsValid()
                && fontFace.metadata.GetHasColorGlyphs()
                && fontFace.paint.IsValid()
                && !fontFace.paint.GetPalettes().IsEmpty();
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

            const LoadedFontFaceBlob& fontFace = desc.fontFaces[glyph.fontFaceIndex];
            const FontBuildState& fontState = fontStates[glyph.fontFaceIndex];
            if (fontState.hasColorGlyphs)
            {
                const auto colorGlyphs = fontFace.paint.GetColorGlyphs();
                if (glyph.glyphIndex < colorGlyphs.Size())
                {
                    const FontFaceColorGlyph::Reader colorGlyph = colorGlyphs[glyph.glyphIndex];
                    const uint32_t layerCount = colorGlyph.GetLayerCount();
                    if (layerCount > 0)
                    {
                        const auto palette = fontFace.paint.GetPalettes()[fontState.paletteIndex];
                        const auto colors = palette.GetColors();
                        const auto layers = fontFace.paint.GetLayers();

                        m_draws.Reserve(m_draws.Size() + layerCount);
                        for (uint32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex)
                        {
                            const FontFaceColorGlyphLayer::Reader layer = layers[colorGlyph.GetFirstLayer() + layerIndex];

                            RetainedTextDraw& draw = m_draws.EmplaceBack();
                            draw.fontFaceIndex = glyph.fontFaceIndex;
                            draw.glyphIndex = layer.GetGlyphIndex();
                            draw.position = glyph.position;
                            draw.size = { fontState.scale, fontState.scale };
                            draw.basisX = { layer.GetTransform00(), layer.GetTransform10() };
                            draw.basisY = { layer.GetTransform01(), layer.GetTransform11() };
                            draw.offset = { layer.GetTransformTx(), layer.GetTransformTy() };
                            draw.flags = 0;
                            draw.color = { 1.0f, 1.0f, 1.0f, Max(layer.GetAlphaScale(), 0.0f) };

                            if ((layer.GetFlags() & CompiledFontColorLayerFlagUseForeground) != 0)
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

                            m_estimatedVertexCount += ScribeGlyphVertexCount;
                        }
                        continue;
                    }
                }
            }

            RetainedTextDraw& draw = m_draws.EmplaceBack();
            draw.fontFaceIndex = glyph.fontFaceIndex;
            draw.glyphIndex = glyph.glyphIndex;
            draw.flags = RetainedTextDrawFlagUseForegroundColor;
            draw.position = glyph.position;
            draw.size = { fontState.scale, fontState.scale };
            draw.color = { 1.0f, 1.0f, 1.0f, 1.0f };
            m_estimatedVertexCount += ScribeGlyphVertexCount;
        }

        return true;
    }

    void RetainedTextModel::Clear()
    {
        m_fontFaces.Clear();
        m_draws.Clear();
        m_estimatedVertexCount = 0;
    }

    const LoadedFontFaceBlob* RetainedTextModel::GetFontFace(uint32_t fontFaceIndex) const
    {
        return fontFaceIndex < m_fontFaces.Size() ? &m_fontFaces[fontFaceIndex] : nullptr;
    }
}
