// Copyright Chad Engler

#pragma once

#include "he/scribe/schema_types.h"

#include "he/core/span.h"
#include "he/core/types.h"

#include <hb.h>

namespace he::rhi
{
    class Device;
}

namespace he::scribe
{
    struct GlyphResource;

    struct FontFaceHandle
    {
        uint32_t value{ 0 };

        bool IsValid() const { return value != 0; }
        bool operator==(const FontFaceHandle& x) const = default;
    };

    struct VectorImageHandle
    {
        uint32_t value{ 0 };

        bool IsValid() const { return value != 0; }
        bool operator==(const VectorImageHandle& x) const = default;
    };

    class ScribeContext
    {
    public:
        ScribeContext() noexcept = default;
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

        bool IsInitialized() const;
        rhi::Device* GetDevice() const;

        FontFaceHandle RegisterFontFace(const FontFaceResourceReader& fontFace);
        VectorImageHandle RegisterVectorImage(const VectorImageResourceReader& image);

        const FontFaceResourceReader* GetFontFace(FontFaceHandle handle) const;
        const VectorImageResourceReader* GetVectorImage(VectorImageHandle handle) const;

        ::hb_font_t* GetHbFont(FontFaceHandle handle);
        bool HasSourceBytes(FontFaceHandle handle) const;
        bool EnsureGlyphResource(FontFaceHandle handle, uint32_t glyphIndex, const GlyphResource*& out);
        bool EnsureVectorShapeResource(VectorImageHandle handle, uint32_t shapeIndex, const GlyphResource*& out);

    private:
        struct Impl;
        Impl* m_impl{ nullptr };
    };
}
