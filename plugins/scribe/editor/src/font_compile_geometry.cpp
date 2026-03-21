// Copyright Chad Engler

#include "font_compile_geometry.h"

#include "he/core/log.h"
#include "he/core/math.h"
#include "he/core/result_fmt.h"
#include "he/core/utils.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <algorithm>

namespace he::scribe::editor
{
    namespace
    {
        constexpr uint32_t CurveTextureWidth = 4096;
        constexpr uint32_t MaxBandCount = 8;
        constexpr uint32_t MaxCubicSubdivisionDepth = 8;

        struct Point2
        {
            float x{ 0.0f };
            float y{ 0.0f };
        };

        struct CurveData
        {
            Point2 p1{};
            Point2 p2{};
            Point2 p3{};
            float minX{ 0.0f };
            float minY{ 0.0f };
            float maxX{ 0.0f };
            float maxY{ 0.0f };
            uint32_t curveTexelIndex{ 0 };
        };

        struct CurveRef
        {
            uint16_t x{ 0 };
            uint16_t y{ 0 };
            float sortKey{ 0.0f };
        };

        class FreeTypeLibrary final
        {
        public:
            ~FreeTypeLibrary() noexcept
            {
                if (m_library != nullptr)
                {
                    FT_Done_FreeType(m_library);
                }
            }

            bool Initialize()
            {
                const FT_Error err = FT_Init_FreeType(&m_library);
                if (err != 0)
                {
                    HE_LOG_ERROR(he_scribe,
                        HE_MSG("Failed to initialize FreeType for font compile."),
                        HE_KV(error, err));
                    return false;
                }

                return true;
            }

            FT_Library Get() const { return m_library; }

        private:
            FT_Library m_library{ nullptr };
        };

        class FreeTypeFace final
        {
        public:
            ~FreeTypeFace() noexcept
            {
                if (m_face != nullptr)
                {
                    FT_Done_Face(m_face);
                }
            }

            bool Load(FT_Library library, Span<const uint8_t> sourceBytes, uint32_t faceIndex)
            {
                const FT_Error err = FT_New_Memory_Face(
                    library,
                    reinterpret_cast<const FT_Byte*>(sourceBytes.Data()),
                    static_cast<FT_Long>(sourceBytes.Size()),
                    static_cast<FT_Long>(faceIndex),
                    &m_face);
                if (err != 0)
                {
                    HE_LOG_ERROR(he_scribe,
                        HE_MSG("Failed to load font face for glyph compilation."),
                        HE_KV(face_index, faceIndex),
                        HE_KV(error, err));
                    return false;
                }

                return true;
            }

            FT_Face Get() const { return m_face; }

        private:
            FT_Face m_face{ nullptr };
        };

        Point2 ToPoint(const FT_Vector& value)
        {
            Point2 point{};
            point.x = static_cast<float>(value.x);
            point.y = static_cast<float>(value.y);
            return point;
        }

        Point2 MidPoint(const Point2& a, const Point2& b)
        {
            Point2 point{};
            point.x = (a.x + b.x) * 0.5f;
            point.y = (a.y + b.y) * 0.5f;
            return point;
        }

        float DistanceToLineSq(const Point2& point, const Point2& a, const Point2& b)
        {
            const float dx = b.x - a.x;
            const float dy = b.y - a.y;
            const float lenSq = (dx * dx) + (dy * dy);
            if (lenSq <= 1.0e-8f)
            {
                const float px = point.x - a.x;
                const float py = point.y - a.y;
                return (px * px) + (py * py);
            }

            const float area = ((point.x - a.x) * dy) - ((point.y - a.y) * dx);
            return (area * area) / lenSq;
        }

        void AppendCurveTexels(Vector<PackedCurveTexel>& out, const CurveData& curve)
        {
            out.PushBack(PackCurveTexel(curve.p1.x, curve.p1.y, curve.p2.x, curve.p2.y));
            out.PushBack(PackCurveTexel(curve.p3.x, curve.p3.y, 0.0f, 0.0f));
        }

        class GlyphOutlineBuilder final
        {
        public:
            explicit GlyphOutlineBuilder(float cubicTolerance)
                : m_cubicToleranceSq(cubicTolerance * cubicTolerance)
            {
            }

            bool Build(Vector<CurveData>& out, const FT_Outline& outline)
            {
                out.Clear();
                m_curves = &out;
                m_hasCurrent = false;

                FT_Outline_Funcs funcs{};
                funcs.move_to = &MoveTo;
                funcs.line_to = &LineTo;
                funcs.conic_to = &ConicTo;
                funcs.cubic_to = &CubicTo;
                funcs.shift = 0;
                funcs.delta = 0;

                const FT_Error err = FT_Outline_Decompose(const_cast<FT_Outline*>(&outline), &funcs, this);
                m_curves = nullptr;
                m_hasCurrent = false;
                return err == 0;
            }

        private:
            static int MoveTo(const FT_Vector* to, void* user)
            {
                GlyphOutlineBuilder& self = *static_cast<GlyphOutlineBuilder*>(user);
                self.m_current = ToPoint(*to);
                self.m_hasCurrent = true;
                return 0;
            }

            static int LineTo(const FT_Vector* to, void* user)
            {
                GlyphOutlineBuilder& self = *static_cast<GlyphOutlineBuilder*>(user);
                const Point2 target = ToPoint(*to);
                self.AddLine(self.m_current, target);
                self.m_current = target;
                return 0;
            }

            static int ConicTo(const FT_Vector* control, const FT_Vector* to, void* user)
            {
                GlyphOutlineBuilder& self = *static_cast<GlyphOutlineBuilder*>(user);
                const Point2 p2 = ToPoint(*control);
                const Point2 p3 = ToPoint(*to);
                self.AddQuadratic(self.m_current, p2, p3);
                self.m_current = p3;
                return 0;
            }

            static int CubicTo(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user)
            {
                GlyphOutlineBuilder& self = *static_cast<GlyphOutlineBuilder*>(user);
                const Point2 p1 = ToPoint(*control1);
                const Point2 p2 = ToPoint(*control2);
                const Point2 p3 = ToPoint(*to);
                self.FlattenCubic(self.m_current, p1, p2, p3, 0);
                self.m_current = p3;
                return 0;
            }

            void AddLine(const Point2& from, const Point2& to)
            {
                AddQuadratic(from, MidPoint(from, to), to);
            }

            void AddQuadratic(const Point2& p1, const Point2& p2, const Point2& p3)
            {
                CurveData& curve = m_curves->EmplaceBack();
                curve.p1 = p1;
                curve.p2 = p2;
                curve.p3 = p3;
                curve.minX = Min(p1.x, p2.x, p3.x);
                curve.minY = Min(p1.y, p2.y, p3.y);
                curve.maxX = Max(p1.x, p2.x, p3.x);
                curve.maxY = Max(p1.y, p2.y, p3.y);
            }

            void FlattenCubic(const Point2& p0, const Point2& p1, const Point2& p2, const Point2& p3, uint32_t depth)
            {
                const float d1 = DistanceToLineSq(p1, p0, p3);
                const float d2 = DistanceToLineSq(p2, p0, p3);
                if ((depth >= MaxCubicSubdivisionDepth) || (Max(d1, d2) <= m_cubicToleranceSq))
                {
                    AddLine(p0, p3);
                    return;
                }

                const Point2 p01 = MidPoint(p0, p1);
                const Point2 p12 = MidPoint(p1, p2);
                const Point2 p23 = MidPoint(p2, p3);
                const Point2 p012 = MidPoint(p01, p12);
                const Point2 p123 = MidPoint(p12, p23);
                const Point2 p0123 = MidPoint(p012, p123);

                FlattenCubic(p0, p01, p012, p0123, depth + 1);
                FlattenCubic(p0123, p123, p23, p3, depth + 1);
            }

        private:
            Vector<CurveData>* m_curves{ nullptr };
            Point2 m_current{};
            float m_cubicToleranceSq{ 0.0f };
            bool m_hasCurrent{ false };
        };

        void ZeroCounts(Vector<uint32_t>& counts)
        {
            for (uint32_t i = 0; i < counts.Size(); ++i)
            {
                counts[i] = 0;
            }
        }

        void GetBandRange(
            int32_t& outStart,
            int32_t& outEnd,
            float curveMin,
            float curveMax,
            float boundsMin,
            float boundsSpan,
            uint32_t bandCount,
            float epsilon)
        {
            if ((bandCount <= 1) || (boundsSpan <= epsilon))
            {
                outStart = 0;
                outEnd = 0;
                return;
            }

            const float bandScale = static_cast<float>(bandCount) / boundsSpan;
            const float startBand = Floor(((curveMin - epsilon) - boundsMin) * bandScale);
            const float endBand = Floor(((curveMax + epsilon) - boundsMin) * bandScale);
            outStart = Clamp(static_cast<int32_t>(startBand), 0, static_cast<int32_t>(bandCount - 1));
            outEnd = Clamp(static_cast<int32_t>(endBand), 0, static_cast<int32_t>(bandCount - 1));
        }

        uint32_t ChooseBandCount(
            const Vector<CurveData>& curves,
            bool horizontalBands,
            float boundsMin,
            float boundsMax,
            float epsilon)
        {
            const float span = boundsMax - boundsMin;
            if (curves.IsEmpty() || (span <= epsilon))
            {
                return 1;
            }

            uint32_t bestBandCount = 1;
            uint32_t bestMaxOccupancy = curves.Size();
            Vector<uint32_t> counts{};

            for (uint32_t bandCount = 1; bandCount <= MaxBandCount; ++bandCount)
            {
                counts.Resize(bandCount);
                ZeroCounts(counts);

                for (uint32_t curveIndex = 0; curveIndex < curves.Size(); ++curveIndex)
                {
                    const CurveData& curve = curves[curveIndex];
                    int32_t startBand = 0;
                    int32_t endBand = 0;
                    GetBandRange(
                        startBand,
                        endBand,
                        horizontalBands ? curve.minY : curve.minX,
                        horizontalBands ? curve.maxY : curve.maxX,
                        boundsMin,
                        span,
                        bandCount,
                        epsilon);

                    for (int32_t band = startBand; band <= endBand; ++band)
                    {
                        counts[static_cast<uint32_t>(band)] += 1;
                    }
                }

                uint32_t maxOccupancy = 0;
                for (uint32_t i = 0; i < counts.Size(); ++i)
                {
                    maxOccupancy = Max(maxOccupancy, counts[i]);
                }

                if ((maxOccupancy < bestMaxOccupancy)
                    || ((maxOccupancy == bestMaxOccupancy) && (bandCount < bestBandCount)))
                {
                    bestBandCount = bandCount;
                    bestMaxOccupancy = maxOccupancy;
                }
            }

            return bestBandCount;
        }

        void BuildBandRefs(
            Vector<Vector<CurveRef>>& outBands,
            const Vector<CurveData>& curves,
            bool horizontalBands,
            float boundsMin,
            float boundsMax,
            uint32_t bandCount,
            float epsilon)
        {
            outBands.Clear();
            outBands.Resize(bandCount);

            const float span = boundsMax - boundsMin;
            for (uint32_t curveIndex = 0; curveIndex < curves.Size(); ++curveIndex)
            {
                const CurveData& curve = curves[curveIndex];
                int32_t startBand = 0;
                int32_t endBand = 0;
                GetBandRange(
                    startBand,
                    endBand,
                    horizontalBands ? curve.minY : curve.minX,
                    horizontalBands ? curve.maxY : curve.maxX,
                    boundsMin,
                    span,
                    bandCount,
                    epsilon);

                CurveRef ref{};
                ref.x = static_cast<uint16_t>(curve.curveTexelIndex & (CurveTextureWidth - 1));
                ref.y = static_cast<uint16_t>(curve.curveTexelIndex / CurveTextureWidth);
                ref.sortKey = horizontalBands ? curve.maxX : curve.maxY;

                for (int32_t band = startBand; band <= endBand; ++band)
                {
                    outBands[static_cast<uint32_t>(band)].PushBack(ref);
                }
            }

            for (uint32_t bandIndex = 0; bandIndex < outBands.Size(); ++bandIndex)
            {
                Vector<CurveRef>& band = outBands[bandIndex];
                if (band.Size() > 1)
                {
                    std::sort(band.Data(), band.Data() + band.Size(), [](const CurveRef& a, const CurveRef& b)
                    {
                        return a.sortKey > b.sortKey;
                    });
                }
            }
        }

        float ComputeBandScale(float minBound, float maxBound, uint32_t bandCount)
        {
            if (bandCount <= 1)
            {
                return 0.0f;
            }

            const float span = maxBound - minBound;
            if (span <= 0.0f)
            {
                return 0.0f;
            }

            return static_cast<float>(bandCount) / span;
        }

        void PadCurveTexture(Vector<PackedCurveTexel>& texels, uint32_t width, uint32_t& outHeight)
        {
            const uint32_t texelCount = Max(texels.Size(), 1u);
            outHeight = (texelCount + (width - 1)) / width;
            texels.Resize(width * outHeight);
        }

        void PadBandTexture(Vector<PackedBandTexel>& texels, uint32_t width, uint32_t& outHeight)
        {
            const uint32_t texelCount = Max(texels.Size(), width);
            outHeight = (texelCount + (width - 1)) / width;
            texels.Resize(width * outHeight);
        }
    }

    bool BuildCompiledFontRenderData(CompiledFontRenderData& out, Span<const uint8_t> sourceBytes, uint32_t faceIndex)
    {
        out = {};
        out.bandTextureWidth = ScribeBandTextureWidth;

        FreeTypeLibrary library;
        if (!library.Initialize())
        {
            return false;
        }

        FreeTypeFace face;
        if (!face.Load(library.Get(), sourceBytes, faceIndex))
        {
            return false;
        }

        FT_Face ftFace = face.Get();
        const uint32_t glyphCount = ftFace->num_glyphs > 0 ? static_cast<uint32_t>(ftFace->num_glyphs) : 0;
        out.glyphs.Resize(glyphCount);
        out.bandOverlapEpsilon = Max(1.0f, static_cast<float>(ftFace->units_per_EM) / 1024.0f);

        for (uint32_t glyphIndex = 0; glyphIndex < glyphCount; ++glyphIndex)
        {
            const FT_Error loadErr = FT_Load_Glyph(
                ftFace,
                static_cast<FT_UInt>(glyphIndex),
                FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING | FT_LOAD_IGNORE_TRANSFORM);
            if (loadErr != 0)
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Failed to load glyph outline for scribe font compile."),
                    HE_KV(face_index, faceIndex),
                    HE_KV(glyph_index, glyphIndex),
                    HE_KV(error, loadErr));
                return false;
            }

            CompiledGlyphRenderEntry& glyphEntry = out.glyphs[glyphIndex];
            glyphEntry.advanceX = static_cast<int32_t>(ftFace->glyph->advance.x);
            glyphEntry.advanceY = static_cast<int32_t>(ftFace->glyph->advance.y);
            glyphEntry.fillRule = FillRule::NonZero;

            if ((ftFace->glyph->format != FT_GLYPH_FORMAT_OUTLINE) || (ftFace->glyph->outline.n_points == 0))
            {
                continue;
            }

            GlyphOutlineBuilder outlineBuilder(out.bandOverlapEpsilon);
            Vector<CurveData> curves{};
            if (!outlineBuilder.Build(curves, ftFace->glyph->outline))
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Failed to decompose glyph outline for scribe font compile."),
                    HE_KV(face_index, faceIndex),
                    HE_KV(glyph_index, glyphIndex));
                return false;
            }

            if (curves.IsEmpty())
            {
                continue;
            }

            glyphEntry.flags |= CompiledFontGlyphFlagHasGeometry;
            glyphEntry.boundsMinX = curves[0].minX;
            glyphEntry.boundsMinY = curves[0].minY;
            glyphEntry.boundsMaxX = curves[0].maxX;
            glyphEntry.boundsMaxY = curves[0].maxY;

            for (uint32_t curveIndex = 0; curveIndex < curves.Size(); ++curveIndex)
            {
                CurveData& curve = curves[curveIndex];
                curve.curveTexelIndex = out.curveTexels.Size();
                AppendCurveTexels(out.curveTexels, curve);
                glyphEntry.boundsMinX = Min(glyphEntry.boundsMinX, curve.minX);
                glyphEntry.boundsMinY = Min(glyphEntry.boundsMinY, curve.minY);
                glyphEntry.boundsMaxX = Max(glyphEntry.boundsMaxX, curve.maxX);
                glyphEntry.boundsMaxY = Max(glyphEntry.boundsMaxY, curve.maxY);
            }

            const uint32_t verticalBandCount = ChooseBandCount(
                curves,
                false,
                glyphEntry.boundsMinX,
                glyphEntry.boundsMaxX,
                out.bandOverlapEpsilon);
            const uint32_t horizontalBandCount = ChooseBandCount(
                curves,
                true,
                glyphEntry.boundsMinY,
                glyphEntry.boundsMaxY,
                out.bandOverlapEpsilon);

            glyphEntry.bandMaxX = verticalBandCount - 1;
            glyphEntry.bandMaxY = horizontalBandCount - 1;
            glyphEntry.bandScaleX = ComputeBandScale(glyphEntry.boundsMinX, glyphEntry.boundsMaxX, verticalBandCount);
            glyphEntry.bandScaleY = ComputeBandScale(glyphEntry.boundsMinY, glyphEntry.boundsMaxY, horizontalBandCount);
            glyphEntry.bandOffsetX = -glyphEntry.boundsMinX * glyphEntry.bandScaleX;
            glyphEntry.bandOffsetY = -glyphEntry.boundsMinY * glyphEntry.bandScaleY;

            Vector<Vector<CurveRef>> horizontalBands{};
            Vector<Vector<CurveRef>> verticalBands{};
            BuildBandRefs(
                horizontalBands,
                curves,
                true,
                glyphEntry.boundsMinY,
                glyphEntry.boundsMaxY,
                horizontalBandCount,
                out.bandOverlapEpsilon);
            BuildBandRefs(
                verticalBands,
                curves,
                false,
                glyphEntry.boundsMinX,
                glyphEntry.boundsMaxX,
                verticalBandCount,
                out.bandOverlapEpsilon);

            const uint32_t glyphBandStart = out.bandTexels.Size();
            glyphEntry.glyphBandLocX = glyphBandStart & (ScribeBandTextureWidth - 1);
            glyphEntry.glyphBandLocY = glyphBandStart / ScribeBandTextureWidth;

            const uint32_t headerCount = horizontalBandCount + verticalBandCount;
            out.bandTexels.Resize(glyphBandStart + headerCount);

            uint32_t currentOffset = headerCount;
            for (uint32_t bandIndex = 0; bandIndex < horizontalBands.Size(); ++bandIndex)
            {
                PackedBandTexel header{};
                header.x = static_cast<uint16_t>(horizontalBands[bandIndex].Size());
                header.y = static_cast<uint16_t>(currentOffset);
                out.bandTexels[glyphBandStart + bandIndex] = header;

                for (uint32_t curveIndex = 0; curveIndex < horizontalBands[bandIndex].Size(); ++curveIndex)
                {
                    PackedBandTexel texel{};
                    texel.x = horizontalBands[bandIndex][curveIndex].x;
                    texel.y = horizontalBands[bandIndex][curveIndex].y;
                    out.bandTexels.PushBack(texel);
                    currentOffset += 1;
                }
            }

            for (uint32_t bandIndex = 0; bandIndex < verticalBands.Size(); ++bandIndex)
            {
                PackedBandTexel header{};
                header.x = static_cast<uint16_t>(verticalBands[bandIndex].Size());
                header.y = static_cast<uint16_t>(currentOffset);
                out.bandTexels[glyphBandStart + horizontalBandCount + bandIndex] = header;

                for (uint32_t curveIndex = 0; curveIndex < verticalBands[bandIndex].Size(); ++curveIndex)
                {
                    PackedBandTexel texel{};
                    texel.x = verticalBands[bandIndex][curveIndex].x;
                    texel.y = verticalBands[bandIndex][curveIndex].y;
                    out.bandTexels.PushBack(texel);
                    currentOffset += 1;
                }
            }
        }

        out.curveTextureWidth = CurveTextureWidth;
        PadCurveTexture(out.curveTexels, out.curveTextureWidth, out.curveTextureHeight);
        PadBandTexture(out.bandTexels, out.bandTextureWidth, out.bandTextureHeight);
        return true;
    }
}
