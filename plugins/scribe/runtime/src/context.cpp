// Copyright Chad Engler

#include "he/scribe/context.h"

#include "he/scribe/compiled_font.h"
#include "he/scribe/compiled_vector_image.h"
#include "he/scribe/layout_engine.h"
#include "he/scribe/renderer.h"

#include "glyph_atlas.h"
#include "glyph_atlas_utils.h"

#include "he/core/allocator.h"
#include "he/core/assert.h"
#include "he/core/hash.h"
#include "he/core/hash_table.h"
#include "he/core/log.h"
#include "he/core/memory_ops.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/utils.h"
#include "he/rhi/cmd_list.h"
#include "he/rhi/cmd_queue.h"
#include "he/rhi/device.h"

#include "hb-ot.h"
#include "hb.h"

namespace he::scribe
{
    namespace
    {
        template <typename TReader>
        bool EnsureRegisteredAtlas(rhi::Device& device, const TReader& reader, GlyphAtlas*& atlas)
        {
            if (atlas)
            {
                return true;
            }

            atlas = Allocator::GetDefault().New<GlyphAtlas>();

            const auto render = reader.GetRender();
            const schema::Blob::Reader curveData = reader.GetCurveData();
            const schema::Blob::Reader bandData = reader.GetBandData();
            TextureDataDesc curveTexture{};
            curveTexture.data = curveData.Data();
            curveTexture.size = { render.GetCurveTextureWidth(), render.GetCurveTextureHeight() };
            curveTexture.rowPitch = render.GetCurveTextureWidth() * sizeof(PackedCurveTexel);
            TextureDataDesc bandTexture{};
            bandTexture.data = bandData.Data();
            bandTexture.size = { render.GetBandTextureWidth(), render.GetBandTextureHeight() };
            bandTexture.rowPitch = render.GetBandTextureWidth() * sizeof(PackedBandTexel);

            if (!UploadTexturePair2D(
                    device,
                    atlas->curveTexture,
                    atlas->curveView,
                    curveTexture,
                    rhi::Format::RGBA16Float,
                    "Scribe Curve Texture",
                    "Scribe Curve Upload Buffer",
                    atlas->bandTexture,
                    atlas->bandView,
                    bandTexture,
                    rhi::Format::RG16Uint,
                    "Scribe Band Texture",
                    "Scribe Band Upload Buffer")
                || !CreateAtlasDescriptorTable(device, *atlas))
            {
                DestroyAtlas(device, atlas);
                return false;
            }

            return true;
        }

        template <typename TRegistered, typename TBuildFn>
        bool EnsureCachedResource(rhi::Device& device, TRegistered& registered, uint32_t index, const GlyphResource*& out, TBuildFn&& buildFn)
        {
            out = nullptr;

            if (const GlyphResource* existing = registered.resources.Find(index))
            {
                out = existing;
                return true;
            }

            if (!EnsureRegisteredAtlas(device, registered.reader, registered.atlas))
            {
                return false;
            }

            GlyphResource resource{};
            if (!buildFn(resource))
            {
                return false;
            }

            resource.atlas = registered.atlas;
            GlyphResource& cached = registered.resources.Emplace(index, Move(resource)).entry.value;
            out = &cached;
            return true;
        }
    }

    ScribeContext::ScribeContext() noexcept
        : m_renderer(*this)
        , m_layoutEngine(*this)
    {}

    ScribeContext::~ScribeContext() noexcept
    {
        Terminate();
    }

    bool ScribeContext::Initialize(rhi::Device& device)
    {
        if (m_device && (m_device != &device))
        {
            return false;
        }

        m_device = &device;
        return true;
    }

    void ScribeContext::Terminate()
    {
        m_renderer.Terminate();

        if (m_device)
        {
            for (HashMapEntry<uint64_t, RegisteredFontFace>& hashAndFont : m_fonts)
            {
                RegisteredFontFace& font = hashAndFont.value;

                if (font.font)
                {
                    hb_font_destroy(font.font);
                }

                if (font.face)
                {
                    hb_face_destroy(font.face);
                }

                if (font.blob)
                {
                    hb_blob_destroy(font.blob);
                }

                for (HashMapEntry<uint32_t, GlyphResource>& indexAndResource : font.resources)
                {
                    GlyphResource& resource = indexAndResource.value;
                    if (resource.atlas)
                    {
                        DestroyAtlas(*m_device, resource.atlas);
                    }
                }
            }

            for (HashMapEntry<uint64_t, RegisteredVectorImage>& hashAndImage : m_images)
            {
                RegisteredVectorImage& image = hashAndImage.value;
                for (HashMapEntry<uint32_t, GlyphResource>& indexAndResource : image.resources)
                {
                    GlyphResource& resource = indexAndResource.value;
                    if (resource.atlas)
                    {
                        DestroyAtlas(*m_device, resource.atlas);
                    }
                }
            }
        }
    }

    FontFaceHandle ScribeContext::RegisterFontFace(const ScribeFontFace::RuntimeResource::Reader& fontFace)
    {
        if (!fontFace.IsValid())
        {
            return {};
        }

        Hash<WyHash> hasher;
        CalculateHash(hasher, fontFace);
        const uint64_t hash = hasher.Final();
        const auto result = m_fonts.Emplace(hash);

        if (result.inserted)
        {
            RegisteredFontFace& registered = result.entry.value;
            registered.resource = registered.builder.AddStruct<ScribeFontFace::RuntimeResource>();
            registered.resource.Copy(fontFace);
            registered.hash = hash;
        }

        return { hash };
    }

    VectorImageHandle ScribeContext::RegisterVectorImage(const ScribeImage::RuntimeResource::Reader& image)
    {
        if (!image.IsValid())
        {
            return {};
        }

        Hash<WyHash> hasher;
        CalculateHash(hasher, image);
        const uint64_t hash = hasher.Final();
        const auto result = m_images.Emplace(hash);

        if (result.inserted)
        {
            RegisteredVectorImage& registered = result.entry.value;
            registered.resource = registered.builder.AddStruct<ScribeImage::RuntimeResource>();
            registered.resource.Copy(image);
            registered.hash = hash;
        }

        return { hash };
    }

    ScribeFontFace::RuntimeResource::Reader ScribeContext::GetFontFace(FontFaceHandle handle) const
    {
        if (!handle.IsValid())
        {
            return {};
        }

        const RegisteredFontFace* fontFace = m_fonts.Find(handle.value);
        return fontFace ? fontFace->resource : ScribeFontFace::RuntimeResource::Reader{};
    }

    ScribeImage::RuntimeResource::Reader ScribeContext::GetVectorImage(VectorImageHandle handle) const
    {
        if (!handle.IsValid())
        {
            return {};
        }

        const RegisteredVectorImage* image = m_images.Find(handle.value);
        return image ? image->resource : ScribeImage::RuntimeResource::Reader{};
    }

    hb_font_t* ScribeContext::GetHbFont(FontFaceHandle handle)
    {
        if (!handle.IsValid())
        {
            return nullptr;
        }

        RegisteredFontFace* fontFace = m_fonts.Find(handle.value);
        if (!fontFace)
        {
            return nullptr;
        }

        if (fontFace->font)
        {
            return fontFace->font;
        }

        const schema::Blob::Reader sourceBytes = fontFace->resource.GetShaping().GetSourceBytes();
        if (sourceBytes.IsEmpty())
        {
            return nullptr;
        }

        hb_blob_t* blob = hb_blob_create(
            reinterpret_cast<const char*>(sourceBytes.Data()),
            static_cast<unsigned int>(sourceBytes.Size()),
            HB_MEMORY_MODE_READONLY,
            nullptr,
            nullptr);
        if (!blob)
        {
            return nullptr;
        }

        hb_face_t* face = hb_face_create(blob, fontFace->resource.GetShaping().GetFaceIndex());
        if (!face)
        {
            hb_blob_destroy(blob);
            return nullptr;
        }

        hb_font_t* hbFont = hb_font_create(face);
        if (!hbFont)
        {
            hb_face_destroy(face);
            hb_blob_destroy(blob);
            return nullptr;
        }

        hb_ot_font_set_funcs(hbFont);
        const uint32_t unitsPerEm = Max(fontFace->resource.GetMetadata().GetUnitsPerEm(), 1u);
        hb_font_set_scale(hbFont, static_cast<int>(unitsPerEm), static_cast<int>(unitsPerEm));
        fontFace->blob = blob;
        fontFace->face = face;
        fontFace->font = hbFont;
        return fontFace->font;
    }

    bool ScribeContext::HasSourceBytes(FontFaceHandle handle) const
    {
        ScribeFontFace::RuntimeResource::Reader fontFace = GetFontFace(handle);
        return fontFace.IsValid() && !fontFace.GetShaping().GetSourceBytes().IsEmpty();
    }

    bool ScribeContext::TryGetGlyphResource(FontFaceHandle handle, uint32_t glyphIndex, const GlyphResource*& out)
    {
        out = nullptr;
        if (!m_device || !handle.IsValid())
        {
            return false;
        }

        const RegisteredFontFace* fontFace = m_fonts.Find(handle.value);
        if (!fontFace)
        {
            return false;
        }

        if (glyphIndex >= fontFace->resource.GetMetadata().GetGlyphCount())
        {
            return false;
        }

        return EnsureCachedResource(*m_device, *fontFace, glyphIndex, out, [&](GlyphResource& resource) -> bool
        {
            CompiledGlyphResourceData glyphData{};
            if (!BuildCompiledGlyphResourceData(glyphData, fontFace->resource, glyphIndex))
            {
                return false;
            }

            MemCopy(resource.vertices, glyphData.vertices, sizeof(glyphData.vertices));
            resource.vertexCount = glyphData.createInfo.vertexCount;
            return true;
        });
    }

    bool ScribeContext::TryGetVectorShapeResource(VectorImageHandle handle, uint32_t shapeIndex, const GlyphResource*& out)
    {
        out = nullptr;
        if (!m_device || !handle.IsValid())
        {
            return false;
        }

        const RegisteredVectorImage* image = m_images.Find(handle.value);
        if (!image)
        {
            return false;
        }

        if (shapeIndex >= image->resource.GetRender().GetShapes().Size())
        {
            return false;
        }

        return EnsureCachedResource(*m_device, *image, shapeIndex, out, [&](GlyphResource& resource) -> bool
        {
            CompiledVectorShapeResourceData shapeData{};
            if (!BuildCompiledVectorShapeResourceData(shapeData, image->resource, shapeIndex))
            {
                return false;
            }

            MemCopy(resource.vertices, shapeData.vertices, sizeof(shapeData.vertices));
            resource.vertexCount = shapeData.createInfo.vertexCount;
            return true;
        });
    }
}
