// Copyright Chad Engler

#include "he/scribe/editor/font_face_compiler.h"

#include "font_compile_geometry.h"
#include "font_import_utils.h"

#include "he/scribe/runtime_blob.h"

#include "he/assets/types.h"
#include "he/assets/types_fmt.h"
#include "he/core/clock_fmt.h"
#include "he/core/log.h"
#include "he/core/result_fmt.h"
#include "he/core/stopwatch.h"

namespace he::scribe::editor
{
    bool FontFaceCompiler::Compile(const assets::CompileContext& ctx, [[maybe_unused]] assets::CompileResult& result)
    {
        constexpr assets::ResourceId ImportSourceId{ ScribeFontFace::ImportSourceResourceName };
        constexpr assets::ResourceId ImportMetadataId{ ScribeFontFace::ImportMetadataResourceName };
        constexpr assets::ResourceId RuntimeBlobId{ ScribeFontFace::RuntimeBlobResourceName };

        Vector<schema::Word> importSourceBytes;
        Result r = ctx.db.GetResource(importSourceBytes, ctx.asset.GetUuid(), ImportSourceId);
        if (!r)
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Failed to load scribe font import source resource."),
                HE_KV(asset_uuid, assets::AssetUuid(ctx.asset.GetUuid())),
                HE_KV(asset_name, ctx.asset.GetName()),
                HE_KV(resource_id, ImportSourceId),
                HE_KV(result, r));
            return false;
        }

        Vector<schema::Word> importMetadataBytes;
        r = ctx.db.GetResource(importMetadataBytes, ctx.asset.GetUuid(), ImportMetadataId);
        if (!r)
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Failed to load scribe font import metadata resource."),
                HE_KV(asset_uuid, assets::AssetUuid(ctx.asset.GetUuid())),
                HE_KV(asset_name, ctx.asset.GetName()),
                HE_KV(resource_id, ImportMetadataId),
                HE_KV(result, r));
            return false;
        }

        const ScribeFontFace::Reader asset = ctx.asset.GetData().TryGetStruct<ScribeFontFace>();
        if (!asset.IsValid())
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Scribe font asset data is invalid or missing."),
                HE_KV(asset_uuid, assets::AssetUuid(ctx.asset.GetUuid())),
                HE_KV(asset_name, ctx.asset.GetName()));
            return false;
        }

        const auto importSource = schema::ReadRoot<ScribeFontFace::ImportSourceResource>(importSourceBytes.Data());
        const auto importMetadata = schema::ReadRoot<ScribeFontFace::ImportMetadataResource>(importMetadataBytes.Data());
        if (!importSource.IsValid() || !importMetadata.IsValid())
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Scribe font import resources are not valid schema payloads."),
                HE_KV(asset_uuid, assets::AssetUuid(ctx.asset.GetUuid())),
                HE_KV(asset_name, ctx.asset.GetName()));
            return false;
        }

        schema::Builder shapingBuilder;
        FontFaceShapingData::Builder shaping = shapingBuilder.AddStruct<FontFaceShapingData>();
        shaping.SetFaceIndex(asset.GetFaceIndex());
        shaping.SetSourceFormat(importSource.GetSourceFormat());

        if (asset.GetPreserveSourceBytesForShaping())
        {
            const auto sourceBlob = importSource.GetSourceBytes();
            shaping.SetSourceBytes(shapingBuilder.AddBlob({ sourceBlob.Data(), sourceBlob.Size() }));
        }
        else
        {
            shaping.SetSourceBytes(shapingBuilder.AddBlob({}));
        }

        shapingBuilder.SetRoot(shaping);

        schema::Builder metadataBuilder;
        FontFaceImportMetadata::Builder metadata = metadataBuilder.AddStruct<FontFaceImportMetadata>();
        FillFontFaceImportMetadata(metadata, {
            asset.GetFaceIndex(),
            importSource.GetFaceCount(),
            importSource.GetSourceFormat(),
            String(importMetadata.GetMetadata().GetFamilyName()),
            String(importMetadata.GetMetadata().GetStyleName()),
            String(importMetadata.GetMetadata().GetPostscriptName()),
            importMetadata.GetMetadata().GetGlyphCount(),
            importMetadata.GetMetadata().GetMetrics().GetUnitsPerEm(),
            importMetadata.GetMetadata().GetMetrics().GetMaxAdvanceWidth(),
            importMetadata.GetMetadata().GetMetrics().GetMaxAdvanceHeight(),
            importMetadata.GetMetadata().GetMetrics().GetAscender(),
            importMetadata.GetMetadata().GetMetrics().GetDescender(),
            importMetadata.GetMetadata().GetMetrics().GetLineHeight(),
            importMetadata.GetMetadata().GetMetrics().GetCapHeight(),
            importMetadata.GetMetadata().GetIsScalable(),
            importMetadata.GetMetadata().GetHasColorGlyphs(),
            importMetadata.GetMetadata().GetHasKerning(),
            importMetadata.GetMetadata().GetHasHorizontalLayout(),
            importMetadata.GetMetadata().GetHasVerticalLayout()
        });
        metadataBuilder.SetRoot(metadata);

        CompiledFontRenderData renderData{};
        Stopwatch timer;
        {
            const auto sourceBlob = importSource.GetSourceBytes();
            if (!BuildCompiledFontRenderData(renderData, { sourceBlob.Data(), sourceBlob.Size() }, asset.GetFaceIndex()))
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Failed to build compiled scribe font render data."),
                    HE_KV(asset_uuid, assets::AssetUuid(ctx.asset.GetUuid())),
                    HE_KV(asset_name, ctx.asset.GetName()),
                    HE_KV(face_index, asset.GetFaceIndex()));
                return false;
            }
        }

        HE_LOG_INFO(he_scribe,
            HE_MSG("Compiled scribe font render data."),
            HE_KV(asset_uuid, assets::AssetUuid(ctx.asset.GetUuid())),
            HE_KV(asset_name, ctx.asset.GetName()),
            HE_KV(time, timer.Elapsed()),
            HE_KV(curve_texels, renderData.curveTexels.Size()),
            HE_KV(band_headers, renderData.bandHeaderCount),
            HE_KV(emitted_band_payload_texels, renderData.emittedBandPayloadTexelCount),
            HE_KV(reused_bands, renderData.reusedBandCount),
            HE_KV(reused_band_payload_texels, renderData.reusedBandPayloadTexelCount));

        schema::Builder renderBuilder;
        FontFaceRenderData::Builder render = renderBuilder.AddStruct<FontFaceRenderData>();
        render.SetCurveTextureWidth(renderData.curveTextureWidth);
        render.SetCurveTextureHeight(renderData.curveTextureHeight);
        render.SetBandTextureWidth(renderData.bandTextureWidth);
        render.SetBandTextureHeight(renderData.bandTextureHeight);
        render.SetBandOverlapEpsilon(renderData.bandOverlapEpsilon);

        auto glyphs = render.InitGlyphs(renderData.glyphs.Size());
        for (uint32_t glyphIndex = 0; glyphIndex < renderData.glyphs.Size(); ++glyphIndex)
        {
            const CompiledGlyphRenderEntry& srcGlyph = renderData.glyphs[glyphIndex];
            FontFaceGlyphRenderData::Builder dstGlyph = glyphs[glyphIndex];
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
        FontFacePaintData::Builder paint = paintBuilder.AddStruct<FontFacePaintData>();
        paint.SetDefaultPaletteIndex(renderData.paint.defaultPaletteIndex);

        auto palettes = paint.InitPalettes(renderData.paint.palettes.Size());
        for (uint32_t paletteIndex = 0; paletteIndex < renderData.paint.palettes.Size(); ++paletteIndex)
        {
            const CompiledFontPalette& srcPalette = renderData.paint.palettes[paletteIndex];
            FontFacePalette::Builder dstPalette = palettes[paletteIndex];
            dstPalette.SetFlags(srcPalette.flags);

            auto colors = dstPalette.InitColors(srcPalette.colors.Size());
            for (uint32_t colorIndex = 0; colorIndex < srcPalette.colors.Size(); ++colorIndex)
            {
                const CompiledFontPaletteColor& srcColor = srcPalette.colors[colorIndex];
                FontFacePaletteColor::Builder dstColor = colors[colorIndex];
                dstColor.SetRed(srcColor.red);
                dstColor.SetGreen(srcColor.green);
                dstColor.SetBlue(srcColor.blue);
                dstColor.SetAlpha(srcColor.alpha);
            }
        }

        auto colorGlyphs = paint.InitColorGlyphs(renderData.paint.colorGlyphs.Size());
        for (uint32_t glyphIndex = 0; glyphIndex < renderData.paint.colorGlyphs.Size(); ++glyphIndex)
        {
            const CompiledColorGlyphEntry& srcColorGlyph = renderData.paint.colorGlyphs[glyphIndex];
            FontFaceColorGlyph::Builder dstColorGlyph = colorGlyphs[glyphIndex];
            dstColorGlyph.SetFirstLayer(srcColorGlyph.firstLayer);
            dstColorGlyph.SetLayerCount(srcColorGlyph.layerCount);
        }

        auto layers = paint.InitLayers(renderData.paint.layers.Size());
        for (uint32_t layerIndex = 0; layerIndex < renderData.paint.layers.Size(); ++layerIndex)
        {
            const CompiledColorGlyphLayerEntry& srcLayer = renderData.paint.layers[layerIndex];
            FontFaceColorGlyphLayer::Builder dstLayer = layers[layerIndex];
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

        schema::Builder blobBuilder;
        CompiledFontFaceBlob::Builder blob = blobBuilder.AddStruct<CompiledFontFaceBlob>();
        RuntimeBlobHeader::Builder header = blob.InitHeader();
        header.SetFormatVersion(RuntimeBlobFormatVersion);
        header.SetKind(RuntimeBlobKind::FontFace);
        header.SetFlags(0);
        blob.SetShapingData(blobBuilder.AddBlob(Span<const schema::Word>(shapingBuilder).AsBytes()));
        blob.SetCurveData(blobBuilder.AddBlob(Span<const PackedCurveTexel>(renderData.curveTexels.Data(), renderData.curveTexels.Size()).AsBytes()));
        blob.SetBandData(blobBuilder.AddBlob(Span<const PackedBandTexel>(renderData.bandTexels.Data(), renderData.bandTexels.Size()).AsBytes()));
        blob.SetPaintData(blobBuilder.AddBlob(Span<const schema::Word>(paintBuilder).AsBytes()));
        blob.SetMetadataData(blobBuilder.AddBlob(Span<const schema::Word>(metadataBuilder).AsBytes()));
        blob.SetRenderData(blobBuilder.AddBlob(Span<const schema::Word>(renderBuilder).AsBytes()));
        blobBuilder.SetRoot(blob);

        r = ctx.db.AddResource(ctx.asset.GetUuid(), RuntimeBlobId, Span<const schema::Word>(blobBuilder).AsBytes());
        if (!r)
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Failed to write compiled scribe font runtime blob."),
                HE_KV(asset_uuid, assets::AssetUuid(ctx.asset.GetUuid())),
                HE_KV(asset_name, ctx.asset.GetName()),
                HE_KV(resource_id, RuntimeBlobId),
                HE_KV(result, r));
            return false;
        }

        return true;
    }
}
