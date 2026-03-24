// Copyright Chad Engler

#include "he/scribe/editor/svg_importer.h"

#include "he/scribe/schema_types.h"

#include "he/assets/types.h"
#include "he/assets/types_fmt.h"
#include "he/core/file.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/result_fmt.h"

namespace he::scribe::editor
{
    bool SvgImporter::CanImport(const char* file)
    {
        return GetExtension(file).EqualToI(".svg");
    }

    assets::ImportError SvgImporter::Import(const assets::ImportContext& ctx, assets::ImportResult& result)
    {
        constexpr assets::ResourceId ImportSourceId{ ScribeImage::ImportSourceResourceName };
        constexpr StringView AssetTypeName = ScribeImage::AssetTypeName;

        Vector<uint8_t> sourceBytes;
        Result readResult = File::ReadAll(sourceBytes, ctx.file);
        if (!readResult)
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Failed to read SVG source file for scribe import."),
                HE_KV(path, ctx.file),
                HE_KV(result, readResult));
            return assets::ImportError::Failure;
        }

        assets::Asset::Builder asset;
        for (const auto existing : ctx.assetFile.GetAssets())
        {
            if (existing.GetType() != AssetTypeName)
            {
                continue;
            }

            asset = result.UpdateAsset(existing.GetUuid());
            break;
        }

        const StringView assetName = GetPathWithoutExtension(GetBaseName(ctx.file));
        if (!asset.IsValid())
        {
            asset = result.CreateAsset(AssetTypeName, assetName);
        }
        else
        {
            asset.InitName(assetName);
        }

        schema::Builder sourceBuilder;
        ScribeImage::ImportSourceResource::Builder source = sourceBuilder.AddStruct<ScribeImage::ImportSourceResource>();
        source.SetSourceBytes(sourceBuilder.AddBlob(Span<const uint8_t>(sourceBytes)));
        source.InitSourceFileName(GetBaseName(ctx.file));
        sourceBuilder.SetRoot(source);

        Result r = ctx.db.AddResource(asset.GetUuid(), ImportSourceId, Span<const schema::Word>(sourceBuilder).AsBytes());
        if (!r)
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Failed to write scribe SVG import source resource."),
                HE_KV(asset_uuid, assets::AssetUuid(asset.GetUuid())),
                HE_KV(asset_name, asset.GetName()),
                HE_KV(result, r),
                HE_KV(path, ctx.file));
            return assets::ImportError::Failure;
        }

        return assets::ImportError::Success;
    }
}
