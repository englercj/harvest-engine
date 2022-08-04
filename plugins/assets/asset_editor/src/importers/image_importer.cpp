// Copyright Chad Engler

#include "he/assets/importers/image_importer.h"

#include "he/assets/types.h"
#include "he/core/log.h"
#include "he/core/path.h"

#include "encoder/basisu_comp.h"
#include "encoder/basisu_enc.h"

namespace he::assets
{
    bool ImageImporter::CanImport(const char* file)
    {
        constexpr StringView PngExt = ".png";
        constexpr StringView TgaExt = ".tga";
        constexpr StringView JpgExt = ".jpg";
        constexpr StringView JpegExt = ".jpeg";
        constexpr StringView JfifExt = ".jfif";

        const StringView ext = GetExtension(file);

        return ext.EqualToI(PngExt)
            || ext.EqualToI(TgaExt)
            || ext.EqualToI(JpgExt)
            || ext.EqualToI(JpegExt)
            || ext.EqualToI(JfifExt);
    }

    bool ImageImporter::Import(const ImportContext& ctx, ImportResult& result)
    {
        basisu::image img;
        if (!basisu::load_image(ctx.file, img))
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to decode image data."),
                HE_KV(path, ctx.file));
            return false;
        }

        if (img.get_width() > basisu::BASISU_MAX_SUPPORTED_TEXTURE_DIMENSION
            || img.get_height() > basisu::BASISU_MAX_SUPPORTED_TEXTURE_DIMENSION)
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Image is too large to be imported as a texture."),
                HE_KV(image_width, img.get_height()),
                HE_KV(image_height, img.get_width()),
                HE_KV(max_width, basisu::BASISU_MAX_SUPPORTED_TEXTURE_DIMENSION),
                HE_KV(max_height, basisu::BASISU_MAX_SUPPORTED_TEXTURE_DIMENSION),
                HE_KV(path, ctx.file));
            return false;
        }

        const StringView assetTypeName = schema::Texture2D::AssetTypeName;
        const StringView assetName = GetPathWithoutExtension(GetBaseName(ctx.file));

        schema::Asset::Builder asset = result.AddAsset(assetTypeName, assetName);
        if (!asset.IsValid())
            return false;

        const Span<const uint8_t> rgbaBytes = Span<const basisu::color_rgba>(img.get_pixels()).AsBytes();

        he::schema::Builder builder;
        schema::Texture2D::PixelsResource::Builder pixelResource = builder.AddStruct<schema::Texture2D::PixelsResource>();
        pixelResource.SetWidth(img.get_width());
        pixelResource.SetHeight(img.get_height());

        he::schema::List<::he::schema::Blob>::Builder mips = pixelResource.InitMips(1);
        mips.Set(0, builder.AddBlob(rgbaBytes));

        builder.SetRoot(pixelResource);

        constexpr ResourceId Texture2DPixelsId{ schema::Texture2D::PixelsResourceName };

        const Span<const uint8_t> resourceBytes = Span<const he::schema::Word>(builder).AsBytes();
        if (!result.AddResource(asset.GetUuid(), Texture2DPixelsId, resourceBytes))
            return false;

        return true;
    }
}
