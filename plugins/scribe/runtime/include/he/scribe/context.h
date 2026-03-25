// Copyright Chad Engler

#pragma once

#include "he/scribe/schema_types.h"

#include "he/core/hash_table.h"
#include "he/core/span.h"
#include "he/core/types.h"
#include "he/scribe/layout_engine.h"
#include "he/scribe/renderer.h"

namespace he::rhi
{
    class Device;
}

namespace he::scribe
{
    struct GlyphResource;
    class Renderer;
    class LayoutEngine;

    struct FontFaceHandle
    {
        uint64_t value{ 0 };

        [[nodiscard]] bool IsValid() const { return value != 0; }
        [[nodiscard]] uint64_t HashCode() const { return value; }

        bool operator==(const FontFaceHandle& x) const = default;
        bool operator<(const FontFaceHandle& x) const = default;
    };

    struct VectorImageHandle
    {
        uint64_t value{ 0 };

        [[nodiscard]] bool IsValid() const { return value != 0; }
        [[nodiscard]] uint64_t HashCode() const { return value; }

        bool operator==(const VectorImageHandle& x) const = default;
        bool operator<(const VectorImageHandle& x) const = default;
    };

    class ScribeContext
    {
    public:
        ScribeContext() noexcept;
        ScribeContext(const ScribeContext&) = delete;
        ScribeContext(ScribeContext&&) = delete;
        ~ScribeContext() noexcept;

        ScribeContext& operator=(const ScribeContext&) = delete;
        ScribeContext& operator=(ScribeContext&&) = delete;

        // If GPU-backed resources have been prepared through this context, the associated
        // rhi::Device must either outlive the context or Terminate() must be called before the
        // device is destroyed.
        bool Initialize(rhi::Device& device);
        void Terminate();

        bool IsInitialized() const { return m_device != nullptr; }
        [[nodiscard]] rhi::Device* GetDevice() const { return m_device; }

        FontFaceHandle RegisterFontFace(const ScribeFontFace::RuntimeResource::Reader& fontFace);
        VectorImageHandle RegisterVectorImage(const ScribeImage::RuntimeResource::Reader& image);

        [[nodiscard]] ScribeFontFace::RuntimeResource::Reader GetFontFace(FontFaceHandle handle) const;
        [[nodiscard]] ScribeImage::RuntimeResource::Reader GetVectorImage(VectorImageHandle handle) const;

        [[nodiscard]] struct hb_font_t* GetHbFont(FontFaceHandle handle);
        [[nodiscard]] bool HasSourceBytes(FontFaceHandle handle) const;
        bool TryGetGlyphResource(FontFaceHandle handle, uint32_t glyphIndex, const GlyphResource*& out);
        bool TryGetVectorShapeResource(VectorImageHandle handle, uint32_t shapeIndex, const GlyphResource*& out);

        [[nodiscard]] Renderer& GetRenderer() { return m_renderer; }
        [[nodiscard]] const Renderer& GetRenderer() const { return m_renderer; }

        [[nodiscard]] LayoutEngine& GetLayoutEngine() { return m_layoutEngine; }
        [[nodiscard]] const LayoutEngine& GetLayoutEngine() const { return m_layoutEngine; }

    private:
        struct RegisteredFontFace
        {
            schema::Builder builder;
            ScribeFontFace::RuntimeResource::Builder resource;
            uint64_t hash{ 0 };

            struct hb_blob_t* blob{ nullptr };
            struct hb_face_t* face{ nullptr };
            struct hb_font_t* font{ nullptr };
            HashMap<uint32_t, GlyphResource> resources;
        };

        struct RegisteredVectorImage
        {
            schema::Builder builder;
            ScribeImage::RuntimeResource::Builder resource;
            uint64_t hash{ 0 };

            HashMap<uint32_t, GlyphResource> resources;
        };

        rhi::Device* m_device{ nullptr };
        HashMap<uint64_t, RegisteredFontFace> m_fonts;
        HashMap<uint64_t, RegisteredVectorImage> m_images;
        Renderer m_renderer;
        LayoutEngine m_layoutEngine;
    };
}
