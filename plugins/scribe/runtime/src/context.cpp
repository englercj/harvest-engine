// Copyright Chad Engler

#include "he/scribe/context.h"

#include "he/scribe/compiled_font.h"
#include "he/scribe/layout_engine.h"
#include "he/scribe/renderer.h"

#include "glyph_atlas.h"
#include "glyph_atlas_utils.h"

#include "he/assets/types.h"
#include "he/core/allocator.h"
#include "he/core/assert.h"
#include "he/core/hash.h"
#include "he/core/hash_table.h"
#include "he/core/log.h"
#include "he/core/memory_ops.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/string.h"
#include "he/core/utils.h"
#include "he/rhi/cmd_list.h"
#include "he/rhi/cmd_queue.h"
#include "he/rhi/device.h"

#include "hb-ot.h"
#include "hb.h"

namespace he
{
    const char* EnumTraits<scribe::StrokeJoinStyle>::ToString(scribe::StrokeJoinStyle x) noexcept
    {
        switch (x)
        {
            case scribe::StrokeJoinStyle::Miter: return "Miter";
            case scribe::StrokeJoinStyle::Bevel: return "Bevel";
            case scribe::StrokeJoinStyle::Round: return "Round";
        }

        return "<unknown>";
    }

    const char* EnumTraits<scribe::StrokeCapStyle>::ToString(scribe::StrokeCapStyle x) noexcept
    {
        switch (x)
        {
            case scribe::StrokeCapStyle::Butt: return "Butt";
            case scribe::StrokeCapStyle::Square: return "Square";
            case scribe::StrokeCapStyle::Round: return "Round";
        }

        return "<unknown>";
    }
}

namespace he::scribe
{
    namespace
    {
        struct StrokeResourceKey
        {
            uint32_t index{ 0 };
            StrokeStyle style{};

            [[nodiscard]] uint64_t HashCode() const
            {
                return CombineHash64(GetHashCode(index), style.HashCode());
            }

            bool operator==(const StrokeResourceKey& x) const = default;
        };
    }

    struct ScribeContext::RegisteredFontFace
    {
        RegisteredFontFace() noexcept
        {
        }

        assets::AssetUuid key;
        Vector<schema::Word> data;
        ScribeFontFace::RuntimeResource::Reader resource;
        Vector<String> aliases;

        ::hb_blob_t* blob{ nullptr };
        ::hb_face_t* face{ nullptr };
        ::hb_font_t* font{ nullptr };
        GlyphAtlas* atlas{ nullptr };
        HashMap<uint32_t, GlyphResource> resources{};
        HashMap<StrokeResourceKey, GlyphResource> strokeResources{};
    };

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

            const auto fill = reader.GetFill();
            const schema::Blob::Reader curveData = fill.GetCurveData();
            const schema::Blob::Reader bandData = fill.GetBandData();
            TextureDataDesc curveTexture{};
            curveTexture.data = curveData.Data();
            curveTexture.size = { fill.GetCurveTextureWidth(), fill.GetCurveTextureHeight() };
            curveTexture.rowPitch = fill.GetCurveTextureWidth() * sizeof(PackedCurveTexel);
            TextureDataDesc bandTexture{};
            bandTexture.data = bandData.Data();
            bandTexture.size = { fill.GetBandTextureWidth(), fill.GetBandTextureHeight() };
            bandTexture.rowPitch = fill.GetBandTextureWidth() * sizeof(PackedBandTexel);

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

        bool CreateDedicatedResource(
            rhi::Device& device,
            GlyphResource& out,
            const GlyphResourceCreateInfo& createInfo)
        {
            out = {};
            if ((createInfo.vertices == nullptr)
                || (createInfo.vertexCount == 0)
                || (createInfo.curveTexture.data == nullptr)
                || (createInfo.bandTexture.data == nullptr))
            {
                return false;
            }

            MemCopy(out.vertices, createInfo.vertices, createInfo.vertexCount * sizeof(PackedGlyphVertex));
            out.vertexCount = createInfo.vertexCount;
            out.vertexColorIsWhite = createInfo.vertexColorIsWhite;
            out.atlas = Allocator::GetDefault().New<GlyphAtlas>();
            if (!UploadTexturePair2D(
                    device,
                    out.atlas->curveTexture,
                    out.atlas->curveView,
                    createInfo.curveTexture,
                    rhi::Format::RGBA16Float,
                    "Scribe Curve Texture",
                    "Scribe Curve Upload Buffer",
                    out.atlas->bandTexture,
                    out.atlas->bandView,
                    createInfo.bandTexture,
                    rhi::Format::RG16Uint,
                    "Scribe Band Texture",
                    "Scribe Band Upload Buffer")
                || !CreateAtlasDescriptorTable(device, *out.atlas))
            {
                DestroyAtlas(device, out.atlas);
                out = {};
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

            if (!EnsureRegisteredAtlas(device, registered.resource, registered.atlas))
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

        template <typename TRegistered, typename TBuildFn>
        bool EnsureCachedStrokeResource(
            TRegistered& registered,
            uint32_t index,
            const StrokeStyle& style,
            const GlyphResource*& out,
            TBuildFn&& buildFn)
        {
            out = nullptr;

            StrokeResourceKey key{};
            key.index = index;
            key.style = style;
            if (const GlyphResource* existing = registered.strokeResources.Find(key))
            {
                out = existing;
                return true;
            }

            GlyphResource resource{};
            if (!buildFn(resource))
            {
                return false;
            }

            GlyphResource& cached = registered.strokeResources.Emplace(Move(key), Move(resource)).entry.value;
            out = &cached;
            return true;
        }
    }

    ScribeContext::ScribeContext() noexcept
        : m_renderer(Allocator::GetDefault().New<Renderer>(*this))
        , m_layoutEngine(Allocator::GetDefault().New<LayoutEngine>(*this))
    {
    }

    ScribeContext::~ScribeContext() noexcept
    {
        Terminate();
        Allocator::GetDefault().Delete(m_layoutEngine);
        Allocator::GetDefault().Delete(m_renderer);
        m_layoutEngine = nullptr;
        m_renderer = nullptr;
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
        if (m_renderer)
        {
            m_renderer->Terminate();
        }

        if (m_device)
        {
            for (auto& hashAndFont : m_fonts)
            {
                RegisteredFontFace& font = *hashAndFont.value;

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

                DestroyAtlas(*m_device, font.atlas);
                for (HashMapEntry<StrokeResourceKey, GlyphResource>& indexAndResource : font.strokeResources)
                {
                    GlyphResource& resource = indexAndResource.value;
                    DestroyAtlas(*m_device, resource.atlas);
                }
            }

        }

        for (auto& hashAndFont : m_fonts)
        {
            Allocator::GetDefault().Delete(hashAndFont.value);
        }

        m_fonts.Clear();
        m_device = nullptr;
    }

    Renderer& ScribeContext::GetRenderer()
    {
        return *m_renderer;
    }

    const Renderer& ScribeContext::GetRenderer() const
    {
        return *m_renderer;
    }

    LayoutEngine& ScribeContext::GetLayoutEngine()
    {
        return *m_layoutEngine;
    }

    const LayoutEngine& ScribeContext::GetLayoutEngine() const
    {
        return *m_layoutEngine;
    }

    FontFaceHandle ScribeContext::RegisterFontFace(Vector<schema::Word>&& fontFaceWords, assets::AssetUuid resourceId)
    {
        return RegisterFontFace(Move(fontFaceWords), resourceId, {});
    }

    FontFaceHandle ScribeContext::RegisterFontFace(Vector<schema::Word>&& fontFaceWords, assets::AssetUuid resourceId, StringView alias)
    {
        if ((resourceId == assets::AssetUuid{}) || fontFaceWords.IsEmpty())
        {
            return {};
        }

        auto result = m_fonts.Emplace(resourceId);

        if (result.inserted)
        {
            RegisteredFontFace* registered = Allocator::GetDefault().New<RegisteredFontFace>();
            result.entry.value = registered;
            registered->data = Move(fontFaceWords);
            registered->resource = schema::ReadRoot<ScribeFontFace::RuntimeResource>(registered->data.Data());
            if (!registered->resource.IsValid())
            {
                Allocator::GetDefault().Delete(registered);
                result.entry.value = nullptr;
                m_fonts.Erase(resourceId);
                return {};
            }
            registered->key = resourceId;
            if (!alias.IsEmpty())
            {
                registered->aliases.PushBack(String(alias));
            }
        }
        else if (!alias.IsEmpty())
        {
            result.entry.value->aliases.PushBack(String(alias));
        }

        return { resourceId };
    }

    bool ScribeContext::AddFontFaceAlias(FontFaceHandle handle, StringView alias)
    {
        if (!handle.IsValid() || alias.IsEmpty())
        {
            return false;
        }

        RegisteredFontFace** fontFace = m_fonts.Find(handle.value);
        if (!fontFace || !(*fontFace))
        {
            return false;
        }

        (*fontFace)->aliases.PushBack(String(alias));
        return true;
    }

    ScribeFontFace::RuntimeResource::Reader ScribeContext::GetFontFace(FontFaceHandle handle) const
    {
        if (!handle.IsValid())
        {
            return {};
        }

        const RegisteredFontFace* const* fontFace = m_fonts.Find(handle.value);
        return fontFace ? (*fontFace)->resource : ScribeFontFace::RuntimeResource::Reader{};
    }

    bool ScribeContext::TryFindFontFaceByAlias(StringView alias, FontFaceHandle& out) const
    {
        out = {};
        if (alias.IsEmpty())
        {
            return false;
        }

        for (const auto& entry : m_fonts)
        {
            const RegisteredFontFace* fontFace = entry.value;
            if (fontFace == nullptr)
            {
                continue;
            }

            for (const String& existingAlias : fontFace->aliases)
            {
                if (StringView(existingAlias.Data(), existingAlias.Size()).EqualToI(alias))
                {
                    out = { fontFace->key };
                    return true;
                }
            }
        }

        return false;
    }

    hb_font_t* ScribeContext::GetHbFont(FontFaceHandle handle)
    {
        if (!handle.IsValid())
        {
            return nullptr;
        }

        RegisteredFontFace** fontFace = m_fonts.Find(handle.value);
        if (!fontFace)
        {
            return nullptr;
        }

        if ((*fontFace)->font)
        {
            return (*fontFace)->font;
        }

        const schema::Blob::Reader sourceBytes = (*fontFace)->resource.GetShaping().GetSourceBytes();
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

        hb_face_t* face = hb_face_create(blob, 0);
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
        const uint32_t unitsPerEm = Max((*fontFace)->resource.GetMetadata().GetUnitsPerEm(), 1u);
        hb_font_set_scale(hbFont, static_cast<int>(unitsPerEm), static_cast<int>(unitsPerEm));
        (*fontFace)->blob = blob;
        (*fontFace)->face = face;
        (*fontFace)->font = hbFont;
        return (*fontFace)->font;
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

        RegisteredFontFace** fontFace = m_fonts.Find(handle.value);
        if (!fontFace)
        {
            return false;
        }

        if (glyphIndex >= (*fontFace)->resource.GetMetadata().GetGlyphCount())
        {
            return false;
        }

        return EnsureCachedResource(*m_device, **fontFace, glyphIndex, out, [&](GlyphResource& resource) -> bool
        {
            CompiledGlyphResourceData glyphData{};
            if (!BuildCompiledGlyphResourceData(glyphData, (*fontFace)->resource, glyphIndex))
            {
                return false;
            }

            MemCopy(resource.vertices, glyphData.vertices, sizeof(glyphData.vertices));
            resource.vertexCount = glyphData.createInfo.vertexCount;
            resource.vertexColorIsWhite = glyphData.createInfo.vertexColorIsWhite;
            return true;
        });
    }

    bool ScribeContext::TryGetStrokedGlyphResource(
        FontFaceHandle handle,
        uint32_t glyphIndex,
        const StrokeStyle& style,
        const GlyphResource*& out)
    {
        out = nullptr;
        if (!m_device || !handle.IsValid() || !style.IsVisible())
        {
            return false;
        }

        RegisteredFontFace** fontFace = m_fonts.Find(handle.value);
        if (!fontFace)
        {
            return false;
        }

        if (glyphIndex >= (*fontFace)->resource.GetMetadata().GetGlyphCount())
        {
            return false;
        }

        return EnsureCachedStrokeResource(**fontFace, glyphIndex, style, out, [&](GlyphResource& resource) -> bool
        {
            CompiledStrokedGlyphResourceData glyphData{};
            if (!BuildCompiledStrokedGlyphResourceData(glyphData, (*fontFace)->resource, glyphIndex, style))
            {
                return false;
            }

            return CreateDedicatedResource(*m_device, resource, glyphData.createInfo);
        });
    }

}
