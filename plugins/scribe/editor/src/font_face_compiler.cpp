// Copyright Chad Engler

#include "he/scribe/editor/font_face_compiler.h"

#include "font_compile_geometry.h"
#include "font_import_utils.h"
#include "resource_build_utils.h"

#include "he/scribe/schema_types.h"

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
        constexpr assets::ResourceId RuntimeBlobId{ ScribeFontFace::RuntimeResourceName };

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

        const ScribeFontFace::ImportSourceResource::Reader importSource =
            schema::ReadRoot<ScribeFontFace::ImportSourceResource>(importSourceBytes.Data());
        const ScribeFontFace::ImportMetadataResource::Reader importMetadata =
            schema::ReadRoot<ScribeFontFace::ImportMetadataResource>(importMetadataBytes.Data());
        if (!importSource.IsValid() || !importMetadata.IsValid())
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Scribe font import resources are not valid schema payloads."),
                HE_KV(asset_uuid, assets::AssetUuid(ctx.asset.GetUuid())),
                HE_KV(asset_name, ctx.asset.GetName()));
            return false;
        }

        CompiledFontRenderData renderData{};
        Stopwatch timer;
        {
            const schema::Blob::Reader sourceBlob = importSource.GetSourceBytes();
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

        schema::Builder blobBuilder;
        FontFaceResource::Builder blob = blobBuilder.AddStruct<FontFaceResource>();

        FontFaceShapingData::Builder shaping = blob.InitShaping();
        shaping.SetFaceIndex(asset.GetFaceIndex());
        shaping.SetSourceFormat(importSource.GetSourceFormat());
        if (asset.GetPreserveSourceBytesForShaping())
        {
            const schema::Blob::Reader sourceBlob = importSource.GetSourceBytes();
            shaping.SetSourceBytes(blobBuilder.AddBlob({ sourceBlob.Data(), sourceBlob.Size() }));
        }
        else
        {
            shaping.SetSourceBytes(blobBuilder.AddBlob({}));
        }

        FontFaceImportMetadata::Builder metadata = blob.InitMetadata();
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

        FillFontFaceResourceRenderData(blob.InitRender(), renderData);
        FillFontFaceResourcePaintData(blob.InitPaint(), renderData.paint);
        blob.SetCurveData(blobBuilder.AddBlob(Span<const PackedCurveTexel>(renderData.curveTexels.Data(), renderData.curveTexels.Size()).AsBytes()));
        blob.SetBandData(blobBuilder.AddBlob(Span<const PackedBandTexel>(renderData.bandTexels.Data(), renderData.bandTexels.Size()).AsBytes()));
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
