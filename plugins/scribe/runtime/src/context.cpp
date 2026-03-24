// Copyright Chad Engler

#include "he/scribe/context.h"

#include "he/scribe/compiled_font.h"
#include "he/scribe/compiled_vector_image.h"

#include "glyph_atlas.h"
#include "glyph_atlas_utils.h"
#include "resource_copy_utils.h"

#include "he/core/allocator.h"
#include "he/core/assert.h"
#include "he/core/hash.h"
#include "he/core/log.h"
#include "he/core/memory_ops.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/utils.h"
#include "he/rhi/cmd_list.h"
#include "he/rhi/cmd_queue.h"
#include "he/rhi/device.h"

#include <hb-ot.h>
#include <hb.h>

namespace he::scribe
{
    namespace
    {
        struct RegisteredFontFace
        {
            Vector<schema::Word> storage{};
            FontFaceResourceReader reader{};
            uint64_t hash{ 0 };
            hb_blob_t* blob{ nullptr };
            hb_face_t* face{ nullptr };
            hb_font_t* font{ nullptr };
            GlyphAtlas* atlas{ nullptr };
            Vector<GlyphResource> glyphResources{};
            Vector<uint8_t> glyphResourceStates{};
        };

        struct RegisteredVectorImage
        {
            Vector<schema::Word> storage{};
            VectorImageResourceReader reader{};
            uint64_t hash{ 0 };
            GlyphAtlas* atlas{ nullptr };
            Vector<GlyphResource> shapeResources{};
            Vector<uint8_t> shapeResourceStates{};
        };
    }

    struct ScribeContext::Impl
    {
        rhi::Device* device{ nullptr };
        Vector<RegisteredFontFace> fonts{};
        Vector<RegisteredVectorImage> images{};
    };

    ScribeContext::~ScribeContext() noexcept
    {
        Terminate();
    }

    bool ScribeContext::Initialize(rhi::Device& device)
    {
        if (!m_impl)
        {
            m_impl = Allocator::GetDefault().New<Impl>();
        }

        if (m_impl->device && (m_impl->device != &device))
        {
            return false;
        }

        m_impl->device = &device;
        return true;
    }

    void ScribeContext::Terminate()
    {
        if (!m_impl)
        {
            return;
        }

        if (m_impl->device)
        {
            for (RegisteredFontFace& font : m_impl->fonts)
            {
                if (font.atlas)
                {
                    DestroyAtlas(*m_impl->device, font.atlas);
                }
            }

            for (RegisteredVectorImage& image : m_impl->images)
            {
                if (image.atlas)
                {
                    DestroyAtlas(*m_impl->device, image.atlas);
                }
            }
        }

        for (RegisteredFontFace& font : m_impl->fonts)
        {
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
        }

        Allocator::GetDefault().Delete(m_impl);
        m_impl = nullptr;
    }

    bool ScribeContext::IsInitialized() const
    {
        return m_impl != nullptr;
    }

    rhi::Device* ScribeContext::GetDevice() const
    {
        return m_impl ? m_impl->device : nullptr;
    }

    FontFaceHandle ScribeContext::RegisterFontFace(const FontFaceResourceReader& fontFace)
    {
        if (!fontFace.IsValid())
        {
            return {};
        }

        if (!m_impl)
        {
            m_impl = Allocator::GetDefault().New<Impl>();
        }

        const Span<const uint8_t> sourceBytes = GetResourceBytes(fontFace);
        const uint64_t hash = HashResourceBytes(sourceBytes);
        for (uint32_t index = 0; index < m_impl->fonts.Size(); ++index)
        {
            const RegisteredFontFace& existing = m_impl->fonts[index];
            if ((existing.hash == hash)
                && (existing.storage.Size() == (sourceBytes.Size() / sizeof(schema::Word)))
                && MemEqual(existing.storage.Data(), sourceBytes.Data(), sourceBytes.Size()))
            {
                return { index + 1u };
            }
        }

        Vector<schema::Word> storage{};
        RegisteredFontFace& registered = m_impl->fonts.EmplaceBack();
        registered.reader = CopyOwnedResource<FontFaceResource>(storage, fontFace);
        registered.storage = Move(storage);
        registered.hash = hash;
        const uint32_t glyphCount = registered.reader.GetMetadata().GetGlyphCount();
        registered.glyphResources.Resize(glyphCount, DefaultInit);
        registered.glyphResourceStates.Resize(glyphCount, DefaultInit);
        return { m_impl->fonts.Size() };
    }

    VectorImageHandle ScribeContext::RegisterVectorImage(const VectorImageResourceReader& image)
    {
        if (!image.IsValid())
        {
            return {};
        }

        if (!m_impl)
        {
            m_impl = Allocator::GetDefault().New<Impl>();
        }

        const Span<const uint8_t> sourceBytes = GetResourceBytes(image);
        const uint64_t hash = HashResourceBytes(sourceBytes);
        for (uint32_t index = 0; index < m_impl->images.Size(); ++index)
        {
            const RegisteredVectorImage& existing = m_impl->images[index];
            if ((existing.hash == hash)
                && (existing.storage.Size() == (sourceBytes.Size() / sizeof(schema::Word)))
                && MemEqual(existing.storage.Data(), sourceBytes.Data(), sourceBytes.Size()))
            {
                return { index + 1u };
            }
        }

        Vector<schema::Word> storage{};
        RegisteredVectorImage& registered = m_impl->images.EmplaceBack();
        registered.reader = CopyOwnedResource<VectorImageResource>(storage, image);
        registered.storage = Move(storage);
        registered.hash = hash;
        const uint32_t shapeCount = registered.reader.GetRender().GetShapes().Size();
        registered.shapeResources.Resize(shapeCount, DefaultInit);
        registered.shapeResourceStates.Resize(shapeCount, DefaultInit);
        return { m_impl->images.Size() };
    }

    const FontFaceResourceReader* ScribeContext::GetFontFace(FontFaceHandle handle) const
    {
        if (!m_impl || !handle.IsValid() || (handle.value > m_impl->fonts.Size()))
        {
            return nullptr;
        }

        return &m_impl->fonts[handle.value - 1u].reader;
    }

    const VectorImageResourceReader* ScribeContext::GetVectorImage(VectorImageHandle handle) const
    {
        if (!m_impl || !handle.IsValid() || (handle.value > m_impl->images.Size()))
        {
            return nullptr;
        }

        return &m_impl->images[handle.value - 1u].reader;
    }

    hb_font_t* ScribeContext::GetHbFont(FontFaceHandle handle)
    {
        if (!m_impl || !handle.IsValid() || (handle.value > m_impl->fonts.Size()))
        {
            return nullptr;
        }

        RegisteredFontFace& font = m_impl->fonts[handle.value - 1u];
        if (font.font)
        {
            return font.font;
        }

        const schema::Blob::Reader sourceBytes = font.reader.GetShaping().GetSourceBytes();
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

        hb_face_t* face = hb_face_create(blob, font.reader.GetShaping().GetFaceIndex());
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
        const uint32_t unitsPerEm = Max(font.reader.GetMetadata().GetUnitsPerEm(), 1u);
        hb_font_set_scale(hbFont, static_cast<int32_t>(unitsPerEm), static_cast<int32_t>(unitsPerEm));
        font.blob = blob;
        font.face = face;
        font.font = hbFont;
        return font.font;
    }

    bool ScribeContext::HasSourceBytes(FontFaceHandle handle) const
    {
        const FontFaceResourceReader* fontFace = GetFontFace(handle);
        return fontFace && !fontFace->GetShaping().GetSourceBytes().IsEmpty();
    }

    bool ScribeContext::EnsureGlyphResource(FontFaceHandle handle, uint32_t glyphIndex, const GlyphResource*& out)
    {
        out = nullptr;
        if (!m_impl || !m_impl->device || !handle.IsValid() || (handle.value > m_impl->fonts.Size()))
        {
            return false;
        }

        RegisteredFontFace& font = m_impl->fonts[handle.value - 1u];
        if (glyphIndex >= font.glyphResourceStates.Size())
        {
            return false;
        }

        if (font.glyphResourceStates[glyphIndex] == 2)
        {
            return false;
        }

        if (font.glyphResourceStates[glyphIndex] == 1)
        {
            out = &font.glyphResources[glyphIndex];
            return true;
        }

        if (!font.atlas)
        {
            font.atlas = Allocator::GetDefault().New<GlyphAtlas>();
            font.atlas->refCount = 1;

            const FontFaceRenderData::Reader render = font.reader.GetRender();
            const schema::Blob::Reader curveData = font.reader.GetCurveData();
            const schema::Blob::Reader bandData = font.reader.GetBandData();
            TextureDataDesc curveTexture{};
            curveTexture.data = curveData.Data();
            curveTexture.size = { render.GetCurveTextureWidth(), render.GetCurveTextureHeight() };
            curveTexture.rowPitch = render.GetCurveTextureWidth() * sizeof(PackedCurveTexel);
            TextureDataDesc bandTexture{};
            bandTexture.data = bandData.Data();
            bandTexture.size = { render.GetBandTextureWidth(), render.GetBandTextureHeight() };
            bandTexture.rowPitch = render.GetBandTextureWidth() * sizeof(PackedBandTexel);

            if (!UploadTexturePair2D(
                    *m_impl->device,
                    font.atlas->curveTexture,
                    font.atlas->curveView,
                    curveTexture,
                    rhi::Format::RGBA16Float,
                    "Scribe Curve Texture",
                    "Scribe Curve Upload Buffer",
                    font.atlas->bandTexture,
                    font.atlas->bandView,
                    bandTexture,
                    rhi::Format::RG16Uint,
                    "Scribe Band Texture",
                    "Scribe Band Upload Buffer")
                || !CreateAtlasDescriptorTable(*m_impl->device, *font.atlas))
            {
                DestroyAtlas(*m_impl->device, font.atlas);
                return false;
            }
        }

        CompiledGlyphResourceData glyphData{};
        if (!BuildCompiledGlyphResourceData(glyphData, font.reader, glyphIndex))
        {
            font.glyphResourceStates[glyphIndex] = 2;
            return false;
        }

        GlyphResource& glyphResource = font.glyphResources[glyphIndex];
        MemCopy(glyphResource.vertices, glyphData.vertices, sizeof(glyphData.vertices));
        glyphResource.vertexCount = glyphData.createInfo.vertexCount;
        glyphResource.atlas = font.atlas;
        font.glyphResourceStates[glyphIndex] = 1;
        out = &glyphResource;
        return true;
    }

    bool ScribeContext::EnsureVectorShapeResource(VectorImageHandle handle, uint32_t shapeIndex, const GlyphResource*& out)
    {
        out = nullptr;
        if (!m_impl || !m_impl->device || !handle.IsValid() || (handle.value > m_impl->images.Size()))
        {
            return false;
        }

        RegisteredVectorImage& image = m_impl->images[handle.value - 1u];
        if (shapeIndex >= image.shapeResourceStates.Size())
        {
            return false;
        }

        if (image.shapeResourceStates[shapeIndex] == 2)
        {
            return false;
        }

        if (image.shapeResourceStates[shapeIndex] == 1)
        {
            out = &image.shapeResources[shapeIndex];
            return true;
        }

        if (!image.atlas)
        {
            image.atlas = Allocator::GetDefault().New<GlyphAtlas>();
            image.atlas->refCount = 1;

            const VectorImageRenderData::Reader render = image.reader.GetRender();
            const schema::Blob::Reader curveData = image.reader.GetCurveData();
            const schema::Blob::Reader bandData = image.reader.GetBandData();
            TextureDataDesc curveTexture{};
            curveTexture.data = curveData.Data();
            curveTexture.size = { render.GetCurveTextureWidth(), render.GetCurveTextureHeight() };
            curveTexture.rowPitch = render.GetCurveTextureWidth() * sizeof(PackedCurveTexel);
            TextureDataDesc bandTexture{};
            bandTexture.data = bandData.Data();
            bandTexture.size = { render.GetBandTextureWidth(), render.GetBandTextureHeight() };
            bandTexture.rowPitch = render.GetBandTextureWidth() * sizeof(PackedBandTexel);

            if (!UploadTexturePair2D(
                    *m_impl->device,
                    image.atlas->curveTexture,
                    image.atlas->curveView,
                    curveTexture,
                    rhi::Format::RGBA16Float,
                    "Scribe Curve Texture",
                    "Scribe Curve Upload Buffer",
                    image.atlas->bandTexture,
                    image.atlas->bandView,
                    bandTexture,
                    rhi::Format::RG16Uint,
                    "Scribe Band Texture",
                    "Scribe Band Upload Buffer")
                || !CreateAtlasDescriptorTable(*m_impl->device, *image.atlas))
            {
                DestroyAtlas(*m_impl->device, image.atlas);
                return false;
            }
        }

        CompiledVectorShapeResourceData shapeData{};
        if (!BuildCompiledVectorShapeResourceData(shapeData, image.reader, shapeIndex))
        {
            image.shapeResourceStates[shapeIndex] = 2;
            return false;
        }

        GlyphResource& shapeResource = image.shapeResources[shapeIndex];
        MemCopy(shapeResource.vertices, shapeData.vertices, sizeof(shapeData.vertices));
        shapeResource.vertexCount = shapeData.createInfo.vertexCount;
        shapeResource.atlas = image.atlas;
        image.shapeResourceStates[shapeIndex] = 1;
        out = &shapeResource;
        return true;
    }
}
