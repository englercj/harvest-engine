// Copyright Chad Engler

#include "he/scribe/editor/image_compiler.h"

#include "he/scribe/runtime_blob.h"

#include "he/assets/types.h"
#include "he/assets/types_fmt.h"
#include "he/core/log.h"
#include "he/core/result_fmt.h"

namespace he::scribe::editor
{
    bool ImageCompiler::Compile(const assets::CompileContext& ctx, [[maybe_unused]] assets::CompileResult& result)
    {
        constexpr assets::ResourceId RuntimeBlobId{ ScribeImage::RuntimeBlobResourceName };

        const ScribeImage::Reader asset = ctx.asset.GetData().TryGetStruct<ScribeImage>();
        if (!asset.IsValid())
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Scribe image asset data is invalid or missing."),
                HE_KV(asset_uuid, assets::AssetUuid(ctx.asset.GetUuid())),
                HE_KV(asset_name, ctx.asset.GetName()));
            return false;
        }

        schema::Builder metadataBuilder;
        VectorImageRuntimeMetadata::Builder metadata = metadataBuilder.AddStruct<VectorImageRuntimeMetadata>();
        metadata.SetSourceViewBoxWidth(0.0f);
        metadata.SetSourceViewBoxHeight(0.0f);
        metadataBuilder.SetRoot(metadata);

        schema::Builder blobBuilder;
        CompiledVectorImageBlob::Builder blob = blobBuilder.AddStruct<CompiledVectorImageBlob>();
        RuntimeBlobHeader::Builder header = blob.InitHeader();
        header.SetFormatVersion(RuntimeBlobFormatVersion);
        header.SetKind(RuntimeBlobKind::VectorImage);
        header.SetFlags(0);
        blob.SetCurveData(blobBuilder.AddBlob({}));
        blob.SetBandData(blobBuilder.AddBlob({}));
        blob.SetPaintData(blobBuilder.AddBlob({}));
        blob.SetMetadataData(blobBuilder.AddBlob(Span<const schema::Word>(metadataBuilder).AsBytes()));
        blobBuilder.SetRoot(blob);

        Result r = ctx.db.AddResource(ctx.asset.GetUuid(), RuntimeBlobId, Span<const schema::Word>(blobBuilder).AsBytes());
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
