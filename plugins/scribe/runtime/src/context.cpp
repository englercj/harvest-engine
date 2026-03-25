// Copyright Chad Engler

#include "he/scribe/context.h"

#include "he/scribe/compiled_font.h"
#include "he/scribe/compiled_vector_image.h"
#include "he/scribe/layout_engine.h"
#include "he/scribe/renderer.h"

#include "glyph_atlas.h"
#include "glyph_atlas_utils.h"
#include "resource_copy_utils.h"

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

#include <hb-ot.h>
#include <hb.h>

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
            atlas->refCount = 1;

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
        bool EnsureCachedResource(
            rhi::Device& device,
            TRegistered& registered,
            uint32_t index,
            const GlyphResource*& out,
            TBuildFn&& buildFn)
        {
            out = nullptr;

            if (const GlyphResource* existing = registered.resources.Find(index))
            {
                out = existing;
                return true;
            }

            if (registered.failedIndices.Contains(index))
            {
                return false;
            }

            if (!EnsureRegisteredAtlas(device, registered.reader, registered.atlas))
            {
                return false;
            }

            GlyphResource resource{};
            if (!buildFn(resource))
            {
                registered.failedIndices.Insert(index);
                return false;
            }

            resource.atlas = registered.atlas;
            GlyphResource& cached = registered.resources.Emplace(index, Move(resource)).entry.value;
            out = &cached;
            return true;
        }
    }

    ScribeContext::ScribeContext() noexcept
        : m_impl(Allocator::GetDefault().New<Impl>(*this))
    {
    }

    ScribeContext::~ScribeContext() noexcept
    {
        Terminate();
    }

    bool ScribeContext::Initialize(rhi::Device& device)
    {
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

        m_impl->renderer.Terminate();

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
        return (m_impl != nullptr) && (m_impl->device != nullptr);
    }

    rhi::Device* ScribeContext::GetDevice() const
    {
        return m_impl ? m_impl->device : nullptr;
    }

    Renderer& ScribeContext::GetRenderer()
    {
        return m_impl->renderer;
    }

    const Renderer& ScribeContext::GetRenderer() const
    {
        return m_impl->renderer;
    }

    LayoutEngine& ScribeContext::GetLayoutEngine()
    {
        return m_impl->layoutEngine;
    }

    const LayoutEngine& ScribeContext::GetLayoutEngine() const
    {
        return m_impl->layoutEngine;
    }

    FontFaceHandle ScribeContext::RegisterFontFace(const FontFaceResourceReader& fontFace)
    {
        if (!fontFace.IsValid())
        {
            return {};
        }

        const Span<const uint8_t> sourceBytes = GetResourceBytes(fontFace);
        const uint64_t hash = HashResourceBytes(sourceBytes);
        if (const Vector<uint32_t>* handles = m_impl->fontHandlesByHash.Find(hash))
        {
            for (uint32_t index : *handles)
            {
                const RegisteredFontFace& existing = m_impl->fonts[index];
                if ((existing.storage.Size() == (sourceBytes.Size() / sizeof(schema::Word)))
                    && MemEqual(existing.storage.Data(), sourceBytes.Data(), sourceBytes.Size()))
                {
                    return { index + 1u };
                }
            }
        }

        Vector<schema::Word> storage{};
        RegisteredFontFace& registered = m_impl->fonts.EmplaceBack();
        registered.reader = CopyOwnedResource<FontFaceResource>(storage, fontFace);
        registered.storage = Move(storage);
        registered.hash = hash;
        m_impl->fontHandlesByHash[hash].EmplaceBack(m_impl->fonts.Size() - 1u);
        return { m_impl->fonts.Size() };
    }

    VectorImageHandle ScribeContext::RegisterVectorImage(const VectorImageResourceReader& image)
    {
        if (!image.IsValid())
        {
            return {};
        }

        const Span<const uint8_t> sourceBytes = GetResourceBytes(image);
        const uint64_t hash = HashResourceBytes(sourceBytes);
        if (const Vector<uint32_t>* handles = m_impl->imageHandlesByHash.Find(hash))
        {
            for (uint32_t index : *handles)
            {
                const RegisteredVectorImage& existing = m_impl->images[index];
                if ((existing.storage.Size() == (sourceBytes.Size() / sizeof(schema::Word)))
                    && MemEqual(existing.storage.Data(), sourceBytes.Data(), sourceBytes.Size()))
                {
                    return { index + 1u };
                }
            }
        }

        Vector<schema::Word> storage{};
        RegisteredVectorImage& registered = m_impl->images.EmplaceBack();
        registered.reader = CopyOwnedResource<VectorImageResource>(storage, image);
        registered.storage = Move(storage);
        registered.hash = hash;
        m_impl->imageHandlesByHash[hash].EmplaceBack(m_impl->images.Size() - 1u);
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
        if (glyphIndex >= font.reader.GetMetadata().GetGlyphCount())
        {
            return false;
        }

        return EnsureCachedResource(
            *m_impl->device,
            font,
            glyphIndex,
            out,
            [&](GlyphResource& resource) -> bool
            {
                CompiledGlyphResourceData glyphData{};
                if (!BuildCompiledGlyphResourceData(glyphData, font.reader, glyphIndex))
                {
                    return false;
                }

                MemCopy(resource.vertices, glyphData.vertices, sizeof(glyphData.vertices));
                resource.vertexCount = glyphData.createInfo.vertexCount;
                return true;
            });
    }

    bool ScribeContext::EnsureVectorShapeResource(VectorImageHandle handle, uint32_t shapeIndex, const GlyphResource*& out)
    {
        out = nullptr;
        if (!m_impl || !m_impl->device || !handle.IsValid() || (handle.value > m_impl->images.Size()))
        {
            return false;
        }

        RegisteredVectorImage& image = m_impl->images[handle.value - 1u];
        if (shapeIndex >= image.reader.GetRender().GetShapes().Size())
        {
            return false;
        }

        return EnsureCachedResource(
            *m_impl->device,
            image,
            shapeIndex,
            out,
            [&](GlyphResource& resource) -> bool
            {
                CompiledVectorShapeResourceData shapeData{};
                if (!BuildCompiledVectorShapeResourceData(shapeData, image.reader, shapeIndex))
                {
                    return false;
                }

                MemCopy(resource.vertices, shapeData.vertices, sizeof(shapeData.vertices));
                resource.vertexCount = shapeData.createInfo.vertexCount;
                return true;
            });
    }
}
