// Copyright Chad Engler

#include "he/assets/compilers/texture2d_compiler.h"

#include "he/assets/types.h"
#include "he/assets/types_fmt.h"
#include "he/core/enum_ops.h"
#include "he/core/file.h"
#include "he/core/result_fmt.h"
#include "he/schema/layout.h"

#include "encoder/basisu_comp.h"

namespace he::assets
{
    static const char* GetBasisMipMapFilter(Texture2D::MipMapFilter x)
    {
        switch (x)
        {
            case Texture2D::MipMapFilter::Box: return "box";
            case Texture2D::MipMapFilter::Tent: return "tent";
            case Texture2D::MipMapFilter::Bell: return "bell";
            case Texture2D::MipMapFilter::BSpline: return "b-spline";
            case Texture2D::MipMapFilter::Mitchell: return "mitchell";
            case Texture2D::MipMapFilter::Blackman: return "blackman";
            case Texture2D::MipMapFilter::Lanczos3: return "lanczos3";
            case Texture2D::MipMapFilter::Lanczos4: return "lanczos4";
            case Texture2D::MipMapFilter::Lanczos6: return "lanczos6";
            case Texture2D::MipMapFilter::Lanczos12: return "lanczos12";
            case Texture2D::MipMapFilter::Kaiser: return "kaiser";
            case Texture2D::MipMapFilter::Gaussian: return "gaussian";
            case Texture2D::MipMapFilter::CatmullRom: return "catmullrom";
            case Texture2D::MipMapFilter::QuadraticInterp: return "quadratic_interp";
            case Texture2D::MipMapFilter::QuadraticApprox: return "quadratic_approx";
            case Texture2D::MipMapFilter::QuadraticMix: return "quadratic_mix";
        }

        HE_LOG_WARN(he_assets, HE_MSG("Encountered unknown texture mipmap filter, assuming 'kaiser'"), HE_KV(filter, x));
        return "kaiser";
    }

    static float GetBasisUastcRdoQuality(Texture2D::CompressionQuality x)
    {
        switch (x)
        {
            case Texture2D::CompressionQuality::VeryLow: return 5.0f;
            case Texture2D::CompressionQuality::Low: return 1.5f;
            case Texture2D::CompressionQuality::Normal: return 1.0f;
            case Texture2D::CompressionQuality::High: return 0.5f;
            case Texture2D::CompressionQuality::VeryHigh: return 0.0f;
        }

        HE_LOG_WARN(he_assets, HE_MSG("Encountered unknown texture compression quality, using default values."), HE_KV(quality, x));
        return 1.0f;
    }

    static int32_t GetBasisUastcFlags(Texture2D::CompressionQuality x)
    {
        switch (x)
        {
            case Texture2D::CompressionQuality::VeryLow: return basisu::cPackUASTCLevelFastest;
            case Texture2D::CompressionQuality::Low: return basisu::cPackUASTCLevelFaster;
            case Texture2D::CompressionQuality::Normal: return basisu::cPackUASTCLevelDefault;
            case Texture2D::CompressionQuality::High: return basisu::cPackUASTCLevelSlower;
            case Texture2D::CompressionQuality::VeryHigh: return basisu::cPackUASTCLevelVerySlow;
        }

        HE_LOG_WARN(he_assets, HE_MSG("Encountered unknown texture compression quality, using default values."), HE_KV(quality, x));
        return basisu::cPackUASTCLevelDefault;
    }

    static uint32_t GetBasisEtc1sQuality(Texture2D::CompressionQuality x)
    {
        static_assert(basisu::BASISU_QUALITY_MIN == 1 && basisu::BASISU_QUALITY_MAX == 255);

        switch (x)
        {
            case Texture2D::CompressionQuality::VeryLow: return 1;
            case Texture2D::CompressionQuality::Low: return 64;
            case Texture2D::CompressionQuality::Normal: return 128;
            case Texture2D::CompressionQuality::High: return 192;
            case Texture2D::CompressionQuality::VeryHigh: return 255;
        }

        HE_LOG_WARN(he_assets, HE_MSG("Encountered unknown texture compression quality, using default values."), HE_KV(quality, x));
        return 128;
    }

    bool Texture2DCompiler::Compile(const CompileContext& ctx, CompileResult& result)
    {
        HE_UNUSED(result);
        constexpr ResourceId Texture2DKtx2Id{ Texture2D::Ktx2ResourceName };
        constexpr ResourceId Texture2DPixelsId{ Texture2D::PixelsResourceName };

        // Get the pixel resource for this image and validate the data for correctness
        Vector<schema::Word> pixelsResourceData;
        Result r = ctx.db.GetResource(pixelsResourceData, ctx.asset.GetUuid(), Texture2DPixelsId);
        if (!r)
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Failed to get pixel resource during texture2d compile."),
                HE_KV(asset_uuid, AssetUuid(ctx.asset.GetUuid())),
                HE_KV(asset_name, ctx.asset.GetName()),
                HE_KV(asset_file_source, ctx.assetFile.GetSource()),
                HE_KV(resource_id, Texture2DPixelsId),
                HE_KV(result, r));
            return false;
        }

        const auto pixelsResource = schema::ReadRoot<Texture2D::PixelsResource>(pixelsResourceData.Data());
        if (!pixelsResource.IsValid())
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Pixel resource is not of the expected type."),
                HE_KV(asset_uuid, AssetUuid(ctx.asset.GetUuid())),
                HE_KV(asset_name, ctx.asset.GetName()),
                HE_KV(asset_file_source, ctx.assetFile.GetSource()),
                HE_KV(resource_id, Texture2DPixelsId),
                HE_KV(result, r));
            return false;
        }

        const Texture2D::Reader asset = ctx.asset.GetData().TryGetStruct<Texture2D>();
        if (!asset.IsValid())
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("Asset data is empty or not of the expected type for a Texture2D asset."),
                HE_KV(asset_uuid, AssetUuid(ctx.asset.GetUuid())),
                HE_KV(asset_name, ctx.asset.GetName()),
                HE_KV(asset_file_source, ctx.assetFile.GetSource()),
                HE_KV(result, r));
            return false;
        }

        basisu::basis_compressor_params params{};
        params.m_debug = false;

        if (!pixelsResource.HasSource())
        {
            HE_LOG_ERROR(he_assets,
                HE_MSG("PixelsResource has an empty source blob."),
                HE_KV(asset_uuid, AssetUuid(ctx.asset.GetUuid())),
                HE_KV(asset_name, ctx.asset.GetName()),
                HE_KV(asset_file_source, ctx.assetFile.GetSource()),
                HE_KV(resource_id, Texture2DPixelsId),
                HE_KV(result, r));
            return false;
        }

        if (pixelsResource.GetSource().Size() != pixelsResource.GetWidth() * pixelsResource.GetHeight())
            return false;

        // Source images
        params.m_source_images.resize(1);
        params.m_source_images[0].init(pixelsResource.GetSource().Data(), pixelsResource.GetWidth(), pixelsResource.GetHeight(), 4);

        if (pixelsResource.HasMips())
        {
            schema::List<schema::Blob>::Reader mips = pixelsResource.GetMips();
            params.m_source_mipmap_images.resize(1);
            params.m_source_mipmap_images[0].resize(mips.Size());

            uint32_t width = pixelsResource.GetWidth() / 2;
            uint32_t height = pixelsResource.GetHeight() / 2;

            for (uint16_t i = 0; i < mips.Size(); ++i)
            {
                params.m_source_mipmap_images[0][i].init(mips[i].Data(), width, height, 4);
                width /= 2;
                height /= 2;
            }
        }

        // Mipmap settings
        Texture2D::MipMapping::Reader mipMapping = asset.GetMipMapping();
        params.m_mip_gen = mipMapping.GetGenerate();
        params.m_mip_scale = mipMapping.GetScale();
        params.m_mip_filter = GetBasisMipMapFilter(mipMapping.GetFilter());
        params.m_mip_srgb = mipMapping.GetSRGB();
        //params.m_mip_premultiplied = mipMapping.GetPremultiply();
        params.m_mip_renormalize = mipMapping.GetRenormalize();
        params.m_mip_wrapping = !mipMapping.GetClamp();
        params.m_mip_fast = !mipMapping.GetSampleFirst();
        params.m_mip_smallest_dimension = mipMapping.GetSmallestDimension();

        // Image settings
        params.m_y_flip = asset.GetFlipY();
        params.m_perceptual = asset.GetSRGB();
        params.m_tex_type = basist::cBASISTexType2D;

        // Compression settings
        const Texture2D::Compression::Reader comp = asset.GetCompression();
        const Texture2D::CompressionQuality quality = comp.GetQuality();

        params.m_uastc = comp.GetFormat() == Texture2D::CompressionFormat::UASTC;
        if (params.m_uastc)
        {
            params.m_rdo_uastc = true;
            params.m_pack_uastc_flags = GetBasisUastcFlags(quality);
            params.m_rdo_uastc_quality_scalar = GetBasisUastcRdoQuality(quality);
        }
        else
        {
            params.m_quality_level = GetBasisEtc1sQuality(quality);
        }

        // Configure KTX2 file creation
        params.m_create_ktx2_file = true;
        params.m_ktx2_uastc_supercompression = basist::KTX2_SS_ZSTANDARD;
        params.m_ktx2_zstd_supercompression_level = comp.GetLevel();
        params.m_ktx2_srgb_transfer_func = params.m_perceptual;

        // Configure multithreading
        params.m_multithreading = false;
        params.m_pJob_pool = nullptr;

        // Configure output
        params.m_status_output = false;

        // Compress and format the texture into a KTX2 file
        basisu::basis_compressor compressor;
        if (!compressor.init(params))
        {
            HE_LOG_ERROR(he_assets, HE_MSG("Failed to initialize texture compressor."));
            return false;
        }

        basisu::basis_compressor::error_code err = compressor.process();
        if (err != basisu::basis_compressor::error_code::cECSuccess)
        {
            HE_LOG_ERROR(he_assets, HE_MSG("Failed to compress texture."), HE_KV(result, err));
            return false;
        }

        const basisu::uint8_vec& data = compressor.get_output_ktx2_file();
        ctx.db.AddResource(ctx.asset.GetUuid(), Texture2DKtx2Id, { data.begin(), data.end() });
        return true;
    }
}

namespace he
{
    const char* AsString(basisu::basis_compressor::error_code x)
    {
        switch (x)
        {
            case basisu::basis_compressor::error_code::cECSuccess: return "Success";
            case basisu::basis_compressor::error_code::cECFailedInitializing: return "FailedInitializing";
            case basisu::basis_compressor::error_code::cECFailedReadingSourceImages: return "FailedReadingSourceImages";
            case basisu::basis_compressor::error_code::cECFailedValidating: return "FailedValidating";
            case basisu::basis_compressor::error_code::cECFailedEncodeUASTC: return "FailedEncodeUASTC";
            case basisu::basis_compressor::error_code::cECFailedFrontEnd: return "FailedFrontEnd";
            case basisu::basis_compressor::error_code::cECFailedFontendExtract: return "FailedFontendExtract";
            case basisu::basis_compressor::error_code::cECFailedBackend: return "FailedBackend";
            case basisu::basis_compressor::error_code::cECFailedCreateBasisFile: return "FailedCreateBasisFile";
            case basisu::basis_compressor::error_code::cECFailedWritingOutput: return "FailedWritingOutput";
            case basisu::basis_compressor::error_code::cECFailedUASTCRDOPostProcess: return "FailedUASTCRDOPostProcess";
            case basisu::basis_compressor::error_code::cECFailedCreateKTX2File: return "FailedCreateKTX2File";
        }

        return "<unknown>";
    }
}
