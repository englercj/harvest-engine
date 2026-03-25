// Copyright Chad Engler

#include "font_compile_geometry.h"
#include "font_import_utils.h"
#include "resource_build_utils.h"

#include "he/scribe/compiled_font.h"
#include "he/scribe/layout_engine.h"
#include "he/scribe/retained_text.h"
#include "he/scribe/renderer.h"
#include "he/scribe/schema_types.h"

#include "he/core/file.h"
#include "he/core/test.h"
#include "he/rhi/device.h"
#include "he/rhi/instance.h"

#include "he/core/log.h"

#include <cstdio>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

using namespace he;
using namespace he::scribe;
using namespace he::scribe::editor;

namespace
{
    constexpr uint32_t ColorGlyphIds[] =
    {
        2335u, // U+1F642
        2266u, // U+1F600
        884u,  // U+1F3A8
        724u,  // U+1F308
        4004u, // U+2728
    };

    bool ResolveRepoFontPath(String& out, const char* fileName)
    {
        static const char* Candidates[] =
        {
            "plugins/editor/src/fonts/",
            "../../../plugins/editor/src/fonts/",
        };

        for (const char* base : Candidates)
        {
            out = base;
            out += fileName;
            if (File::Exists(out.Data()))
            {
                return true;
            }
        }

        out.Clear();
        return false;
    }

    bool ReadFontFile(Vector<uint8_t>& out, const char* path)
    {
        out.Clear();
        const Result r = File::ReadAll(out, path);
        return !!r;
    }

    bool BuildLoadedCompiledFontFace(
        Vector<schema::Word>& storage,
        FontFaceResourceReader& out,
        const Vector<uint8_t>& fontBytes,
        [[maybe_unused]] const char* displayName)
    {
        FontFaceInfo faceInfo{};
        if (!InspectFontFace(fontBytes, 0, faceInfo))
        {
            return false;
        }

        CompiledFontRenderData renderData{};
        if (!BuildCompiledFontRenderData(renderData, fontBytes, 0))
        {
            return false;
        }

        schema::Builder rootBuilder;
        FontFaceResource::Builder root = rootBuilder.AddStruct<FontFaceResource>();

        FontFaceShapingData::Builder shaping = root.GetShaping();
        shaping.SetFaceIndex(faceInfo.faceIndex);
        shaping.SetSourceBytes(rootBuilder.AddBlob(fontBytes));

        FillFontFaceRuntimeMetadata(
            root.GetMetadata(),
            renderData.glyphs.Size(),
            faceInfo.unitsPerEm,
            faceInfo.ascender,
            faceInfo.descender,
            faceInfo.lineHeight,
            faceInfo.capHeight,
            faceInfo.hasColorGlyphs);

        FillFontFaceResourceRenderData(root.GetRender(), renderData);
        FillFontFaceResourcePaintData(root.GetPaint(), renderData.paint);
        root.SetCurveData(rootBuilder.AddBlob(Span<const PackedCurveTexel>(renderData.curveTexels.Data(), renderData.curveTexels.Size()).AsBytes()));
        root.SetBandData(rootBuilder.AddBlob(Span<const PackedBandTexel>(renderData.bandTexels.Data(), renderData.bandTexels.Size()).AsBytes()));
        rootBuilder.SetRoot(root);

        storage = Span<const schema::Word>(rootBuilder);
        out = schema::ReadRoot<FontFaceResource>(storage.Data());
        return out.IsValid();
    }

    bool BuildRetainedTextFromTemporaryFaceCopy(
        RetainedTextModel& out,
        ScribeContext& context,
        Span<const FontFaceResourceReader> fontFaces,
        const char* text,
        float fontSize,
        bool darkBackgroundPreferred = true)
    {
        Vector<FontFaceHandle> handles{};
        handles.Reserve(fontFaces.Size());
        for (const FontFaceResourceReader& fontFace : fontFaces)
        {
            const FontFaceHandle handle = context.RegisterFontFace(fontFace);
            if (!handle.IsValid())
            {
                return false;
            }

            handles.EmplaceBack(handle);
        }

        LayoutEngine engine(context);
        LayoutResult layout;
        LayoutOptions options{};
        options.fontSize = fontSize;
        options.maxWidth = 4096.0f;
        options.wrap = false;
        options.direction = TextDirection::Auto;
        if (!engine.LayoutText(
            layout,
            Span<const FontFaceHandle>(handles.Data(), handles.Size()),
            text,
            options))
        {
            return false;
        }

        RetainedTextBuildDesc desc{};
        desc.context = &context;
        desc.fontFaces = Span<const FontFaceHandle>(handles.Data(), handles.Size());
        desc.layout = &layout;
        desc.fontSize = fontSize;
        desc.darkBackgroundPreferred = darkBackgroundPreferred;
        return out.Build(desc);
    }

    bool RegisterFontFaces(
        ScribeContext& context,
        Span<const FontFaceResourceReader> fontFaces,
        Vector<FontFaceHandle>& out)
    {
        out.Clear();
        out.Reserve(fontFaces.Size());
        for (const FontFaceResourceReader& fontFace : fontFaces)
        {
            const FontFaceHandle handle = context.RegisterFontFace(fontFace);
            if (!handle.IsValid())
            {
                out.Clear();
                return false;
            }

            out.EmplaceBack(handle);
        }

        return true;
    }

    struct NullRendererHarness
    {
        rhi::Instance* instance{ nullptr };
        rhi::Device* device{ nullptr };
        ScribeContext context{};
        Renderer& renderer;

        NullRendererHarness() noexcept
            : renderer(context.GetRenderer())
        {
        }

        ~NullRendererHarness() noexcept
        {
            Terminate();
        }

        bool Initialize()
        {
            rhi::InstanceDesc instanceDesc{};
            instanceDesc.api = rhi::Api_Null;
            if (!rhi::Instance::Create(instanceDesc, instance) || !instance)
            {
                return false;
            }

            rhi::DeviceDesc deviceDesc{};
            if (!instance->CreateDevice(deviceDesc, device) || !device)
            {
                Terminate();
                return false;
            }

            if (!context.Initialize(*device)
                || !renderer.Initialize(rhi::Format::BGRA8Unorm_sRGB))
            {
                Terminate();
                return false;
            }

            return true;
        }

        void Terminate()
        {
            renderer.Terminate();
            context.Terminate();

            if (device)
            {
                instance->DestroyDevice(device);
                device = nullptr;
            }

            if (instance)
            {
                rhi::Instance::Destroy(instance);
                instance = nullptr;
            }
        }
    };

    class FreeTypeScope final
    {
    public:
        ~FreeTypeScope() noexcept
        {
            if (m_face)
            {
                FT_Done_Face(m_face);
            }

            if (m_library)
            {
                FT_Done_FreeType(m_library);
            }
        }

        bool Initialize(Span<const uint8_t> sourceBytes)
        {
            if (FT_Init_FreeType(&m_library) != 0)
            {
                return false;
            }

            if (FT_New_Memory_Face(
                    m_library,
                    reinterpret_cast<const FT_Byte*>(sourceBytes.Data()),
                    static_cast<FT_Long>(sourceBytes.Size()),
                    0,
                    &m_face) != 0)
            {
                return false;
            }

            return true;
        }

        FT_Face GetFace() const { return m_face; }

    private:
        FT_Library m_library{ nullptr };
        FT_Face m_face{ nullptr };
    };

    float UnpackFloat16(uint16_t value)
    {
        const uint32_t sign = static_cast<uint32_t>(value & 0x8000u) << 16u;
        const uint32_t exponent = (value >> 10u) & 0x1fu;
        const uint32_t mantissa = value & 0x03ffu;

        if (exponent == 0)
        {
            if (mantissa == 0)
            {
                return BitCast<float>(sign);
            }

            int32_t normalizedExponent = -14;
            uint32_t normalizedMantissa = mantissa;
            while ((normalizedMantissa & 0x0400u) == 0u)
            {
                normalizedMantissa <<= 1u;
                --normalizedExponent;
            }

            normalizedMantissa &= 0x03ffu;
            const uint32_t bits = sign
                | (static_cast<uint32_t>(normalizedExponent + 127) << 23u)
                | (normalizedMantissa << 13u);
            return BitCast<float>(bits);
        }

        if (exponent == 0x1fu)
        {
            const uint32_t bits = sign | 0x7f800000u | (mantissa << 13u);
            return BitCast<float>(bits);
        }

        const uint32_t bits = sign
            | ((exponent + 112u) << 23u)
            | (mantissa << 13u);
        return BitCast<float>(bits);
    }

    PackedBandTexel GetBandTexel(const CompiledFontRenderData& renderData, uint32_t x, uint32_t y)
    {
        return renderData.bandTexels[(y * renderData.bandTextureWidth) + x];
    }

    Vec2f LoadCurvePoint(const PackedCurveTexel& texel, uint32_t componentOffset)
    {
        if (componentOffset == 0)
        {
            return { UnpackFloat16(texel.x), UnpackFloat16(texel.y) };
        }

        return { UnpackFloat16(texel.z), UnpackFloat16(texel.w) };
    }

    uint32_t CalcRootCodeCpu(float y1, float y2, float y3)
    {
        const uint32_t i1 = BitCast<uint32_t>(y1) >> 31u;
        const uint32_t i2 = BitCast<uint32_t>(y2) >> 30u;
        const uint32_t i3 = BitCast<uint32_t>(y3) >> 29u;

        uint32_t shift = (i2 & 2u) | (i1 & ~2u);
        shift = (i3 & 4u) | (shift & ~4u);
        return (0x2E74u >> shift) & 0x0101u;
    }

    Vec2f SolveHorizPolyCpu(const Vec4f& p12, const Vec2f& p3)
    {
        const Vec2f a = { p12.x - (p12.z * 2.0f) + p3.x, p12.y - (p12.w * 2.0f) + p3.y };
        const Vec2f b = { p12.x - p12.z, p12.y - p12.w };
        const float ra = 1.0f / a.y;
        const float rb = 0.5f / b.y;

        const float d = Sqrt(Max((b.y * b.y) - (a.y * p12.y), 0.0f));
        float t1 = (b.y - d) * ra;
        float t2 = (b.y + d) * ra;
        if (Abs(a.y) < (1.0f / 65536.0f))
        {
            t1 = p12.y * rb;
            t2 = t1;
        }

        return {
            ((a.x * t1) - (b.x * 2.0f)) * t1 + p12.x,
            ((a.x * t2) - (b.x * 2.0f)) * t2 + p12.x
        };
    }

    Vec2f SolveVertPolyCpu(const Vec4f& p12, const Vec2f& p3)
    {
        const Vec2f a = { p12.x - (p12.z * 2.0f) + p3.x, p12.y - (p12.w * 2.0f) + p3.y };
        const Vec2f b = { p12.x - p12.z, p12.y - p12.w };
        const float ra = 1.0f / a.x;
        const float rb = 0.5f / b.x;

        const float d = Sqrt(Max((b.x * b.x) - (a.x * p12.x), 0.0f));
        float t1 = (b.x - d) * ra;
        float t2 = (b.x + d) * ra;
        if (Abs(a.x) < (1.0f / 65536.0f))
        {
            t1 = p12.x * rb;
            t2 = t1;
        }

        return {
            ((a.y * t1) - (b.y * 2.0f)) * t1 + p12.y,
            ((a.y * t2) - (b.y * 2.0f)) * t2 + p12.y
        };
    }

    Vec2u CalcBandLocCpu(Vec2u glyphLoc, uint32_t offset)
    {
        Vec2u bandLoc = { glyphLoc.x + offset, glyphLoc.y };
        bandLoc.y += bandLoc.x >> 12u;
        bandLoc.x &= (1u << 12u) - 1u;
        return bandLoc;
    }

    float CalcCoverageCpu(float xcov, float ycov, float xwgt, float ywgt, bool evenOdd)
    {
        float coverage = Max(
            Abs((xcov * xwgt + ycov * ywgt) / Max(xwgt + ywgt, 1.0f / 65536.0f)),
            Min(Abs(xcov), Abs(ycov)));

        if (!evenOdd)
        {
            return Clamp(coverage, 0.0f, 1.0f);
        }

        const float wrapped = (coverage * 0.5f) - Floor(coverage * 0.5f);
        return 1.0f - Abs(1.0f - wrapped * 2.0f);
    }

    float EvaluateGlyphCoverageCpu(
        const CompiledFontRenderData& renderData,
        const CompiledGlyphRenderEntry& glyph,
        float renderX,
        float renderY,
        const Vec2f& pixelsPerUnit)
    {
        const Vec2u glyphLoc = { glyph.glyphBandLocX, glyph.glyphBandLocY };
        const uint32_t bandMaxX = glyph.bandMaxX;
        const uint32_t bandMaxY = glyph.bandMaxY;
        const bool evenOdd = glyph.fillRule == FillRule::EvenOdd;

        const int32_t bandIndexX = Clamp(
            static_cast<int32_t>(renderX * glyph.bandScaleX + glyph.bandOffsetX),
            0,
            static_cast<int32_t>(bandMaxX));
        const int32_t bandIndexY = Clamp(
            static_cast<int32_t>(renderY * glyph.bandScaleY + glyph.bandOffsetY),
            0,
            static_cast<int32_t>(bandMaxY));

        float xcov = 0.0f;
        float xwgt = 0.0f;
        const PackedBandTexel hbandData = GetBandTexel(renderData, glyphLoc.x + static_cast<uint32_t>(bandIndexY), glyphLoc.y);
        const Vec2u hbandLoc = CalcBandLocCpu(glyphLoc, hbandData.y);
        for (uint32_t curveIndex = 0; curveIndex < hbandData.x; ++curveIndex)
        {
            const PackedBandTexel curveLoc = GetBandTexel(renderData, hbandLoc.x + curveIndex, hbandLoc.y);
            const PackedCurveTexel curveTexel0 = renderData.curveTexels[(curveLoc.y * renderData.curveTextureWidth) + curveLoc.x];
            const PackedCurveTexel curveTexel1 = renderData.curveTexels[(curveLoc.y * renderData.curveTextureWidth) + curveLoc.x + 1u];
            const Vec2f p1 = LoadCurvePoint(curveTexel0, 0);
            const Vec2f p2 = LoadCurvePoint(curveTexel0, 1);
            const Vec2f p3 = LoadCurvePoint(curveTexel1, 0);

            if (Max(Max(p1.x, p2.x), p3.x) * pixelsPerUnit.x < -0.5f)
            {
                break;
            }

            const Vec4f p12 = { p1.x - renderX, p1.y - renderY, p2.x - renderX, p2.y - renderY };
            const Vec2f rp3 = { p3.x - renderX, p3.y - renderY };
            const uint32_t code = CalcRootCodeCpu(p12.y, p12.w, rp3.y);
            if (code == 0u)
            {
                continue;
            }

            Vec2f roots = SolveHorizPolyCpu(p12, rp3);
            roots.x *= pixelsPerUnit.x;
            roots.y *= pixelsPerUnit.x;
            if ((code & 1u) != 0u)
            {
                xcov += Clamp(roots.x + 0.5f, 0.0f, 1.0f);
                xwgt = Max(xwgt, Clamp(1.0f - (Abs(roots.x) * 2.0f), 0.0f, 1.0f));
            }

            if (code > 1u)
            {
                xcov -= Clamp(roots.y + 0.5f, 0.0f, 1.0f);
                xwgt = Max(xwgt, Clamp(1.0f - (Abs(roots.y) * 2.0f), 0.0f, 1.0f));
            }
        }

        float ycov = 0.0f;
        float ywgt = 0.0f;
        const PackedBandTexel vbandData = GetBandTexel(
            renderData,
            glyphLoc.x + bandMaxY + 1u + static_cast<uint32_t>(bandIndexX),
            glyphLoc.y);
        const Vec2u vbandLoc = CalcBandLocCpu(glyphLoc, vbandData.y);
        for (uint32_t curveIndex = 0; curveIndex < vbandData.x; ++curveIndex)
        {
            const PackedBandTexel curveLoc = GetBandTexel(renderData, vbandLoc.x + curveIndex, vbandLoc.y);
            const PackedCurveTexel curveTexel0 = renderData.curveTexels[(curveLoc.y * renderData.curveTextureWidth) + curveLoc.x];
            const PackedCurveTexel curveTexel1 = renderData.curveTexels[(curveLoc.y * renderData.curveTextureWidth) + curveLoc.x + 1u];
            const Vec2f p1 = LoadCurvePoint(curveTexel0, 0);
            const Vec2f p2 = LoadCurvePoint(curveTexel0, 1);
            const Vec2f p3 = LoadCurvePoint(curveTexel1, 0);

            if (Max(Max(p1.y, p2.y), p3.y) * pixelsPerUnit.y < -0.5f)
            {
                break;
            }

            const Vec4f p12 = { p1.x - renderX, p1.y - renderY, p2.x - renderX, p2.y - renderY };
            const Vec2f rp3 = { p3.x - renderX, p3.y - renderY };
            const uint32_t code = CalcRootCodeCpu(p12.x, p12.z, rp3.x);
            if (code == 0u)
            {
                continue;
            }

            Vec2f roots = SolveVertPolyCpu(p12, rp3);
            roots.x *= pixelsPerUnit.y;
            roots.y *= pixelsPerUnit.y;
            if ((code & 1u) != 0u)
            {
                ycov -= Clamp(roots.x + 0.5f, 0.0f, 1.0f);
                ywgt = Max(ywgt, Clamp(1.0f - (Abs(roots.x) * 2.0f), 0.0f, 1.0f));
            }

            if (code > 1u)
            {
                ycov += Clamp(roots.y + 0.5f, 0.0f, 1.0f);
                ywgt = Max(ywgt, Clamp(1.0f - (Abs(roots.y) * 2.0f), 0.0f, 1.0f));
            }
        }

        return CalcCoverageCpu(xcov, ycov, xwgt, ywgt, evenOdd);
    }

    void DumpGlyphBandCoverageCpu(
        const CompiledFontRenderData& renderData,
        const CompiledGlyphRenderEntry& glyph,
        float renderX,
        float renderY,
        const Vec2f& pixelsPerUnit)
    {
        const Vec2u glyphLoc = { glyph.glyphBandLocX, glyph.glyphBandLocY };
        const uint32_t bandMaxX = glyph.bandMaxX;
        const uint32_t bandMaxY = glyph.bandMaxY;

        const int32_t bandIndexX = Clamp(
            static_cast<int32_t>(renderX * glyph.bandScaleX + glyph.bandOffsetX),
            0,
            static_cast<int32_t>(bandMaxX));
        const int32_t bandIndexY = Clamp(
            static_cast<int32_t>(renderY * glyph.bandScaleY + glyph.bandOffsetY),
            0,
            static_cast<int32_t>(bandMaxY));

        HE_LOG_INFO(he_scribe,
            HE_MSG("Dumping compiled T coverage sample."),
            HE_KV(render_x, renderX),
            HE_KV(render_y, renderY),
            HE_KV(band_index_x, bandIndexX),
            HE_KV(band_index_y, bandIndexY),
            HE_KV(bounds_min_x, glyph.boundsMinX),
            HE_KV(bounds_min_y, glyph.boundsMinY),
            HE_KV(bounds_max_x, glyph.boundsMaxX),
            HE_KV(bounds_max_y, glyph.boundsMaxY));

        const PackedBandTexel hbandData = GetBandTexel(renderData, glyphLoc.x + static_cast<uint32_t>(bandIndexY), glyphLoc.y);
        const Vec2u hbandLoc = CalcBandLocCpu(glyphLoc, hbandData.y);
        HE_LOG_INFO(he_scribe,
            HE_MSG("Horizontal band payload."),
            HE_KV(curve_count, hbandData.x),
            HE_KV(offset, hbandData.y));
        for (uint32_t curveIndex = 0; curveIndex < hbandData.x; ++curveIndex)
        {
            const PackedBandTexel curveLoc = GetBandTexel(renderData, hbandLoc.x + curveIndex, hbandLoc.y);
            const PackedCurveTexel curveTexel0 = renderData.curveTexels[(curveLoc.y * renderData.curveTextureWidth) + curveLoc.x];
            const PackedCurveTexel curveTexel1 = renderData.curveTexels[(curveLoc.y * renderData.curveTextureWidth) + curveLoc.x + 1u];
            const Vec2f p1 = LoadCurvePoint(curveTexel0, 0);
            const Vec2f p2 = LoadCurvePoint(curveTexel0, 1);
            const Vec2f p3 = LoadCurvePoint(curveTexel1, 0);
            const Vec4f p12 = { p1.x - renderX, p1.y - renderY, p2.x - renderX, p2.y - renderY };
            const Vec2f rp3 = { p3.x - renderX, p3.y - renderY };
            const uint32_t code = CalcRootCodeCpu(p12.y, p12.w, rp3.y);
            Vec2f roots{};
            if (code != 0u)
            {
                roots = SolveHorizPolyCpu(p12, rp3);
                roots.x *= pixelsPerUnit.x;
                roots.y *= pixelsPerUnit.x;
            }
            HE_LOG_INFO(he_scribe,
                HE_MSG("Horizontal curve."),
                HE_KV(curve_index, curveIndex),
                HE_KV(curve_loc_x, curveLoc.x),
                HE_KV(curve_loc_y, curveLoc.y),
                HE_KV(p1x, p1.x),
                HE_KV(p1y, p1.y),
                HE_KV(p2x, p2.x),
                HE_KV(p2y, p2.y),
                HE_KV(p3x, p3.x),
                HE_KV(p3y, p3.y),
                HE_KV(code, code),
                HE_KV(root0, roots.x),
                HE_KV(root1, roots.y));
        }

        const PackedBandTexel vbandData = GetBandTexel(
            renderData,
            glyphLoc.x + bandMaxY + 1u + static_cast<uint32_t>(bandIndexX),
            glyphLoc.y);
        const Vec2u vbandLoc = CalcBandLocCpu(glyphLoc, vbandData.y);
        HE_LOG_INFO(he_scribe,
            HE_MSG("Vertical band payload."),
            HE_KV(curve_count, vbandData.x),
            HE_KV(offset, vbandData.y));
        for (uint32_t curveIndex = 0; curveIndex < vbandData.x; ++curveIndex)
        {
            const PackedBandTexel curveLoc = GetBandTexel(renderData, vbandLoc.x + curveIndex, vbandLoc.y);
            const PackedCurveTexel curveTexel0 = renderData.curveTexels[(curveLoc.y * renderData.curveTextureWidth) + curveLoc.x];
            const PackedCurveTexel curveTexel1 = renderData.curveTexels[(curveLoc.y * renderData.curveTextureWidth) + curveLoc.x + 1u];
            const Vec2f p1 = LoadCurvePoint(curveTexel0, 0);
            const Vec2f p2 = LoadCurvePoint(curveTexel0, 1);
            const Vec2f p3 = LoadCurvePoint(curveTexel1, 0);
            const Vec4f p12 = { p1.x - renderX, p1.y - renderY, p2.x - renderX, p2.y - renderY };
            const Vec2f rp3 = { p3.x - renderX, p3.y - renderY };
            const uint32_t code = CalcRootCodeCpu(p12.x, p12.z, rp3.x);
            Vec2f roots{};
            if (code != 0u)
            {
                roots = SolveVertPolyCpu(p12, rp3);
                roots.x *= pixelsPerUnit.y;
                roots.y *= pixelsPerUnit.y;
            }
            HE_LOG_INFO(he_scribe,
                HE_MSG("Vertical curve."),
                HE_KV(curve_index, curveIndex),
                HE_KV(curve_loc_x, curveLoc.x),
                HE_KV(curve_loc_y, curveLoc.y),
                HE_KV(p1x, p1.x),
                HE_KV(p1y, p1.y),
                HE_KV(p2x, p2.x),
                HE_KV(p2y, p2.y),
                HE_KV(p3x, p3.x),
                HE_KV(p3y, p3.y),
                HE_KV(code, code),
                HE_KV(root0, roots.x),
                HE_KV(root1, roots.y));
        }
    }

    void DumpGlyphCoverageSliceCpu(
        const CompiledFontRenderData& renderData,
        const CompiledGlyphRenderEntry& glyph,
        float minX,
        float maxX,
        float renderY,
        const Vec2f& pixelsPerUnit,
        float stepX)
    {
        for (float renderX = minX; renderX <= maxX; renderX += stepX)
        {
            const float coverage = EvaluateGlyphCoverageCpu(renderData, glyph, renderX, renderY, pixelsPerUnit);
            HE_LOG_INFO(he_scribe,
                HE_MSG("Coverage slice sample."),
                HE_KV(render_x, renderX),
                HE_KV(render_y, renderY),
                HE_KV(coverage, coverage));
        }
    }

    void DumpNamedGlyphCoverageSliceCpu(
        const CompiledFontRenderData& renderData,
        const CompiledGlyphRenderEntry& glyph,
        float minX,
        float maxX,
        float renderY,
        const Vec2f& pixelsPerUnit,
        float stepX,
        const char* label)
    {
        for (float renderX = minX; renderX <= maxX; renderX += stepX)
        {
            const float coverage = EvaluateGlyphCoverageCpu(renderData, glyph, renderX, renderY, pixelsPerUnit);
            HE_LOG_INFO(he_scribe,
                HE_MSG("Named coverage slice sample."),
                HE_KV(label, label),
                HE_KV(render_x, renderX),
                HE_KV(render_y, renderY),
                HE_KV(coverage, coverage));
        }
    }
}

HE_TEST(scribe, color_font_pipeline, extracts_standalone_face_source_bytes)
{
    String fontPath{};
    HE_ASSERT(ResolveRepoFontPath(fontPath, "NotoSans-Regular.ttf"));

    Vector<uint8_t> fontBytes{};
    HE_ASSERT(ReadFontFile(fontBytes, fontPath.Data()));

    FontFaceInfo originalFace{};
    HE_ASSERT(InspectFontFace(fontBytes, 0, originalFace));

    Vector<uint8_t> extractedBytes{};
    HE_ASSERT(ExtractFontFaceSourceBytes(extractedBytes, fontBytes, 0));
    HE_EXPECT(!extractedBytes.IsEmpty());

    FontFaceInfo extractedFace{};
    HE_ASSERT(InspectFontFace(extractedBytes, 0, extractedFace));

    HE_EXPECT_EQ(extractedFace.faceIndex, 0u);
    HE_EXPECT_EQ_STR(extractedFace.familyName.Data(), originalFace.familyName.Data());
    HE_EXPECT_EQ_STR(extractedFace.styleName.Data(), originalFace.styleName.Data());
    HE_EXPECT_EQ(extractedFace.unitsPerEm, originalFace.unitsPerEm);
    HE_EXPECT_EQ(extractedFace.ascender, originalFace.ascender);
    HE_EXPECT_EQ(extractedFace.descender, originalFace.descender);
    HE_EXPECT_EQ(extractedFace.lineHeight, originalFace.lineHeight);
    HE_EXPECT_EQ(extractedFace.capHeight, originalFace.capHeight);
}

HE_TEST(scribe, color_font_pipeline, compiles_layered_color_glyphs)
{
    static constexpr const char* ColorFontPath = "C:/Windows/Fonts/seguiemj.ttf";
    if (!File::Exists(ColorFontPath))
    {
        return;
    }

    Vector<uint8_t> fontBytes;
    HE_ASSERT(ReadFontFile(fontBytes, ColorFontPath));

    CompiledFontRenderData renderData{};
    HE_ASSERT(BuildCompiledFontRenderData(renderData, fontBytes, 0));
    HE_EXPECT_GT(renderData.paint.palettes.Size(), 0u);
    HE_EXPECT_GT(renderData.paint.layers.Size(), 0u);

    for (uint32_t glyphId : ColorGlyphIds)
    {
        HE_EXPECT_LT(glyphId, renderData.paint.colorGlyphs.Size());
        HE_EXPECT_GT(renderData.paint.colorGlyphs[glyphId].layerCount, 0u);
    }
}

HE_TEST(scribe, color_font_pipeline, compiles_repeatable_repo_font_payloads)
{
    String repoFontPath;
    HE_ASSERT(ResolveRepoFontPath(repoFontPath, "NotoSans-Regular.ttf"));

    Vector<uint8_t> fontBytes;
    HE_ASSERT(ReadFontFile(fontBytes, repoFontPath.Data()));

    CompiledFontRenderData first{};
    CompiledFontRenderData second{};
    HE_ASSERT(BuildCompiledFontRenderData(first, fontBytes, 0));
    HE_ASSERT(BuildCompiledFontRenderData(second, fontBytes, 0));

    HE_EXPECT_EQ(first.curveTexels.Size(), second.curveTexels.Size());
    HE_EXPECT_EQ(first.bandTexels.Size(), second.bandTexels.Size());
    HE_EXPECT_EQ(first.glyphs.Size(), second.glyphs.Size());
    HE_EXPECT_EQ(first.bandHeaderCount, second.bandHeaderCount);
    HE_EXPECT_EQ(first.emittedBandPayloadTexelCount, second.emittedBandPayloadTexelCount);
    HE_EXPECT_EQ(first.reusedBandCount, second.reusedBandCount);
    HE_EXPECT_EQ(first.reusedBandPayloadTexelCount, second.reusedBandPayloadTexelCount);

    HE_EXPECT_EQ_MEM(
        first.curveTexels.Data(),
        second.curveTexels.Data(),
        first.curveTexels.Size() * sizeof(PackedCurveTexel));
    HE_EXPECT_EQ_MEM(
        first.bandTexels.Data(),
        second.bandTexels.Data(),
        first.bandTexels.Size() * sizeof(PackedBandTexel));
    HE_EXPECT_EQ_MEM(
        first.glyphs.Data(),
        second.glyphs.Data(),
        first.glyphs.Size() * sizeof(CompiledGlyphRenderEntry));
}

HE_TEST(scribe, color_font_pipeline, compiled_capital_t_bounds_match_freetype_outline_bbox)
{
    String repoFontPath;
    HE_ASSERT(ResolveRepoFontPath(repoFontPath, "NotoSans-Regular.ttf"));

    Vector<uint8_t> fontBytes;
    HE_ASSERT(ReadFontFile(fontBytes, repoFontPath.Data()));

    Vector<schema::Word> storage;
    FontFaceResourceReader font{};
    HE_ASSERT(BuildLoadedCompiledFontFace(storage, font, fontBytes, "Noto Sans"));

    ScribeContext context{};
    Vector<FontFaceHandle> handles{};
    HE_ASSERT(RegisterFontFaces(context, Span<const FontFaceResourceReader>(&font, 1), handles));
        LayoutEngine& engine = context.GetLayoutEngine();
    LayoutResult layout;
    LayoutOptions options{};
    options.fontSize = 96.0f;
    options.wrap = false;
    HE_ASSERT(engine.LayoutText(layout, Span<const FontFaceHandle>(handles.Data(), handles.Size()), "T", options));
    HE_ASSERT(layout.glyphs.Size() == 1);

    const uint32_t glyphIndex = layout.glyphs[0].glyphIndex;

    CompiledFontRenderData renderData{};
    HE_ASSERT(BuildCompiledFontRenderData(renderData, fontBytes, 0));
    HE_ASSERT(glyphIndex < renderData.glyphs.Size());

    FreeTypeScope freeType;
    HE_ASSERT(freeType.Initialize(fontBytes));
    HE_ASSERT(FT_Load_Glyph(
        freeType.GetFace(),
        static_cast<FT_UInt>(glyphIndex),
        FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING | FT_LOAD_IGNORE_TRANSFORM) == 0);

    const FT_GlyphSlot glyph = freeType.GetFace()->glyph;
    HE_ASSERT(glyph->format == FT_GLYPH_FORMAT_OUTLINE);

    FT_BBox bbox{};
    bbox.xMin = glyph->outline.points[0].x;
    bbox.yMin = glyph->outline.points[0].y;
    bbox.xMax = glyph->outline.points[0].x;
    bbox.yMax = glyph->outline.points[0].y;
    for (int pointIndex = 1; pointIndex < glyph->outline.n_points; ++pointIndex)
    {
        const FT_Vector point = glyph->outline.points[pointIndex];
        bbox.xMin = Min(bbox.xMin, point.x);
        bbox.yMin = Min(bbox.yMin, point.y);
        bbox.xMax = Max(bbox.xMax, point.x);
        bbox.yMax = Max(bbox.yMax, point.y);
    }

    const CompiledGlyphRenderEntry& compiledGlyph = renderData.glyphs[glyphIndex];
    constexpr float Tolerance = 2.1f;
    HE_EXPECT_LE(Abs(compiledGlyph.boundsMinX - static_cast<float>(bbox.xMin)), Tolerance);
    HE_EXPECT_LE(Abs(compiledGlyph.boundsMinY - static_cast<float>(bbox.yMin)), Tolerance);
    HE_EXPECT_LE(Abs(compiledGlyph.boundsMaxX - static_cast<float>(bbox.xMax)), Tolerance);
    HE_EXPECT_LE(Abs(compiledGlyph.boundsMaxY - static_cast<float>(bbox.yMax)), Tolerance);
}

HE_TEST(scribe, color_font_pipeline, compiled_capital_t_has_no_detached_left_edge_coverage)
{
    String repoFontPath;
    HE_ASSERT(ResolveRepoFontPath(repoFontPath, "NotoSans-Regular.ttf"));

    Vector<uint8_t> fontBytes;
    HE_ASSERT(ReadFontFile(fontBytes, repoFontPath.Data()));

    Vector<schema::Word> storage;
    FontFaceResourceReader font{};
    HE_ASSERT(BuildLoadedCompiledFontFace(storage, font, fontBytes, "Noto Sans"));

    ScribeContext context{};
    Vector<FontFaceHandle> handles{};
    HE_ASSERT(RegisterFontFaces(context, Span<const FontFaceResourceReader>(&font, 1), handles));
    LayoutEngine& engine = context.GetLayoutEngine();
    LayoutResult layout;
    LayoutOptions options{};
    options.fontSize = 96.0f;
    options.wrap = false;
    HE_ASSERT(engine.LayoutText(layout, Span<const FontFaceHandle>(handles.Data(), handles.Size()), "T", options));
    HE_ASSERT(layout.glyphs.Size() == 1);

    const uint32_t glyphIndex = layout.glyphs[0].glyphIndex;
    const float pixelsPerUnitValue = options.fontSize / static_cast<float>(Max(font.GetMetadata().GetUnitsPerEm(), 1u));
    const Vec2f pixelsPerUnit = { pixelsPerUnitValue, pixelsPerUnitValue };
    CompiledFontRenderData renderData{};
    HE_ASSERT(BuildCompiledFontRenderData(renderData, fontBytes, 0));
    HE_ASSERT(glyphIndex < renderData.glyphs.Size());

    const CompiledGlyphRenderEntry& glyph = renderData.glyphs[glyphIndex];
    float maxCoverageOutsideLeft = 0.0f;
    float maxCoverageOutsideLeftX = 0.0f;
    float maxCoverageOutsideLeftY = 0.0f;
    for (uint32_t yIndex = 0; yIndex < 64u; ++yIndex)
    {
        const float t = (static_cast<float>(yIndex) + 0.5f) / 64.0f;
        const float sampleY = Lerp(glyph.boundsMinY, glyph.boundsMaxY, t);
        for (uint32_t xIndex = 0; xIndex < 32u; ++xIndex)
        {
            const float sampleX = glyph.boundsMinX - 8.0f + (static_cast<float>(xIndex) * 0.25f);
            const float coverage = EvaluateGlyphCoverageCpu(renderData, glyph, sampleX, sampleY, pixelsPerUnit);
            if (coverage > maxCoverageOutsideLeft)
            {
                maxCoverageOutsideLeft = coverage;
                maxCoverageOutsideLeftX = sampleX;
                maxCoverageOutsideLeftY = sampleY;
            }
        }
    }

    FreeTypeScope freeType{};
    HE_ASSERT(freeType.Initialize(fontBytes));

    FT_Face face = freeType.GetFace();
    HE_ASSERT(face != nullptr);
    HE_ASSERT(FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP) == 0);
    HE_ASSERT(face->glyph);
    HE_ASSERT(face->glyph->format == FT_GLYPH_FORMAT_OUTLINE);

    FT_BBox bbox{};
    bbox.xMin = Limits<int32_t>::Max;
    bbox.yMin = Limits<int32_t>::Max;
    bbox.xMax = Limits<int32_t>::Min;
    bbox.yMax = Limits<int32_t>::Min;

    const FT_Outline& outline = face->glyph->outline;
    for (int32_t pointIndex = 0; pointIndex < outline.n_points; ++pointIndex)
    {
        const FT_Vector& point = outline.points[pointIndex];
        bbox.xMin = Min(bbox.xMin, point.x);
        bbox.yMin = Min(bbox.yMin, point.y);
        bbox.xMax = Max(bbox.xMax, point.x);
        bbox.yMax = Max(bbox.yMax, point.y);
    }

    const float outlineMinX = static_cast<float>(bbox.xMin);
    float maxCoverageInsideLeftMargin = 0.0f;
    if (glyph.boundsMinX < outlineMinX)
    {
        for (uint32_t yIndex = 0; yIndex < 64u; ++yIndex)
        {
            const float t = (static_cast<float>(yIndex) + 0.5f) / 64.0f;
            const float sampleY = Lerp(glyph.boundsMinY, glyph.boundsMaxY, t);
            for (uint32_t xIndex = 0; xIndex < 32u; ++xIndex)
            {
                const float s = (static_cast<float>(xIndex) + 0.5f) / 32.0f;
                const float sampleX = Lerp(glyph.boundsMinX, outlineMinX, s);
                maxCoverageInsideLeftMargin = Max(
                    maxCoverageInsideLeftMargin,
                    EvaluateGlyphCoverageCpu(renderData, glyph, sampleX, sampleY, pixelsPerUnit));
            }
        }
    }

    HE_LOG_INFO(he_scribe,
        HE_MSG("Compiled T left-edge coverage diagnostics."),
        HE_KV(bounds_min_x, glyph.boundsMinX),
        HE_KV(bounds_min_y, glyph.boundsMinY),
        HE_KV(bounds_max_x, glyph.boundsMaxX),
        HE_KV(bounds_max_y, glyph.boundsMaxY),
        HE_KV(outline_min_x, outlineMinX),
        HE_KV(max_coverage_outside_left, maxCoverageOutsideLeft),
        HE_KV(sample_x, maxCoverageOutsideLeftX),
        HE_KV(sample_y, maxCoverageOutsideLeftY));
    DumpGlyphBandCoverageCpu(renderData, glyph, maxCoverageOutsideLeftX, maxCoverageOutsideLeftY, pixelsPerUnit);
    DumpGlyphCoverageSliceCpu(
        renderData,
        glyph,
        glyph.boundsMinX - 2.0f,
        glyph.boundsMinX + 6.0f,
        maxCoverageOutsideLeftY,
        pixelsPerUnit,
        0.125f);
    DumpNamedGlyphCoverageSliceCpu(
        renderData,
        glyph,
        227.0f,
        239.0f,
        320.0f,
        pixelsPerUnit,
        0.125f,
        "stem_left_mid");
    DumpNamedGlyphCoverageSliceCpu(
        renderData,
        glyph,
        317.0f,
        329.0f,
        320.0f,
        pixelsPerUnit,
        0.125f,
        "stem_right_mid");
    DumpNamedGlyphCoverageSliceCpu(
        renderData,
        glyph,
        4.0f,
        16.0f,
        320.0f,
        pixelsPerUnit,
        0.125f,
        "bounds_left_mid");
    DumpGlyphBandCoverageCpu(
        renderData,
        glyph,
        9.75f,
        320.0f,
        pixelsPerUnit);

    HE_EXPECT_LE(maxCoverageOutsideLeft, 1.0e-3f);
    HE_EXPECT_LE(maxCoverageInsideLeftMargin, 1.0e-3f);
}

HE_TEST(scribe, color_font_pipeline, segoeui_capital_t_mid_left_bounds_coverage_diagnostic)
{
    static constexpr const char* SegoeUiPath = "C:/Windows/Fonts/segoeui.ttf";
    if (!File::Exists(SegoeUiPath))
    {
        return;
    }

    Vector<uint8_t> fontBytes;
    HE_ASSERT(ReadFontFile(fontBytes, SegoeUiPath));

    Vector<schema::Word> storage;
    FontFaceResourceReader font{};
    HE_ASSERT(BuildLoadedCompiledFontFace(storage, font, fontBytes, "Segoe UI"));

    ScribeContext context{};
    Vector<FontFaceHandle> handles{};
    HE_ASSERT(RegisterFontFaces(context, Span<const FontFaceResourceReader>(&font, 1), handles));
    LayoutEngine engine(context);
    LayoutResult layout;
    LayoutOptions options{};
    options.fontSize = 96.0f;
    options.wrap = false;
    HE_ASSERT(engine.LayoutText(layout, Span<const FontFaceHandle>(handles.Data(), handles.Size()), "T", options));
    HE_ASSERT(layout.glyphs.Size() == 1);

    const uint32_t glyphIndex = layout.glyphs[0].glyphIndex;
    const float pixelsPerUnitValue = options.fontSize / static_cast<float>(Max(font.GetMetadata().GetUnitsPerEm(), 1u));
    const Vec2f pixelsPerUnit = { pixelsPerUnitValue, pixelsPerUnitValue };

    CompiledFontRenderData renderData{};
    HE_ASSERT(BuildCompiledFontRenderData(renderData, fontBytes, 0));
    HE_ASSERT(glyphIndex < renderData.glyphs.Size());

    const CompiledGlyphRenderEntry& glyph = renderData.glyphs[glyphIndex];
    const float sampleY = Lerp(glyph.boundsMinY, glyph.boundsMaxY, 0.45f);
    float maxCoverage = 0.0f;
    for (float sampleX = glyph.boundsMinX - 2.0f; sampleX <= glyph.boundsMinX + 6.0f; sampleX += 0.125f)
    {
        maxCoverage = Max(maxCoverage, EvaluateGlyphCoverageCpu(renderData, glyph, sampleX, sampleY, pixelsPerUnit));
    }

    HE_LOG_INFO(he_scribe,
        HE_MSG("Segoe UI T mid-left bounds coverage diagnostic."),
        HE_KV(bounds_min_x, glyph.boundsMinX),
        HE_KV(bounds_min_y, glyph.boundsMinY),
        HE_KV(bounds_max_x, glyph.boundsMaxX),
        HE_KV(bounds_max_y, glyph.boundsMaxY),
        HE_KV(sample_y, sampleY),
        HE_KV(max_coverage, maxCoverage));
    DumpNamedGlyphCoverageSliceCpu(
        renderData,
        glyph,
        glyph.boundsMinX - 2.0f,
        glyph.boundsMinX + 6.0f,
        sampleY,
        pixelsPerUnit,
        0.125f,
        "segoeui_bounds_left");
}

HE_TEST(scribe, color_font_pipeline, resolves_compiled_layers_from_runtime_resource)
{
    static constexpr const char* ColorFontPath = "C:/Windows/Fonts/seguiemj.ttf";
    if (!File::Exists(ColorFontPath))
    {
        return;
    }

    Vector<uint8_t> fontBytes;
    HE_ASSERT(ReadFontFile(fontBytes, ColorFontPath));

    Vector<schema::Word> storage;
    FontFaceResourceReader font{};
    HE_ASSERT(BuildLoadedCompiledFontFace(storage, font, fontBytes, "seguiemj.ttf"));

    const uint32_t paletteIndex = SelectCompiledFontPalette(font, true);
    Vector<CompiledColorGlyphLayer> layers{};
    for (uint32_t glyphId : ColorGlyphIds)
    {
        HE_EXPECT(GetCompiledColorGlyphLayers(layers, font, glyphId, paletteIndex));
        HE_EXPECT_GT(layers.Size(), 0u);

        bool sawNonWhite = false;
        for (const CompiledColorGlyphLayer& layer : layers)
        {
            if ((layer.color.x != 1.0f) || (layer.color.y != 1.0f) || (layer.color.z != 1.0f))
            {
                sawNonWhite = true;
                break;
            }
        }

        HE_EXPECT(sawNonWhite);
    }
}

HE_TEST(scribe, color_font_pipeline, layout_prefers_color_face_for_emoji_scene)
{
    static constexpr const char* ColorFontPath = "C:/Windows/Fonts/seguiemj.ttf";
    if (!File::Exists(ColorFontPath))
    {
        return;
    }

    String repoFontPath;
    HE_ASSERT(ResolveRepoFontPath(repoFontPath, "NotoSans-Regular.ttf"));

    Vector<uint8_t> repoFontBytes;
    HE_ASSERT(ReadFontFile(repoFontBytes, repoFontPath.Data()));

    Vector<uint8_t> colorFontBytes;
    HE_ASSERT(ReadFontFile(colorFontBytes, ColorFontPath));

    Vector<schema::Word> repoStorage;
    Vector<schema::Word> colorStorage;
    FontFaceResourceReader repoFont{};
    FontFaceResourceReader colorFont{};
    HE_ASSERT(BuildLoadedCompiledFontFace(repoStorage, repoFont, repoFontBytes, "NotoSans-Regular.ttf"));
    HE_ASSERT(BuildLoadedCompiledFontFace(colorStorage, colorFont, colorFontBytes, "seguiemj.ttf"));

    const FontFaceResourceReader faces[] =
    {
        repoFont,
        colorFont,
    };

    ScribeContext context{};
    Vector<FontFaceHandle> handles{};
    HE_ASSERT(RegisterFontFaces(context, Span<const FontFaceResourceReader>(faces), handles));
    LayoutEngine engine(context);
    LayoutResult layout;
    LayoutOptions options{};
    options.fontSize = 24.0f;
    options.maxWidth = 1024.0f;
    options.wrap = false;
    options.direction = TextDirection::Auto;

    HE_ASSERT(engine.LayoutText(
        layout,
        Span<const FontFaceHandle>(handles.Data(), handles.Size()),
        "\xF0\x9F\x99\x82 \xF0\x9F\x98\x80 \xF0\x9F\x8E\xA8 \xF0\x9F\x8C\x88 \xE2\x9C\xA8",
        options));

    uint32_t coloredGlyphCount = 0;
    for (const ShapedGlyph& glyph : layout.glyphs)
    {
        if (glyph.glyphIndex == 3)
        {
            continue;
        }

        HE_EXPECT_EQ(glyph.fontFaceIndex, 1u);
        ++coloredGlyphCount;
    }

    HE_EXPECT_EQ(coloredGlyphCount, 5u);
}

HE_TEST(scribe, color_font_pipeline, shaped_emoji_scene_resolves_nonwhite_layers)
{
    static constexpr const char* ColorFontPath = "C:/Windows/Fonts/seguiemj.ttf";
    if (!File::Exists(ColorFontPath))
    {
        return;
    }

    String repoFontPath;
    HE_ASSERT(ResolveRepoFontPath(repoFontPath, "NotoSans-Regular.ttf"));

    Vector<uint8_t> repoFontBytes;
    HE_ASSERT(ReadFontFile(repoFontBytes, repoFontPath.Data()));

    Vector<uint8_t> colorFontBytes;
    HE_ASSERT(ReadFontFile(colorFontBytes, ColorFontPath));

    Vector<schema::Word> repoStorage;
    Vector<schema::Word> colorStorage;
    FontFaceResourceReader repoFont{};
    FontFaceResourceReader colorFont{};
    HE_ASSERT(BuildLoadedCompiledFontFace(repoStorage, repoFont, repoFontBytes, "NotoSans-Regular.ttf"));
    HE_ASSERT(BuildLoadedCompiledFontFace(colorStorage, colorFont, colorFontBytes, "seguiemj.ttf"));

    const FontFaceResourceReader faces[] =
    {
        repoFont,
        colorFont,
    };

    ScribeContext context{};
    Vector<FontFaceHandle> handles{};
    HE_ASSERT(RegisterFontFaces(context, Span<const FontFaceResourceReader>(faces), handles));
    LayoutEngine engine(context);
    LayoutResult layout;
    LayoutOptions options{};
    options.fontSize = 24.0f;
    options.maxWidth = 1024.0f;
    options.wrap = false;
    options.direction = TextDirection::Auto;

    HE_ASSERT(engine.LayoutText(
        layout,
        Span<const FontFaceHandle>(handles.Data(), handles.Size()),
        "\xF0\x9F\x99\x82 \xF0\x9F\x98\x80 \xF0\x9F\x8E\xA8 \xF0\x9F\x8C\x88 \xE2\x9C\xA8",
        options));

    Vector<CompiledColorGlyphLayer> layers{};
    const uint32_t paletteIndex = SelectCompiledFontPalette(colorFont, true);
    uint32_t resolvedGlyphCount = 0;

    for (const ShapedGlyph& glyph : layout.glyphs)
    {
        if (glyph.glyphIndex == 3)
        {
            continue;
        }

        HE_EXPECT_EQ(glyph.fontFaceIndex, 1u);
        HE_EXPECT(GetCompiledColorGlyphLayers(layers, colorFont, glyph.glyphIndex, paletteIndex));
        HE_EXPECT_GT(layers.Size(), 0u);

        bool sawNonWhite = false;
        for (const CompiledColorGlyphLayer& layer : layers)
        {
            if ((layer.color.x != 1.0f) || (layer.color.y != 1.0f) || (layer.color.z != 1.0f))
            {
                sawNonWhite = true;
                break;
            }
        }

        HE_EXPECT(sawNonWhite);
        ++resolvedGlyphCount;
    }

    HE_EXPECT_EQ(resolvedGlyphCount, 5u);
}

HE_TEST(scribe, retained_text, builds_monochrome_draws_from_layout)
{
    String repoFontPath;
    HE_ASSERT(ResolveRepoFontPath(repoFontPath, "NotoSans-Regular.ttf"));

    Vector<uint8_t> fontBytes;
    HE_ASSERT(ReadFontFile(fontBytes, repoFontPath.Data()));

    Vector<schema::Word> storage;
    FontFaceResourceReader font{};
    HE_ASSERT(BuildLoadedCompiledFontFace(storage, font, fontBytes, "Noto Sans"));

    ScribeContext context{};
    Vector<FontFaceHandle> handles{};
    HE_ASSERT(RegisterFontFaces(context, Span<const FontFaceResourceReader>(&font, 1), handles));
    LayoutEngine engine(context);
    LayoutResult layout;
    LayoutOptions options{};
    options.fontSize = 28.0f;
    options.wrap = false;
    HE_ASSERT(engine.LayoutText(layout, Span<const FontFaceHandle>(handles.Data(), handles.Size()), "Retained text", options));

    RetainedTextModel retainedText;
    RetainedTextBuildDesc desc{};
    desc.context = &context;
    desc.fontFaces = Span<const FontFaceHandle>(handles.Data(), handles.Size());
    desc.layout = &layout;
    desc.fontSize = options.fontSize;
    HE_ASSERT(retainedText.Build(desc));

    HE_EXPECT_EQ(retainedText.GetDrawCount(), layout.glyphs.Size());
    HE_EXPECT_EQ(retainedText.GetEstimatedVertexCount(), retainedText.GetDrawCount() * ScribeGlyphVertexCount);
    for (const RetainedTextDraw& draw : retainedText.GetDraws())
    {
        HE_EXPECT_EQ(draw.fontFaceIndex, 0u);
        HE_EXPECT((draw.flags & RetainedTextDrawFlagUseForegroundColor) != 0);
        HE_EXPECT_EQ(draw.color.x, 1.0f);
        HE_EXPECT_EQ(draw.color.y, 1.0f);
        HE_EXPECT_EQ(draw.color.z, 1.0f);
        HE_EXPECT_EQ(draw.color.w, 1.0f);
    }
}

HE_TEST(scribe, retained_text, applies_styled_run_color_and_transform)
{
    String sansPath;
    String monoPath;
    HE_ASSERT(ResolveRepoFontPath(sansPath, "NotoSans-Regular.ttf"));
    HE_ASSERT(ResolveRepoFontPath(monoPath, "NotoMono-Regular.ttf"));

    Vector<uint8_t> sansBytes;
    Vector<uint8_t> monoBytes;
    HE_ASSERT(ReadFontFile(sansBytes, sansPath.Data()));
    HE_ASSERT(ReadFontFile(monoBytes, monoPath.Data()));

    Vector<schema::Word> sansStorage;
    Vector<schema::Word> monoStorage;
    FontFaceResourceReader sans{};
    FontFaceResourceReader mono{};
    HE_ASSERT(BuildLoadedCompiledFontFace(sansStorage, sans, sansBytes, "Noto Sans"));
    HE_ASSERT(BuildLoadedCompiledFontFace(monoStorage, mono, monoBytes, "Noto Mono"));

    const FontFaceResourceReader faces[] = { sans, mono };
    ScribeContext context{};
    Vector<FontFaceHandle> handles{};
    HE_ASSERT(RegisterFontFaces(context, Span<const FontFaceResourceReader>(faces, HE_LENGTH_OF(faces)), handles));
    const String text = "alpha beta";
    const uint32_t betaStart = 6;
    const uint32_t betaEnd = 10;

    TextStyle styles[2]{};
    styles[1].fontFaceIndex = 1;
    styles[1].color = { 0.20f, 0.45f, 0.90f, 1.0f };
    styles[1].stretchX = 1.2f;
    styles[1].stretchY = 0.9f;
    styles[1].skewX = 0.25f;
    styles[1].rotationRadians = 0.2f;
    styles[1].baselineShiftEm = 0.15f;
    styles[1].glyphScale = 0.9f;

    const TextStyleSpan spans[] =
    {
        { betaStart, betaEnd, 1 }
    };

    StyledTextLayoutDesc layoutDesc{};
    layoutDesc.fontFaces = Span<const FontFaceHandle>(handles.Data(), handles.Size());
    layoutDesc.text = text;
    layoutDesc.styles = Span<const TextStyle>(styles, HE_LENGTH_OF(styles));
    layoutDesc.styleSpans = Span<const TextStyleSpan>(spans, HE_LENGTH_OF(spans));
    layoutDesc.options.fontSize = 28.0f;
    layoutDesc.options.wrap = false;

    LayoutEngine engine(context);
    LayoutResult layout;
    HE_ASSERT(engine.LayoutStyledText(layout, layoutDesc));

    RetainedTextModel retainedText;
    RetainedTextBuildDesc retainedDesc{};
    retainedDesc.context = &context;
    retainedDesc.fontFaces = Span<const FontFaceHandle>(handles.Data(), handles.Size());
    retainedDesc.layout = &layout;
    retainedDesc.fontSize = layoutDesc.options.fontSize;
    retainedDesc.styles = Span<const TextStyle>(styles, HE_LENGTH_OF(styles));
    HE_ASSERT(retainedText.Build(retainedDesc));

    bool sawStyledDraw = false;
    for (const RetainedTextDraw& draw : retainedText.GetDraws())
    {
        if (draw.fontFaceIndex != 1u)
        {
            continue;
        }

        sawStyledDraw = true;
        HE_EXPECT_EQ(draw.color.x, styles[1].color.x);
        HE_EXPECT_EQ(draw.color.y, styles[1].color.y);
        HE_EXPECT_EQ(draw.color.z, styles[1].color.z);
        HE_EXPECT_EQ(draw.color.w, styles[1].color.w);
        HE_EXPECT_LT(draw.position.y, layout.glyphs[0].position.y + layoutDesc.options.fontSize);
        HE_EXPECT_NE(draw.basisX.x, 1.0f);
        HE_EXPECT_NE(draw.basisY.x, 0.0f);
    }

    HE_EXPECT(sawStyledDraw);
}

HE_TEST(scribe, retained_text, textsub_demo_line_starts_are_the_leftmost_rendered_glyphs)
{
    String repoFontPath;
    HE_ASSERT(ResolveRepoFontPath(repoFontPath, "NotoSans-Regular.ttf"));

    Vector<uint8_t> fontBytes;
    HE_ASSERT(ReadFontFile(fontBytes, repoFontPath.Data()));

    Vector<schema::Word> storage;
    FontFaceResourceReader font{};
    HE_ASSERT(BuildLoadedCompiledFontFace(storage, font, fontBytes, "Noto Sans"));
    ScribeContext context{};
    Vector<FontFaceHandle> handles{};
    HE_ASSERT(RegisterFontFaces(context, Span<const FontFaceResourceReader>(&font, 1), handles));

    const FontFaceResourceReader faces[] = { font };
    const String text = "TextSub1 Sub2\nTextSup1 Sup2";

    TextStyle styles[5]{};
    styles[1].baselineShiftEm = 0.20f;
    styles[1].glyphScale = 0.72f;
    styles[2].baselineShiftEm = 0.20f;
    styles[2].glyphScale = 0.60f;
    styles[3].baselineShiftEm = -0.35f;
    styles[3].glyphScale = 0.72f;
    styles[4].baselineShiftEm = -0.35f;
    styles[4].glyphScale = 0.60f;

    const TextStyleSpan spans[] =
    {
        { 4u, 8u, 1u },
        { 9u, 13u, 2u },
        { 18u, 22u, 3u },
        { 23u, 27u, 4u },
    };

    StyledTextLayoutDesc layoutDesc{};
    layoutDesc.fontFaces = Span<const FontFaceHandle>(handles.Data(), handles.Size());
    layoutDesc.text = text;
    layoutDesc.styles = Span<const TextStyle>(styles, HE_LENGTH_OF(styles));
    layoutDesc.styleSpans = Span<const TextStyleSpan>(spans, HE_LENGTH_OF(spans));
    layoutDesc.options.fontSize = 24.0f;
    layoutDesc.options.wrap = false;

    LayoutEngine engine(context);
    LayoutResult layout;
    HE_ASSERT(engine.LayoutStyledText(layout, layoutDesc));
    HE_EXPECT_EQ(layout.lines.Size(), 2u);

    RetainedTextModel retainedText;
    RetainedTextBuildDesc retainedDesc{};
    retainedDesc.context = &context;
    retainedDesc.fontFaces = Span<const FontFaceHandle>(handles.Data(), handles.Size());
    retainedDesc.layout = &layout;
    retainedDesc.fontSize = layoutDesc.options.fontSize;
    retainedDesc.styles = Span<const TextStyle>(styles, HE_LENGTH_OF(styles));
    HE_ASSERT(retainedText.Build(retainedDesc));
    HE_EXPECT_EQ(retainedText.GetDrawCount(), layout.glyphs.Size());

    CompiledFontRenderData renderData{};
    HE_ASSERT(BuildCompiledFontRenderData(renderData, fontBytes, 0));

    for (uint32_t lineIndex = 0; lineIndex < layout.lines.Size(); ++lineIndex)
    {
        const TextLine& line = layout.lines[lineIndex];
        const TextCluster& firstCluster = layout.clusters[line.clusterStart];
        const uint32_t firstGlyphIndex = firstCluster.glyphStart;
        HE_ASSERT(firstGlyphIndex < layout.glyphs.Size());
        HE_ASSERT(firstGlyphIndex < retainedText.GetDrawCount());

        const ShapedGlyph& firstGlyph = layout.glyphs[firstGlyphIndex];
        HE_ASSERT(firstGlyph.glyphIndex < renderData.glyphs.Size());

        float minRenderedX = Limits<float>::Max;
        uint32_t minRenderedGlyphLayoutIndex = Limits<uint32_t>::Max;
        for (uint32_t glyphLayoutIndex = 0; glyphLayoutIndex < layout.glyphs.Size(); ++glyphLayoutIndex)
        {
            const ShapedGlyph& glyph = layout.glyphs[glyphLayoutIndex];
            if (glyph.lineIndex != lineIndex)
            {
                continue;
            }

            const RetainedTextDraw& draw = retainedText.GetDraws()[glyphLayoutIndex];
            const CompiledGlyphRenderEntry& compiledGlyph = renderData.glyphs[glyph.glyphIndex];
            const float renderedMinX = draw.position.x + (compiledGlyph.boundsMinX * draw.size.x);
            if (renderedMinX < minRenderedX)
            {
                minRenderedX = renderedMinX;
                minRenderedGlyphLayoutIndex = glyphLayoutIndex;
            }
        }

        HE_EXPECT_EQ(minRenderedGlyphLayoutIndex, firstGlyphIndex);
        HE_EXPECT_EQ(layout.glyphs[minRenderedGlyphLayoutIndex].textByteStart, firstGlyph.textByteStart);
        HE_EXPECT_EQ(layout.glyphs[minRenderedGlyphLayoutIndex].glyphIndex, firstGlyph.glyphIndex);
    }
}

HE_TEST(scribe, retained_text, emits_decoration_quads_for_styled_runs)
{
    String repoFontPath;
    HE_ASSERT(ResolveRepoFontPath(repoFontPath, "NotoSans-Regular.ttf"));

    Vector<uint8_t> fontBytes;
    HE_ASSERT(ReadFontFile(fontBytes, repoFontPath.Data()));

    Vector<schema::Word> storage;
    FontFaceResourceReader font{};
    HE_ASSERT(BuildLoadedCompiledFontFace(storage, font, fontBytes, "Noto Sans"));
    ScribeContext context{};
    Vector<FontFaceHandle> handles{};
    HE_ASSERT(RegisterFontFaces(context, Span<const FontFaceResourceReader>(&font, 1), handles));

    const FontFaceResourceReader faces[] = { font };
    const String text = "Underline\nStrike-through";

    TextStyle styles[3]{};
    styles[1].decorations = TextDecorationFlags::Underline;
    styles[1].decorationColor = { 0.10f, 0.20f, 0.30f, 1.0f };
    styles[2].decorations = TextDecorationFlags::Strikethrough;
    styles[2].decorationColor = { 0.70f, 0.10f, 0.20f, 1.0f };

    const TextStyleSpan spans[] =
    {
        { 0, 9, 1 },
        { 10, 24, 2 },
    };

    StyledTextLayoutDesc layoutDesc{};
    layoutDesc.fontFaces = Span<const FontFaceHandle>(handles.Data(), handles.Size());
    layoutDesc.text = text;
    layoutDesc.styles = Span<const TextStyle>(styles, HE_LENGTH_OF(styles));
    layoutDesc.styleSpans = Span<const TextStyleSpan>(spans, HE_LENGTH_OF(spans));
    layoutDesc.options.fontSize = 28.0f;
    layoutDesc.options.wrap = false;

    LayoutEngine engine(context);
    LayoutResult layout;
    HE_ASSERT(engine.LayoutStyledText(layout, layoutDesc));

    RetainedTextModel retainedText;
    RetainedTextBuildDesc retainedDesc{};
    retainedDesc.context = &context;
    retainedDesc.fontFaces = Span<const FontFaceHandle>(handles.Data(), handles.Size());
    retainedDesc.layout = &layout;
    retainedDesc.fontSize = layoutDesc.options.fontSize;
    retainedDesc.styles = Span<const TextStyle>(styles, HE_LENGTH_OF(styles));
    HE_ASSERT(retainedText.Build(retainedDesc));

    HE_EXPECT_GE(retainedText.GetQuadCount(), 2u);
    HE_EXPECT_GE(retainedText.GetEstimatedVertexCount(), (retainedText.GetDrawCount() * ScribeGlyphVertexCount) + (2u * 6u));

    bool sawUnderlineColor = false;
    bool sawStrikeColor = false;
    for (const RetainedTextQuad& quad : retainedText.GetQuads())
    {
        sawUnderlineColor |= (quad.color.x == styles[1].decorationColor.x)
            && (quad.color.y == styles[1].decorationColor.y)
            && (quad.color.z == styles[1].decorationColor.z);
        sawStrikeColor |= (quad.color.x == styles[2].decorationColor.x)
            && (quad.color.y == styles[2].decorationColor.y)
            && (quad.color.z == styles[2].decorationColor.z);
    }

    HE_EXPECT(sawUnderlineColor);
    HE_EXPECT(sawStrikeColor);
}

HE_TEST(scribe, retained_text, expands_shadow_and_outline_effects_into_extra_draws)
{
    String repoFontPath;
    HE_ASSERT(ResolveRepoFontPath(repoFontPath, "NotoSans-Regular.ttf"));

    Vector<uint8_t> fontBytes;
    HE_ASSERT(ReadFontFile(fontBytes, repoFontPath.Data()));

    Vector<schema::Word> storage;
    FontFaceResourceReader font{};
    HE_ASSERT(BuildLoadedCompiledFontFace(storage, font, fontBytes, "Noto Sans"));
    ScribeContext context{};
    Vector<FontFaceHandle> handles{};
    HE_ASSERT(RegisterFontFaces(context, Span<const FontFaceResourceReader>(&font, 1), handles));

    const FontFaceResourceReader faces[] = { font };
    const String text = "Both";

    TextStyle styles[2]{};
    styles[1].effects = TextEffectFlags::Shadow | TextEffectFlags::Outline;
    styles[1].color = { 0.90f, 0.20f, 0.10f, 1.0f };
    styles[1].shadowColor = { 0.0f, 0.0f, 0.0f, 0.25f };
    styles[1].shadowOffsetEm = { 0.08f, 0.06f };
    styles[1].outlineColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    styles[1].outlineWidthEm = 0.05f;

    const TextStyleSpan spans[] =
    {
        { 0, static_cast<uint32_t>(text.Size()), 1 },
    };

    StyledTextLayoutDesc layoutDesc{};
    layoutDesc.fontFaces = Span<const FontFaceHandle>(handles.Data(), handles.Size());
    layoutDesc.text = text;
    layoutDesc.styles = Span<const TextStyle>(styles, HE_LENGTH_OF(styles));
    layoutDesc.styleSpans = Span<const TextStyleSpan>(spans, HE_LENGTH_OF(spans));
    layoutDesc.options.fontSize = 40.0f;
    layoutDesc.options.wrap = false;

    LayoutEngine engine(context);
    LayoutResult layout;
    HE_ASSERT(engine.LayoutStyledText(layout, layoutDesc));

    RetainedTextModel retainedText;
    RetainedTextBuildDesc retainedDesc{};
    retainedDesc.context = &context;
    retainedDesc.fontFaces = Span<const FontFaceHandle>(handles.Data(), handles.Size());
    retainedDesc.layout = &layout;
    retainedDesc.fontSize = layoutDesc.options.fontSize;
    retainedDesc.styles = Span<const TextStyle>(styles, HE_LENGTH_OF(styles));
    HE_ASSERT(retainedText.Build(retainedDesc));

    const uint32_t glyphCount = layout.glyphs.Size();
    HE_EXPECT_EQ(retainedText.GetDrawCount(), glyphCount * 10u);

    uint32_t outlineLikeDraws = 0;
    uint32_t shadowLikeDraws = 0;
    uint32_t fillDraws = 0;
    for (const RetainedTextDraw& draw : retainedText.GetDraws())
    {
        if ((draw.color.x == styles[1].color.x)
            && (draw.color.y == styles[1].color.y)
            && (draw.color.z == styles[1].color.z))
        {
            ++fillDraws;
        }
        else if ((draw.color.w == styles[1].shadowColor.w)
            && (draw.color.x == styles[1].shadowColor.x))
        {
            ++shadowLikeDraws;
        }
        else
        {
            ++outlineLikeDraws;
        }
    }

    HE_EXPECT_EQ(fillDraws, glyphCount);
    HE_EXPECT_EQ(shadowLikeDraws, glyphCount);
    HE_EXPECT_EQ(outlineLikeDraws, glyphCount * 8u);
}

HE_TEST(scribe, retained_text, expands_color_glyphs_into_layered_draws)
{
    static constexpr const char* ColorFontPath = "C:/Windows/Fonts/seguiemj.ttf";
    if (!File::Exists(ColorFontPath))
    {
        return;
    }

    Vector<uint8_t> fontBytes;
    HE_ASSERT(ReadFontFile(fontBytes, ColorFontPath));

    Vector<schema::Word> storage;
    FontFaceResourceReader font{};
    HE_ASSERT(BuildLoadedCompiledFontFace(storage, font, fontBytes, "Segoe UI Emoji"));

    ScribeContext context{};
    Vector<FontFaceHandle> handles{};
    HE_ASSERT(RegisterFontFaces(context, Span<const FontFaceResourceReader>(&font, 1), handles));
    LayoutEngine engine(context);
    LayoutResult layout;
    LayoutOptions options{};
    options.fontSize = 44.0f;
    options.wrap = false;
    HE_ASSERT(engine.LayoutText(layout, Span<const FontFaceHandle>(handles.Data(), handles.Size()), "🙂😀🎨", options));

    RetainedTextModel retainedText;
    RetainedTextBuildDesc desc{};
    desc.context = &context;
    desc.fontFaces = Span<const FontFaceHandle>(handles.Data(), handles.Size());
    desc.layout = &layout;
    desc.fontSize = options.fontSize;
    HE_ASSERT(retainedText.Build(desc));

    const uint32_t paletteIndex = SelectCompiledFontPalette(font, true);
    Vector<CompiledColorGlyphLayer> layers{};
    uint32_t expectedDrawCount = 0;
    for (const ShapedGlyph& glyph : layout.glyphs)
    {
        HE_ASSERT(GetCompiledColorGlyphLayers(layers, font, glyph.glyphIndex, paletteIndex));
        expectedDrawCount += layers.IsEmpty() ? 1u : layers.Size();
    }

    bool foundPaletteLayer = false;
    HE_EXPECT_EQ(retainedText.GetDrawCount(), expectedDrawCount);
    HE_EXPECT_EQ(retainedText.GetEstimatedVertexCount(), expectedDrawCount * ScribeGlyphVertexCount);
    for (const RetainedTextDraw& draw : retainedText.GetDraws())
    {
        if ((draw.flags & RetainedTextDrawFlagUseForegroundColor) == 0)
        {
            foundPaletteLayer = true;
            break;
        }
    }

    HE_EXPECT(foundPaletteLayer);
}

HE_TEST(scribe, retained_text, prepares_with_renderer_after_temporary_face_span_expires)
{
    String repoFontPath;
    HE_ASSERT(ResolveRepoFontPath(repoFontPath, "NotoSans-Regular.ttf"));

    Vector<uint8_t> fontBytes;
    HE_ASSERT(ReadFontFile(fontBytes, repoFontPath.Data()));

    Vector<schema::Word> storage;
    FontFaceResourceReader font{};
    HE_ASSERT(BuildLoadedCompiledFontFace(storage, font, fontBytes, "Noto Sans"));
    ScribeContext context{};

    RetainedTextModel retainedText;
    HE_ASSERT(BuildRetainedTextFromTemporaryFaceCopy(
        retainedText,
        context,
        Span<const FontFaceResourceReader>(&font, 1),
        "Retained text",
        28.0f));

    HE_EXPECT_GT(retainedText.GetDrawCount(), 0u);
    for (const RetainedTextDraw& draw : retainedText.GetDraws())
    {
        HE_EXPECT(context.GetFontFace(retainedText.GetFontFaceHandle(draw.fontFaceIndex)).IsValid());
    }
    storage.Clear();

    NullRendererHarness harness;
    HE_ASSERT(harness.Initialize());
    HE_EXPECT(harness.renderer.PrepareRetainedText(retainedText));

    RetainedTextInstanceDesc instance{};
    harness.renderer.QueueRetainedText(retainedText, instance);
}

HE_TEST(scribe, retained_text, prepares_emoji_fallback_scene_after_temporary_face_span_expires)
{
    static constexpr const char* ColorFontPath = "C:/Windows/Fonts/seguiemj.ttf";
    if (!File::Exists(ColorFontPath))
    {
        return;
    }

    String repoFontPath;
    HE_ASSERT(ResolveRepoFontPath(repoFontPath, "NotoSans-Regular.ttf"));

    Vector<uint8_t> repoFontBytes;
    HE_ASSERT(ReadFontFile(repoFontBytes, repoFontPath.Data()));

    Vector<uint8_t> colorFontBytes;
    HE_ASSERT(ReadFontFile(colorFontBytes, ColorFontPath));

    Vector<schema::Word> repoStorage;
    Vector<schema::Word> colorStorage;
    FontFaceResourceReader repoFont{};
    FontFaceResourceReader colorFont{};
    HE_ASSERT(BuildLoadedCompiledFontFace(repoStorage, repoFont, repoFontBytes, "NotoSans-Regular.ttf"));
    HE_ASSERT(BuildLoadedCompiledFontFace(colorStorage, colorFont, colorFontBytes, "seguiemj.ttf"));

    const FontFaceResourceReader faces[] =
    {
        repoFont,
        colorFont,
    };
    ScribeContext context{};

    RetainedTextModel retainedText;
    HE_ASSERT(BuildRetainedTextFromTemporaryFaceCopy(
        retainedText,
        context,
        Span<const FontFaceResourceReader>(faces, HE_LENGTH_OF(faces)),
        "\xF0\x9F\x99\x82 \xF0\x9F\x98\x80 \xF0\x9F\x8E\xA8 \xF0\x9F\x8C\x88 \xE2\x9C\xA8",
        44.0f));

    bool sawFallbackFaceDraw = false;
    bool sawPaletteColorDraw = false;
    for (const RetainedTextDraw& draw : retainedText.GetDraws())
    {
        HE_EXPECT(context.GetFontFace(retainedText.GetFontFaceHandle(draw.fontFaceIndex)).IsValid());
        sawFallbackFaceDraw |= draw.fontFaceIndex == 1u;
        sawPaletteColorDraw |= (draw.flags & RetainedTextDrawFlagUseForegroundColor) == 0;
    }

    HE_EXPECT(sawFallbackFaceDraw);
    HE_EXPECT(sawPaletteColorDraw);
    repoStorage.Clear();
    colorStorage.Clear();

    NullRendererHarness harness;
    HE_ASSERT(harness.Initialize());
    HE_EXPECT(harness.renderer.PrepareRetainedText(retainedText));

    RetainedTextInstanceDesc instance{};
    harness.renderer.QueueRetainedText(retainedText, instance);
}
