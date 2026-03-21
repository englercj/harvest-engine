// Copyright Chad Engler

#include "he/scribe/editor/font_face_compiler.h"

#include "font_compile_geometry.h"
#include "font_import_utils.h"

#include "he/scribe/runtime_blob.h"

#include "he/assets/types.h"
#include "he/assets/types_fmt.h"
#include "he/core/log.h"
#include "he/core/result_fmt.h"

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

        schema::Builder blobBuilder;
        CompiledFontFaceBlob::Builder blob = blobBuilder.AddStruct<CompiledFontFaceBlob>();
        RuntimeBlobHeader::Builder header = blob.InitHeader();
        header.SetFormatVersion(RuntimeBlobFormatVersion);
        header.SetKind(RuntimeBlobKind::FontFace);
        header.SetFlags(0);
        blob.SetShapingData(blobBuilder.AddBlob(Span<const schema::Word>(shapingBuilder).AsBytes()));
        blob.SetCurveData(blobBuilder.AddBlob(Span<const PackedCurveTexel>(renderData.curveTexels.Data(), renderData.curveTexels.Size()).AsBytes()));
        blob.SetBandData(blobBuilder.AddBlob(Span<const PackedBandTexel>(renderData.bandTexels.Data(), renderData.bandTexels.Size()).AsBytes()));
        blob.SetPaintData(blobBuilder.AddBlob({}));
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
