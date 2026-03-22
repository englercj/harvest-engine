// Copyright Chad Engler

#include "he/scribe/runtime_blob.h"

#include "he/core/log.h"

namespace he::scribe
{
    namespace
    {
        template <typename T>
        bool ReadNestedSchema(T& out, schema::Blob::Reader blob)
        {
            if (!blob.IsValid())
            {
                return false;
            }

            const Span<const uint8_t> bytes = blob;
            if (bytes.IsEmpty())
            {
                return false;
            }

            if ((bytes.Size() % sizeof(schema::Word)) != 0)
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Nested schema blob is not word aligned."),
                    HE_KV(size, bytes.Size()));
                return false;
            }

            const auto* words = reinterpret_cast<const schema::Word*>(bytes.Data());
            out = schema::ReadRoot<typename T::StructType>(words);
            return out.IsValid();
        }

        bool ValidateHeader(RuntimeBlobHeader::Reader header, RuntimeBlobKind kind)
        {
            if (!header.IsValid())
            {
                return false;
            }

            if (header.GetFormatVersion() != RuntimeBlobFormatVersion)
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Unsupported scribe runtime blob version."),
                    HE_KV(expected, RuntimeBlobFormatVersion),
                    HE_KV(actual, header.GetFormatVersion()));
                return false;
            }

            if (header.GetKind() != kind)
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Unexpected scribe runtime blob kind."),
                    HE_KV(expected, kind),
                    HE_KV(actual, header.GetKind()));
                return false;
            }

            return true;
        }

        bool ValidateFontFaceRenderData(const LoadedFontFaceBlob& blob)
        {
            if (!blob.render.IsValid())
            {
                return false;
            }

            const uint32_t curveWidth = blob.render.GetCurveTextureWidth();
            const uint32_t curveHeight = blob.render.GetCurveTextureHeight();
            const uint32_t bandWidth = blob.render.GetBandTextureWidth();
            const uint32_t bandHeight = blob.render.GetBandTextureHeight();

            if ((curveWidth == 0) || (curveHeight == 0) || (bandWidth == 0) || (bandHeight == 0))
            {
                HE_LOG_ERROR(he_scribe, HE_MSG("Scribe font render data has an invalid texture extent."));
                return false;
            }

            if (bandWidth != ScribeBandTextureWidth)
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Scribe font band texture width does not match shader assumptions."),
                    HE_KV(expected, ScribeBandTextureWidth),
                    HE_KV(actual, bandWidth));
                return false;
            }

            const auto curveBytes = blob.root.GetCurveData();
            const auto bandBytes = blob.root.GetBandData();
            const size_t expectedCurveBytes = static_cast<size_t>(curveWidth) * curveHeight * sizeof(PackedCurveTexel);
            const size_t expectedBandBytes = static_cast<size_t>(bandWidth) * bandHeight * sizeof(PackedBandTexel);
            if ((curveBytes.Size() != expectedCurveBytes) || (bandBytes.Size() != expectedBandBytes))
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Scribe font render payload size does not match metadata."),
                    HE_KV(expected_curve_bytes, expectedCurveBytes),
                    HE_KV(actual_curve_bytes, curveBytes.Size()),
                    HE_KV(expected_band_bytes, expectedBandBytes),
                    HE_KV(actual_band_bytes, bandBytes.Size()));
                return false;
            }

            return true;
        }

        bool ValidateFontFacePaintData(const LoadedFontFaceBlob& blob)
        {
            if (!blob.paint.IsValid())
            {
                return false;
            }

            const auto colorGlyphs = blob.paint.GetColorGlyphs();
            const auto layers = blob.paint.GetLayers();
            const auto palettes = blob.paint.GetPalettes();
            const auto renderGlyphs = blob.render.GetGlyphs();

            if ((colorGlyphs.Size() != 0) && (colorGlyphs.Size() != renderGlyphs.Size()))
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Scribe font color glyph table size does not match render glyph table."),
                    HE_KV(color_glyph_count, colorGlyphs.Size()),
                    HE_KV(render_glyph_count, renderGlyphs.Size()));
                return false;
            }

            if (blob.paint.GetDefaultPaletteIndex() >= palettes.Size() && !palettes.IsEmpty())
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Scribe font default palette index is out of range."),
                    HE_KV(default_palette_index, blob.paint.GetDefaultPaletteIndex()),
                    HE_KV(palette_count, palettes.Size()));
                return false;
            }

            for (uint32_t glyphIndex = 0; glyphIndex < colorGlyphs.Size(); ++glyphIndex)
            {
                const FontFaceColorGlyph::Reader colorGlyph = colorGlyphs[glyphIndex];
                const uint32_t firstLayer = colorGlyph.GetFirstLayer();
                const uint32_t layerCount = colorGlyph.GetLayerCount();
                if ((firstLayer + layerCount) > layers.Size())
                {
                    HE_LOG_ERROR(he_scribe,
                        HE_MSG("Scribe font color glyph layer span is out of range."),
                        HE_KV(glyph_index, glyphIndex),
                        HE_KV(first_layer, firstLayer),
                        HE_KV(layer_count, layerCount),
                        HE_KV(total_layers, layers.Size()));
                    return false;
                }

                for (uint32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex)
                {
                    const FontFaceColorGlyphLayer::Reader layer = layers[firstLayer + layerIndex];
                    if (layer.GetAlphaScale() < 0.0f)
                    {
                        HE_LOG_ERROR(he_scribe,
                            HE_MSG("Scribe font color glyph layer has a negative alpha scale."),
                            HE_KV(glyph_index, glyphIndex),
                            HE_KV(layer_index, firstLayer + layerIndex),
                            HE_KV(alpha_scale, layer.GetAlphaScale()));
                        return false;
                    }
                }
            }

            return true;
        }
    }

    bool LoadCompiledFontFaceBlob(LoadedFontFaceBlob& out, Span<const schema::Word> data)
    {
        out = {};

        if (data.IsEmpty())
        {
            return false;
        }

        out.root = schema::ReadRoot<CompiledFontFaceBlob>(data.Data());
        if (!out.root.IsValid())
        {
            return false;
        }

        if (!ValidateHeader(out.root.GetHeader(), RuntimeBlobKind::FontFace))
        {
            out = {};
            return false;
        }

        if (!ReadNestedSchema(out.shaping, out.root.GetShapingData())
            || !ReadNestedSchema(out.metadata, out.root.GetMetadataData())
            || !ReadNestedSchema(out.render, out.root.GetRenderData())
            || !ReadNestedSchema(out.paint, out.root.GetPaintData()))
        {
            out = {};
            return false;
        }

        if (!ValidateFontFaceRenderData(out)
            || !ValidateFontFacePaintData(out))
        {
            out = {};
            return false;
        }

        return true;
    }

    bool LoadCompiledFontFamilyBlob(LoadedFontFamilyBlob& out, Span<const schema::Word> data)
    {
        out = {};

        if (data.IsEmpty())
        {
            return false;
        }

        out.root = schema::ReadRoot<CompiledFontFamilyBlob>(data.Data());
        if (!out.root.IsValid())
        {
            return false;
        }

        if (!ValidateHeader(out.root.GetHeader(), RuntimeBlobKind::FontFamily))
        {
            out = {};
            return false;
        }

        if (!ReadNestedSchema(out.family, out.root.GetFamilyData()))
        {
            out = {};
            return false;
        }

        return true;
    }

    bool LoadCompiledVectorImageBlob(LoadedVectorImageBlob& out, Span<const schema::Word> data)
    {
        out = {};

        if (data.IsEmpty())
        {
            return false;
        }

        out.root = schema::ReadRoot<CompiledVectorImageBlob>(data.Data());
        if (!out.root.IsValid())
        {
            return false;
        }

        if (!ValidateHeader(out.root.GetHeader(), RuntimeBlobKind::VectorImage))
        {
            out = {};
            return false;
        }

        if (!ReadNestedSchema(out.metadata, out.root.GetMetadataData()))
        {
            out = {};
            return false;
        }

        return true;
    }
}
