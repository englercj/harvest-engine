// Copyright Chad Engler

#include "he/scribe/editor/font_family_compiler.h"

#include "he/scribe/schema_types.h"

#include "he/assets/types.h"
#include "he/assets/types_fmt.h"
#include "he/core/log.h"
#include "he/core/result_fmt.h"

namespace he::scribe::editor
{
    bool FontFamilyCompiler::Compile(const assets::CompileContext& ctx, [[maybe_unused]] assets::CompileResult& result)
    {
        constexpr assets::ResourceId RuntimeResourceId{ ScribeFontFamily::RuntimeResourceName };

        const ScribeFontFamily::Reader asset = ctx.asset.GetData().TryGetStruct<ScribeFontFamily>();
        if (!asset.IsValid())
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Scribe font family asset data is invalid or missing."),
                HE_KV(asset_uuid, assets::AssetUuid(ctx.asset.GetUuid())),
                HE_KV(asset_name, ctx.asset.GetName()));
            return false;
        }

        schema::Builder blobBuilder;
        FontFamilyResource::Builder blob = blobBuilder.AddStruct<FontFamilyResource>();
        const schema::List<schema::Uuid>::Reader faces = asset.GetFaces();
        schema::List<schema::Uuid>::Builder faceAssets = blob.InitFaceAssets(faces.Size());
        for (uint16_t i = 0; i < faces.Size(); ++i)
        {
            faceAssets.Set(i, faces[i]);
        }
        blobBuilder.SetRoot(blob);

        Result r = ctx.db.AddResource(ctx.asset.GetUuid(), RuntimeResourceId, Span<const schema::Word>(blobBuilder).AsBytes());
        if (!r)
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Failed to write compiled scribe font family runtime resource."),
                HE_KV(asset_uuid, assets::AssetUuid(ctx.asset.GetUuid())),
                HE_KV(asset_name, ctx.asset.GetName()),
                HE_KV(resource_id, RuntimeResourceId),
                HE_KV(result, r));
            return false;
        }

        return true;
    }
}
