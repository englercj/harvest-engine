// Copyright Chad Engler

#pragma once

#include "he/scribe/schema_types.h"

#include "he/core/hash_table.h"
#include "he/core/span.h"
#include "he/core/types.h"

namespace he::rhi
{
    class Device;
}

struct hb_font_t;

namespace he::scribe
{
    enum class StrokeJoinStyle : uint8_t
    {
        Miter,
        Bevel,
        Round,
    };

    enum class StrokeCapStyle : uint8_t
    {
        Butt,
        Square,
        Round,
    };

    struct StrokeStyle
    {
        float width{ 0.0f };
        StrokeJoinStyle joinStyle{ StrokeJoinStyle::Miter };
        StrokeCapStyle capStyle{ StrokeCapStyle::Butt };
        float miterLimit{ 4.0f };

        [[nodiscard]] bool IsVisible() const
        {
            return (width > 0.0f) && (miterLimit > 0.0f);
        }

        [[nodiscard]] uint64_t HashCode() const
        {
            return CombineHash64(
                CombineHash64(
                    CombineHash64(GetHashCode(width), GetHashCode(joinStyle)),
                    GetHashCode(capStyle)),
                GetHashCode(miterLimit));
        }

        bool operator==(const StrokeStyle& x) const = default;
    };

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

        [[nodiscard]] ::hb_font_t* GetHbFont(FontFaceHandle handle);
        [[nodiscard]] bool HasSourceBytes(FontFaceHandle handle) const;
        bool TryGetGlyphResource(FontFaceHandle handle, uint32_t glyphIndex, const GlyphResource*& out);
        bool TryGetStrokedGlyphResource(
            FontFaceHandle handle,
            uint32_t glyphIndex,
            const StrokeStyle& style,
            const GlyphResource*& out);
        bool TryGetVectorShapeResource(VectorImageHandle handle, uint32_t shapeIndex, const GlyphResource*& out);
        bool TryGetStrokedVectorShapeResource(
            VectorImageHandle handle,
            uint32_t shapeIndex,
            const StrokeStyle& style,
            const GlyphResource*& out);

        [[nodiscard]] Renderer& GetRenderer();
        [[nodiscard]] const Renderer& GetRenderer() const;

        [[nodiscard]] LayoutEngine& GetLayoutEngine();
        [[nodiscard]] const LayoutEngine& GetLayoutEngine() const;

    private:
        struct RegisteredFontFace;
        struct RegisteredVectorImage;

        rhi::Device* m_device{ nullptr };
        HashMap<uint64_t, RegisteredFontFace*> m_fonts;
        HashMap<uint64_t, RegisteredVectorImage*> m_images;
        Renderer* m_renderer{ nullptr };
        LayoutEngine* m_layoutEngine{ nullptr };
    };
}
