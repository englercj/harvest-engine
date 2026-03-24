// Copyright Chad Engler

#include "he/scribe/editor/image_compiler.h"

#include "image_compile_geometry.h"
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
    bool ImageCompiler::Compile(const assets::CompileContext& ctx, [[maybe_unused]] assets::CompileResult& result)
    {
        constexpr assets::ResourceId ImportSourceId{ ScribeImage::ImportSourceResourceName };
        constexpr assets::ResourceId RuntimeResourceId{ ScribeImage::RuntimeResourceName };

        Vector<schema::Word> importSourceBytes;
        Result r = ctx.db.GetResource(importSourceBytes, ctx.asset.GetUuid(), ImportSourceId);
        if (!r)
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Failed to load scribe SVG import source resource."),
                HE_KV(asset_uuid, assets::AssetUuid(ctx.asset.GetUuid())),
                HE_KV(asset_name, ctx.asset.GetName()),
                HE_KV(resource_id, ImportSourceId),
                HE_KV(result, r));
            return false;
        }

        const ScribeImage::Reader asset = ctx.asset.GetData().TryGetStruct<ScribeImage>();
        if (!asset.IsValid())
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Scribe image asset data is invalid or missing."),
                HE_KV(asset_uuid, assets::AssetUuid(ctx.asset.GetUuid())),
                HE_KV(asset_name, ctx.asset.GetName()));
            return false;
        }

        const ScribeImage::ImportSourceResource::Reader importSource =
            schema::ReadRoot<ScribeImage::ImportSourceResource>(importSourceBytes.Data());
        if (!importSource.IsValid())
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Scribe SVG import source resource is not a valid schema payload."),
                HE_KV(asset_uuid, assets::AssetUuid(ctx.asset.GetUuid())),
                HE_KV(asset_name, ctx.asset.GetName()));
            return false;
        }

        CompiledVectorImageData imageData{};
        Stopwatch timer;
        {
            const schema::Blob::Reader sourceBlob = importSource.GetSourceBytes();
            if (!BuildCompiledVectorImageData(imageData, { sourceBlob.Data(), sourceBlob.Size() }, asset.GetFlatteningTolerance()))
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Failed to build compiled scribe vector image data."),
                    HE_KV(asset_uuid, assets::AssetUuid(ctx.asset.GetUuid())),
                    HE_KV(asset_name, ctx.asset.GetName()));
                return false;
            }
        }

        HE_LOG_INFO(he_scribe,
            HE_MSG("Compiled scribe vector image data."),
            HE_KV(asset_uuid, assets::AssetUuid(ctx.asset.GetUuid())),
            HE_KV(asset_name, ctx.asset.GetName()),
            HE_KV(time, timer.Elapsed()),
            HE_KV(curve_texels, imageData.curveTexels.Size()),
            HE_KV(band_headers, imageData.bandHeaderCount),
            HE_KV(emitted_band_payload_texels, imageData.emittedBandPayloadTexelCount),
            HE_KV(reused_bands, imageData.reusedBandCount),
            HE_KV(reused_band_payload_texels, imageData.reusedBandPayloadTexelCount));

        schema::Builder blobBuilder;
        VectorImageResource::Builder blob = blobBuilder.AddStruct<VectorImageResource>();

        FillVectorImageResourceMetadata(blob.GetMetadata(), imageData);
        FillVectorImageResourceRenderData(blob.GetRender(), imageData);
        FillVectorImageResourcePaintData(blob.GetPaint(), imageData);
        blob.SetCurveData(blobBuilder.AddBlob(Span<const PackedCurveTexel>(imageData.curveTexels.Data(), imageData.curveTexels.Size()).AsBytes()));
        blob.SetBandData(blobBuilder.AddBlob(Span<const PackedBandTexel>(imageData.bandTexels.Data(), imageData.bandTexels.Size()).AsBytes()));
        blobBuilder.SetRoot(blob);

        r = ctx.db.AddResource(ctx.asset.GetUuid(), RuntimeResourceId, Span<const schema::Word>(blobBuilder).AsBytes());
        if (!r)
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Failed to write compiled scribe image runtime blob."),
                HE_KV(asset_uuid, assets::AssetUuid(ctx.asset.GetUuid())),
                HE_KV(asset_name, ctx.asset.GetName()),
                HE_KV(resource_id, RuntimeResourceId),
                HE_KV(result, r));
            return false;
        }

        return true;
    }
}
