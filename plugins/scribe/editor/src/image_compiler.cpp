// Copyright Chad Engler

#include "he/scribe/editor/image_compiler.h"

#include "image_compile_geometry.h"

#include "he/scribe/runtime_blob.h"

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
        constexpr assets::ResourceId RuntimeBlobId{ ScribeImage::RuntimeBlobResourceName };

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

        const auto importSource = schema::ReadRoot<ScribeImage::ImportSourceResource>(importSourceBytes.Data());
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
            const auto sourceBlob = importSource.GetSourceBytes();
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

        schema::Builder metadataBuilder;
        VectorImageRuntimeMetadata::Builder metadata = metadataBuilder.AddStruct<VectorImageRuntimeMetadata>();
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
        VectorImageRenderData::Builder render = renderBuilder.AddStruct<VectorImageRenderData>();
        render.SetCurveTextureWidth(imageData.curveTextureWidth);
        render.SetCurveTextureHeight(imageData.curveTextureHeight);
        render.SetBandTextureWidth(imageData.bandTextureWidth);
        render.SetBandTextureHeight(imageData.bandTextureHeight);
        render.SetBandOverlapEpsilon(imageData.bandOverlapEpsilon);
        auto shapes = render.InitShapes(imageData.shapes.Size());
        for (uint32_t shapeIndex = 0; shapeIndex < imageData.shapes.Size(); ++shapeIndex)
        {
            const CompiledVectorShapeRenderEntry& srcShape = imageData.shapes[shapeIndex];
            VectorImageShapeRenderData::Builder dstShape = shapes[shapeIndex];
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
        VectorImagePaintData::Builder paint = paintBuilder.AddStruct<VectorImagePaintData>();
        auto layers = paint.InitLayers(imageData.layers.Size());
        for (uint32_t layerIndex = 0; layerIndex < imageData.layers.Size(); ++layerIndex)
        {
            const CompiledVectorImageLayerEntry& srcLayer = imageData.layers[layerIndex];
            VectorImageLayer::Builder dstLayer = layers[layerIndex];
            dstLayer.SetShapeIndex(srcLayer.shapeIndex);
            dstLayer.SetRed(srcLayer.red);
            dstLayer.SetGreen(srcLayer.green);
            dstLayer.SetBlue(srcLayer.blue);
            dstLayer.SetAlpha(srcLayer.alpha);
        }
        paintBuilder.SetRoot(paint);

        schema::Builder blobBuilder;
        CompiledVectorImageBlob::Builder blob = blobBuilder.AddStruct<CompiledVectorImageBlob>();
        RuntimeBlobHeader::Builder header = blob.InitHeader();
        header.SetFormatVersion(RuntimeBlobFormatVersion);
        header.SetKind(RuntimeBlobKind::VectorImage);
        header.SetFlags(0);
        blob.SetCurveData(blobBuilder.AddBlob(Span<const PackedCurveTexel>(imageData.curveTexels.Data(), imageData.curveTexels.Size()).AsBytes()));
        blob.SetBandData(blobBuilder.AddBlob(Span<const PackedBandTexel>(imageData.bandTexels.Data(), imageData.bandTexels.Size()).AsBytes()));
        blob.SetPaintData(blobBuilder.AddBlob(Span<const schema::Word>(paintBuilder).AsBytes()));
        blob.SetMetadataData(blobBuilder.AddBlob(Span<const schema::Word>(metadataBuilder).AsBytes()));
        blob.SetRenderData(blobBuilder.AddBlob(Span<const schema::Word>(renderBuilder).AsBytes()));
        blobBuilder.SetRoot(blob);

        r = ctx.db.AddResource(ctx.asset.GetUuid(), RuntimeBlobId, Span<const schema::Word>(blobBuilder).AsBytes());
        if (!r)
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Failed to write compiled scribe image runtime blob."),
                HE_KV(asset_uuid, assets::AssetUuid(ctx.asset.GetUuid())),
                HE_KV(asset_name, ctx.asset.GetName()),
                HE_KV(resource_id, RuntimeBlobId),
                HE_KV(result, r));
            return false;
        }

        return true;
    }
}
