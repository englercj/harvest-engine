// Copyright Chad Engler

#include "he/assets/compilers/texture2d_compiler.h"

#include "he/assets/types.h"
#include "he/core/enum_ops.h"
#include "he/core/file.h"
#include "he/schema/layout.h"

#include "encoder/basisu_comp.h"

namespace he::assets
{
    static const char* GetBasisMipMapFilter(schema::Texture2D::MipMapFilter x)
    {
        switch (x)
        {
            case schema::Texture2D::MipMapFilter::Box: return "box";
            case schema::Texture2D::MipMapFilter::Tent: return "tent";
            case schema::Texture2D::MipMapFilter::Bell: return "bell";
            case schema::Texture2D::MipMapFilter::BSpline: return "b-spline";
            case schema::Texture2D::MipMapFilter::Mitchell: return "mitchell";
            case schema::Texture2D::MipMapFilter::Blackman: return "blackman";
            case schema::Texture2D::MipMapFilter::Lanczos3: return "lanczos3";
            case schema::Texture2D::MipMapFilter::Lanczos4: return "lanczos4";
            case schema::Texture2D::MipMapFilter::Lanczos6: return "lanczos6";
            case schema::Texture2D::MipMapFilter::Lanczos12: return "lanczos12";
            case schema::Texture2D::MipMapFilter::Kaiser: return "kaiser";
            case schema::Texture2D::MipMapFilter::Gaussian: return "gaussian";
            case schema::Texture2D::MipMapFilter::CatmullRom: return "catmullrom";
            case schema::Texture2D::MipMapFilter::QuadraticInterp: return "quadratic_interp";
            case schema::Texture2D::MipMapFilter::QuadraticApprox: return "quadratic_approx";
            case schema::Texture2D::MipMapFilter::QuadraticMix: return "quadratic_mix";
        }

        HE_LOG_WARN(he_assets, HE_MSG("Encountered unknown texture mipmap filter, assuming 'kaiser'"), HE_KV(filter, x));
        return "kaiser";
    }

    static float GetBasisUastcRdoQuality(schema::Texture2D::CompressionQuality x)
    {
        switch (x)
        {
            case schema::Texture2D::CompressionQuality::VeryLow: return 5.0f;
            case schema::Texture2D::CompressionQuality::Low: return 1.5f;
            case schema::Texture2D::CompressionQuality::Normal: return 1.0f;
            case schema::Texture2D::CompressionQuality::High: return 0.5f;
            case schema::Texture2D::CompressionQuality::VeryHigh: return 0.0f;
        }

        HE_LOG_WARN(he_assets, HE_MSG("Encountered unknown texture compression quality, using default values."), HE_KV(quality, x));
        return 1.0f;
    }

    static int32_t GetBasisUastcFlags(schema::Texture2D::CompressionQuality x)
    {
        switch (x)
        {
            case schema::Texture2D::CompressionQuality::VeryLow: return basisu::cPackUASTCLevelFastest;
            case schema::Texture2D::CompressionQuality::Low: return basisu::cPackUASTCLevelFaster;
            case schema::Texture2D::CompressionQuality::Normal: return basisu::cPackUASTCLevelDefault;
            case schema::Texture2D::CompressionQuality::High: return basisu::cPackUASTCLevelSlower;
            case schema::Texture2D::CompressionQuality::VeryHigh: return basisu::cPackUASTCLevelVerySlow;
        }

        HE_LOG_WARN(he_assets, HE_MSG("Encountered unknown texture compression quality, using default values."), HE_KV(quality, x));
        return basisu::cPackUASTCLevelDefault;
    }

    static uint32_t GetBasisEtc1sQuality(schema::Texture2D::CompressionQuality x)
    {
        static_assert(basisu::BASISU_QUALITY_MIN == 1 && basisu::BASISU_QUALITY_MAX == 255);

        switch (x)
        {
            case schema::Texture2D::CompressionQuality::VeryLow: return 1;
            case schema::Texture2D::CompressionQuality::Low: return 64;
            case schema::Texture2D::CompressionQuality::Normal: return 128;
            case schema::Texture2D::CompressionQuality::High: return 192;
            case schema::Texture2D::CompressionQuality::VeryHigh: return 255;
        }

        HE_LOG_WARN(he_assets, HE_MSG("Encountered unknown texture compression quality, using default values."), HE_KV(quality, x));
        return 128;
    }

    bool Texture2DCompiler::Compile(const CompileContext& ctx, CompileResult& result)
    {
        constexpr ResourceId Texture2DPixelsId{ schema::Texture2D::PixelsResourceName };

        Vector<he::schema::Word> pixelsResourceData;
        if (!ctx.GetResource(ctx.asset.GetUuid(), Texture2DPixelsId, pixelsResourceData))
            return false;

        const auto pixelsResource = he::schema::ReadRoot<schema::Texture2D::PixelsResource>(pixelsResourceData.Data());
        if (!pixelsResource.IsValid())
            return false;

        if (!ctx.asset.HasData())
            return false;

        const schema::Texture2D::Reader asset = ctx.asset.GetData().TryGetStruct<schema::Texture2D>();
        if (!asset.IsValid())
            return false;

        basisu::basis_compressor_params params{};
        params.m_debug = false;

        if (!pixelsResource.HasSource())
            return false;

        if (pixelsResource.GetSource().Size() != pixelsResource.GetWidth() * pixelsResource.GetHeight())
            return false;

        // Source images
        params.m_source_images.resize(1);
        params.m_source_images[0].init(pixelsResource.GetSource().Data(), pixelsResource.GetWidth(), pixelsResource.GetHeight(), 4);

        if (pixelsResource.HasMips())
        {
            he::schema::List<he::schema::Blob>::Reader mips = pixelsResource.GetMips();
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
        schema::Texture2D::MipMapping::Reader mipMapping = asset.GetMipMapping();
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
        const schema::Texture2D::Compression::Reader comp = asset.GetCompression();
        const schema::Texture2D::CompressionQuality quality = comp.GetQuality();

        params.m_uastc = comp.GetFormat() == schema::Texture2D::CompressionFormat::UASTC;
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

        // configure KTX2 file creation
        params.m_create_ktx2_file = true;
        params.m_ktx2_uastc_supercompression = basist::KTX2_SS_ZSTANDARD;
        params.m_ktx2_zstd_supercompression_level = comp.GetLevel();
        params.m_ktx2_srgb_transfer_func = params.m_perceptual;

        // TODO: use our job system for multithreading
        params.m_multithreading = false;
        params.m_pJob_pool = nullptr;

        // No status output
        params.m_status_output = false;

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

        constexpr ResourceId Texture2DKtx2Id{ schema::Texture2D::Ktx2ResourceName };

        const basisu::uint8_vec& data = compressor.get_output_ktx2_file();
        result.AddResource(ctx.asset.GetUuid(), Texture2DKtx2Id, { data.data(), data.size() });
        return true;
    }
}

namespace he
{
    template <>
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
