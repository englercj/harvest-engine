// Copyright Chad Engler

#include "he/scribe/editor/font_importer.h"

#include "font_import_utils.h"

#include "he/assets/types.h"
#include "he/assets/types_fmt.h"
#include "he/core/fmt.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/result_fmt.h"

namespace he::scribe::editor
{
    namespace
    {
        String BuildAssetName(const char* file, const FontFaceInfo& info)
        {
            String name = info.familyName;

            if (!info.styleName.IsEmpty())
            {
                if (!name.IsEmpty())
                {
                    name += " ";
                }

                name += info.styleName;
            }

            if (name.IsEmpty())
            {
                name = GetPathWithoutExtension(GetBaseName(file));
            }

            if (info.faceCount > 1)
            {
                name = Format("{} #{}", name, info.faceIndex);
            }

            return name;
        }
    }

    bool FontImporter::CanImport(const char* file)
    {
        const StringView ext = GetExtension(file);
        return ext.EqualToI(".ttf")
            || ext.EqualToI(".otf")
            || ext.EqualToI(".ttc")
            || ext.EqualToI(".otc");
    }

    assets::ImportError FontImporter::Import(const assets::ImportContext& ctx, assets::ImportResult& result)
    {
        constexpr assets::ResourceId ImportSourceId{ ScribeFontFace::ImportSourceResourceName };
        constexpr StringView AssetTypeName = ScribeFontFace::AssetTypeName;
        const assets::AssetUuid sourceAssetUuid(ctx.assetFile.GetUuid());

        Vector<uint8_t> sourceBytes;
        if (!ReadFontSourceBytes(sourceBytes, ctx.file))
        {
            return assets::ImportError::Failure;
        }

        Vector<FontFaceInfo> faces{};
        if (!InspectFontFaces(faces, sourceBytes) || faces.IsEmpty())
        {
            return assets::ImportError::Failure;
        }

        const uint32_t faceCount = faces.Size();

        schema::Builder sourceBuilder;
        ScribeFontFace::ImportSourceResource::Builder sourceResource = sourceBuilder.AddStruct<ScribeFontFace::ImportSourceResource>();
        sourceResource.SetSourceBytes(sourceBuilder.AddBlob(Span<const uint8_t>(sourceBytes)));
        sourceBuilder.SetRoot(sourceResource);

        Result r = ctx.db.AddResource(sourceAssetUuid, ImportSourceId, Span<const schema::Word>(sourceBuilder).AsBytes());
        if (!r)
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Failed to write shared scribe font import source resource."),
                HE_KV(asset_file_uuid, assets::AssetFileUuid(ctx.assetFile.GetUuid())),
                HE_KV(result, r),
                HE_KV(path, ctx.file));
            return assets::ImportError::Failure;
        }

        for (uint32_t faceIndex = 0; faceIndex < faceCount; ++faceIndex)
        {
            const FontFaceInfo& faceInfo = faces[faceIndex];

            assets::Asset::Builder asset;

            for (const assets::Asset::Reader existing : ctx.assetFile.GetAssets())
            {
                if (existing.GetType() != AssetTypeName)
                {
                    continue;
                }

                const ScribeFontFace::Reader existingData = existing.GetData().TryGetStruct<ScribeFontFace>();
                if (!existingData.IsValid() || existingData.GetFaceIndex() != faceIndex)
                {
                    continue;
                }

                asset = result.UpdateAsset(existing.GetUuid());
                break;
            }

            if (!asset.IsValid())
            {
                asset = result.CreateAsset(AssetTypeName, BuildAssetName(ctx.file, faceInfo));
            }
            else
            {
                asset.InitName(BuildAssetName(ctx.file, faceInfo));
            }

            schema::Builder* assetBuilder = asset.GetData().GetBuilder();
            ScribeFontFace::Builder assetData = assetBuilder->AddStruct<ScribeFontFace>();
            FillFontFaceAssetData(assetData, faceInfo);
            asset.GetData().Set(assetData.AsReader());
        }

        return assets::ImportError::Success;
    }
}
