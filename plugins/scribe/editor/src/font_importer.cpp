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
        constexpr assets::ResourceId ImportMetadataId{ ScribeFontFace::ImportMetadataResourceName };
        constexpr StringView AssetTypeName = ScribeFontFace::AssetTypeName;

        Vector<uint8_t> sourceBytes;
        if (!ReadFontSourceBytes(sourceBytes, ctx.file))
        {
            return assets::ImportError::Failure;
        }

        const FontSourceFormat sourceFormat = DeduceFontSourceFormat(ctx.file);

        FontFaceInfo firstFace{};
        if (!InspectFontFace(sourceBytes, 0, sourceFormat, firstFace))
        {
            return assets::ImportError::Failure;
        }

        const uint32_t faceCount = firstFace.faceCount > 0 ? firstFace.faceCount : 1;
        const StringView sourceFileName = GetBaseName(ctx.file);
        schema::Uuid::Reader sourceOwnerAssetUuid{};

        for (uint32_t faceIndex = 0; faceIndex < faceCount; ++faceIndex)
        {
            FontFaceInfo faceInfo{};
            if (!InspectFontFace(sourceBytes, faceIndex, sourceFormat, faceInfo))
            {
                return assets::ImportError::Failure;
            }

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

            if (faceIndex == 0)
            {
                sourceOwnerAssetUuid = asset.GetUuid();
            }

            schema::Builder sourceBuilder;
            ScribeFontFace::ImportSourceResource::Builder sourceResource = sourceBuilder.AddStruct<ScribeFontFace::ImportSourceResource>();
            sourceResource.SetSourceFormat(sourceFormat);
            sourceResource.SetSourceOwnerAsset(sourceOwnerAssetUuid);
            if (faceIndex == 0)
            {
                sourceResource.SetSourceBytes(sourceBuilder.AddBlob(Span<const uint8_t>(sourceBytes)));
            }
            else
            {
                sourceResource.SetSourceBytes(sourceBuilder.AddBlob({}));
            }
            sourceResource.InitSourceFileName(sourceFileName);
            sourceResource.SetFaceCount(faceCount);
            sourceBuilder.SetRoot(sourceResource);

            Result r = ctx.db.AddResource(asset.GetUuid(), ImportSourceId, Span<const schema::Word>(sourceBuilder).AsBytes());
            if (!r)
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Failed to write scribe font import source resource."),
                    HE_KV(asset_uuid, assets::AssetUuid(asset.GetUuid())),
                    HE_KV(asset_name, asset.GetName()),
                    HE_KV(result, r),
                    HE_KV(path, ctx.file));
                return assets::ImportError::Failure;
            }

            schema::Builder metadataBuilder;
            ScribeFontFace::ImportMetadataResource::Builder metadataResource = metadataBuilder.AddStruct<ScribeFontFace::ImportMetadataResource>();
            FillFontFaceImportMetadata(metadataResource.InitMetadata(), faceInfo);
            metadataBuilder.SetRoot(metadataResource);

            r = ctx.db.AddResource(asset.GetUuid(), ImportMetadataId, Span<const schema::Word>(metadataBuilder).AsBytes());
            if (!r)
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Failed to write scribe font import metadata resource."),
                    HE_KV(asset_uuid, assets::AssetUuid(asset.GetUuid())),
                    HE_KV(asset_name, asset.GetName()),
                    HE_KV(result, r),
                    HE_KV(path, ctx.file));
                return assets::ImportError::Failure;
            }
        }

        return assets::ImportError::Success;
    }
}
