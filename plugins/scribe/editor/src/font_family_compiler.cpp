// Copyright Chad Engler

#include "he/scribe/editor/font_family_compiler.h"

#include "he/scribe/runtime_blob.h"

#include "he/assets/types.h"
#include "he/assets/types_fmt.h"
#include "he/core/log.h"
#include "he/core/result_fmt.h"

namespace he::scribe::editor
{
    bool FontFamilyCompiler::Compile(const assets::CompileContext& ctx, [[maybe_unused]] assets::CompileResult& result)
    {
        constexpr assets::ResourceId RuntimeBlobId{ ScribeFontFamily::RuntimeBlobResourceName };

        const ScribeFontFamily::Reader asset = ctx.asset.GetData().TryGetStruct<ScribeFontFamily>();
        if (!asset.IsValid())
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Scribe font family asset data is invalid or missing."),
                HE_KV(asset_uuid, assets::AssetUuid(ctx.asset.GetUuid())),
                HE_KV(asset_name, ctx.asset.GetName()));
            return false;
        }

        schema::Builder familyBuilder;
        FontFamilyRuntimeData::Builder family = familyBuilder.AddStruct<FontFamilyRuntimeData>();
        const auto faces = asset.GetFaces();
        auto faceAssets = family.InitFaceAssets(faces.Size());
        for (uint16_t i = 0; i < faces.Size(); ++i)
        {
            faceAssets.Set(i, faces[i]);
        }
        familyBuilder.SetRoot(family);

        schema::Builder blobBuilder;
        CompiledFontFamilyBlob::Builder blob = blobBuilder.AddStruct<CompiledFontFamilyBlob>();
        RuntimeBlobHeader::Builder header = blob.InitHeader();
        header.SetFormatVersion(RuntimeBlobFormatVersion);
        header.SetKind(RuntimeBlobKind::FontFamily);
        header.SetFlags(0);
        blob.SetFamilyData(blobBuilder.AddBlob(Span<const schema::Word>(familyBuilder).AsBytes()));
        blobBuilder.SetRoot(blob);

        Result r = ctx.db.AddResource(ctx.asset.GetUuid(), RuntimeBlobId, Span<const schema::Word>(blobBuilder).AsBytes());
        if (!r)
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Failed to write compiled scribe font family runtime blob."),
                HE_KV(asset_uuid, assets::AssetUuid(ctx.asset.GetUuid())),
                HE_KV(asset_name, ctx.asset.GetName()),
                HE_KV(resource_id, RuntimeBlobId),
                HE_KV(result, r));
            return false;
        }

        return true;
    }
}
