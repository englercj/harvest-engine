// Copyright Chad Engler

#include "he/assets/importers/image_importer.h"

#include "he/assets/types.h"
#include "he/assets/types_fmt.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/result_fmt.h"
#include "he/core/string_fmt.h"

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

    ImportError ImageImporter::Import(const ImportContext& ctx, ImportResult& result)
    {
        constexpr ResourceId Texture2DPixelsId{ Texture2D::PixelsResourceName };

        // Load and validate the image is good to be used
        basisu::image img;
        if (!basisu::load_image(ctx.file, img))
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to decode image data."),
                HE_KV(path, ctx.file));
            return ImportError::Failure;
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
            return ImportError::Failure;
        }

        constexpr StringView assetTypeName = Texture2D::AssetTypeName;

        // Update or create a texture 2d asset for this image
        Asset::Builder asset;
        for (auto&& existing : ctx.assetFile.GetAssets())
        {
            if (existing.GetType() == assetTypeName)
            {
                asset = result.UpdateAsset(existing.GetUuid());
                break;
            }
        }

        if (!asset.IsValid())
        {
            const StringView assetName = GetPathWithoutExtension(GetBaseName(ctx.file));
            asset = result.CreateAsset(assetTypeName, assetName);
        }

        HE_ASSERT(asset.IsValid());

        // Create a resource to store the pixels of the image, which can be used later by the compiler
        const basisu::color_rgba_vec& pixels = img.get_pixels();
        const Span<const uint8_t> rgbaBytes = Span(pixels.begin(), pixels.end()).AsBytes();

        schema::Builder builder;
        Texture2D::PixelsResource::Builder pixelResource = builder.AddStruct<Texture2D::PixelsResource>();
        pixelResource.SetWidth(img.get_width());
        pixelResource.SetHeight(img.get_height());
        pixelResource.SetSource(builder.AddBlob(rgbaBytes));
        builder.SetRoot(pixelResource);

        const Span<const uint8_t> resourceBytes = Span<const schema::Word>(builder).AsBytes();
        Result r = ctx.db.AddResource(asset.GetUuid(), Texture2DPixelsId, resourceBytes);
        if (!r)
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to add pixel resource during image import."),
                HE_KV(asset_uuid, AssetUuid(asset.GetUuid())),
                HE_KV(asset_name, asset.GetName()),
                HE_KV(result, r),
                HE_KV(path, ctx.file));
            return ImportError::Failure;
        }

        return ImportError::Success;
    }
}
