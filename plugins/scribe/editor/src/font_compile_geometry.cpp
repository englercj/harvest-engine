// Copyright Chad Engler

#include "font_compile_geometry.h"

#include "curve_compile_utils.h"
#include "outline_build_utils.h"
#include "packed_geometry_compile_utils.h"

#include "he/core/log.h"
#include "he/core/math.h"
#include "he/core/result_fmt.h"
#include "he/core/utils.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_COLOR_H
#include FT_OUTLINE_H

#include <algorithm>

namespace he::scribe::editor
{
    namespace
    {
        using curve_compile::Affine2D;
        using curve_compile::CurveData;
        using curve_compile::Point2;

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

        float Fixed16Dot16ToFloat(FT_Fixed value)
        {
            return static_cast<float>(value) / 65536.0f;
        }

        Affine2D ComposeAffine(const Affine2D& outer, const Affine2D& inner)
        {
            Affine2D result{};
            result.m00 = (outer.m00 * inner.m00) + (outer.m01 * inner.m10);
            result.m01 = (outer.m00 * inner.m01) + (outer.m01 * inner.m11);
            result.m10 = (outer.m10 * inner.m00) + (outer.m11 * inner.m10);
            result.m11 = (outer.m10 * inner.m01) + (outer.m11 * inner.m11);
            result.tx = (outer.m00 * inner.tx) + (outer.m01 * inner.ty) + outer.tx;
            result.ty = (outer.m10 * inner.tx) + (outer.m11 * inner.ty) + outer.ty;
            return result;
        }

        Affine2D MakeObjectSpaceAffine(float a, float b, float c, float d, float tx, float ty)
        {
            Affine2D result{};
            result.m00 = a;
            result.m01 = -b;
            result.m10 = -c;
            result.m11 = d;
            result.tx = tx;
            result.ty = -ty;
            return result;
        }

        Affine2D MakeTranslateAffine(FT_Fixed dx, FT_Fixed dy)
        {
            return MakeObjectSpaceAffine(
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                Fixed16Dot16ToFloat(dx),
                Fixed16Dot16ToFloat(dy));
        }

        Affine2D MakeScaleAffine(FT_Fixed scaleX, FT_Fixed scaleY, FT_Fixed centerX, FT_Fixed centerY)
        {
            const float sx = Fixed16Dot16ToFloat(scaleX);
            const float sy = Fixed16Dot16ToFloat(scaleY);
            const float cx = Fixed16Dot16ToFloat(centerX);
            const float cy = Fixed16Dot16ToFloat(centerY);
            return MakeObjectSpaceAffine(
                sx,
                0.0f,
                0.0f,
                sy,
                cx - (sx * cx),
                cy - (sy * cy));
        }

        Affine2D MakeRotateAffine(FT_Fixed angle, FT_Fixed centerX, FT_Fixed centerY)
        {
            const float radians = Fixed16Dot16ToFloat(angle) * 3.14159265358979323846f;
            float s = 0.0f;
            float c = 1.0f;
            SinCos(radians, s, c);

            const float cx = Fixed16Dot16ToFloat(centerX);
            const float cy = Fixed16Dot16ToFloat(centerY);
            const float tx = cx - ((c * cx) - (s * cy));
            const float ty = cy - ((s * cx) + (c * cy));
            return MakeObjectSpaceAffine(c, -s, s, c, tx, ty);
        }

        Affine2D MakeSkewAffine(FT_Fixed xSkewAngle, FT_Fixed ySkewAngle, FT_Fixed centerX, FT_Fixed centerY)
        {
            const float kx = Tan(Fixed16Dot16ToFloat(xSkewAngle) * 3.14159265358979323846f);
            const float ky = Tan(Fixed16Dot16ToFloat(ySkewAngle) * 3.14159265358979323846f);
            const float cx = Fixed16Dot16ToFloat(centerX);
            const float cy = Fixed16Dot16ToFloat(centerY);
            const float tx = cx - (cx + (kx * cy));
            const float ty = cy - ((ky * cx) + cy);
            return MakeObjectSpaceAffine(1.0f, kx, ky, 1.0f, tx, ty);
        }

        Affine2D MakeTransformAffine(const FT_Affine23& affine)
        {
            return MakeObjectSpaceAffine(
                Fixed16Dot16ToFloat(affine.xx),
                Fixed16Dot16ToFloat(affine.xy),
                Fixed16Dot16ToFloat(affine.yx),
                Fixed16Dot16ToFloat(affine.yy),
                Fixed16Dot16ToFloat(affine.dx),
                Fixed16Dot16ToFloat(affine.dy));
        }

        class GlyphOutlineBuilder final
        {
        public:
            explicit GlyphOutlineBuilder(float cubicTolerance)
                : m_builder(cubicTolerance)
            {
            }

            bool Build(
                Vector<CurveData>& outCurves,
                Vector<StrokeSourcePoint>& outPoints,
                Vector<StrokeSourceCommand>& outCommands,
                const FT_Outline& outline)
            {
                outCurves.Clear();
                outPoints.Clear();
                outCommands.Clear();
                m_builder.Clear();

                FT_Outline_Funcs funcs{};
                funcs.move_to = &MoveTo;
                funcs.line_to = &LineTo;
                funcs.conic_to = &ConicTo;
                funcs.cubic_to = &CubicTo;
                funcs.shift = 0;
                funcs.delta = 0;

                const FT_Error err = FT_Outline_Decompose(const_cast<FT_Outline*>(&outline), &funcs, this);
                if (m_builder.HasOpenSubpath())
                {
                    m_builder.CloseSubpath(false);
                }

                outCurves = Move(m_builder.Curves());
                outPoints = Move(m_builder.Points());
                outCommands = Move(m_builder.Commands());
                return err == 0;
            }

        private:
            static int MoveTo(const FT_Vector* to, void* user)
            {
                GlyphOutlineBuilder& self = *static_cast<GlyphOutlineBuilder*>(user);
                if (self.m_builder.HasOpenSubpath())
                {
                    self.m_builder.CloseSubpath(false);
                }

                self.m_builder.BeginSubpath(ToPoint(*to));
                return 0;
            }

            static int LineTo(const FT_Vector* to, void* user)
            {
                GlyphOutlineBuilder& self = *static_cast<GlyphOutlineBuilder*>(user);
                self.m_builder.LineTo(ToPoint(*to));
                return 0;
            }

            static int ConicTo(const FT_Vector* control, const FT_Vector* to, void* user)
            {
                GlyphOutlineBuilder& self = *static_cast<GlyphOutlineBuilder*>(user);
                self.m_builder.QuadraticTo(ToPoint(*control), ToPoint(*to));
                return 0;
            }

            static int CubicTo(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user)
            {
                GlyphOutlineBuilder& self = *static_cast<GlyphOutlineBuilder*>(user);
                self.m_builder.CubicTo(ToPoint(*control1), ToPoint(*control2), ToPoint(*to));
                return 0;
            }

        private:
            OutlineBuilder m_builder;
        };

        class GlyphStrokeCountBuilder final
        {
        public:
            explicit GlyphStrokeCountBuilder([[maybe_unused]] float cubicTolerance)
            {
            }

            bool Build(const FT_Outline& outline)
            {
                m_pointCount = 0;
                m_commandCount = 0;
                m_hasContour = false;
                m_current = {};

                FT_Outline_Funcs funcs{};
                funcs.move_to = &MoveTo;
                funcs.line_to = &LineTo;
                funcs.conic_to = &ConicTo;
                funcs.cubic_to = &CubicTo;
                funcs.shift = 0;
                funcs.delta = 0;

                const FT_Error err = FT_Outline_Decompose(const_cast<FT_Outline*>(&outline), &funcs, this);
                if (err != 0)
                {
                    return false;
                }

                if (m_hasContour)
                {
                    AppendClose();
                }

                return true;
            }

            uint32_t GetPointCount() const { return m_pointCount; }
            uint32_t GetCommandCount() const { return m_commandCount; }

        private:
            void AppendMoveTo(const Point2& point)
            {
                ++m_commandCount;
                ++m_pointCount;
                m_current = point;
                m_hasContour = true;
            }

            void AppendLineTo(const Point2& point)
            {
                ++m_commandCount;
                ++m_pointCount;
                m_current = point;
            }

            void AppendQuadraticTo(const Point2& point)
            {
                ++m_commandCount;
                m_pointCount += 2;
                m_current = point;
            }

            void AppendCubicTo(const Point2& point)
            {
                ++m_commandCount;
                m_pointCount += 3;
                m_current = point;
            }

            void AppendClose()
            {
                ++m_commandCount;
                m_hasContour = false;
            }

            static int MoveTo(const FT_Vector* to, void* user)
            {
                GlyphStrokeCountBuilder& self = *static_cast<GlyphStrokeCountBuilder*>(user);
                if (self.m_hasContour)
                {
                    self.AppendClose();
                }

                self.AppendMoveTo(ToPoint(*to));
                return 0;
            }

            static int LineTo(const FT_Vector* to, void* user)
            {
                GlyphStrokeCountBuilder& self = *static_cast<GlyphStrokeCountBuilder*>(user);
                self.AppendLineTo(ToPoint(*to));
                return 0;
            }

            static int ConicTo(const FT_Vector* control, const FT_Vector* to, void* user)
            {
                GlyphStrokeCountBuilder& self = *static_cast<GlyphStrokeCountBuilder*>(user);
                (void)control;
                self.AppendQuadraticTo(ToPoint(*to));
                return 0;
            }

            static int CubicTo(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user)
            {
                GlyphStrokeCountBuilder& self = *static_cast<GlyphStrokeCountBuilder*>(user);
                (void)control1;
                (void)control2;
                self.AppendCubicTo(ToPoint(*to));
                return 0;
            }

        private:
            Point2 m_current{};
            uint32_t m_pointCount{ 0 };
            uint32_t m_commandCount{ 0 };
            bool m_hasContour{ false };
        };

        bool EstimateCompiledStrokeStorage(
            uint32_t& outPointCount,
            uint32_t& outCommandCount,
            FT_Face face,
            uint32_t glyphCount,
            float cubicTolerance)
        {
            outPointCount = 0;
            outCommandCount = 0;

            GlyphStrokeCountBuilder counter(cubicTolerance);
            for (uint32_t glyphIndex = 0; glyphIndex < glyphCount; ++glyphIndex)
            {
                const FT_Error loadErr = FT_Load_Glyph(
                    face,
                    static_cast<FT_UInt>(glyphIndex),
                    FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING | FT_LOAD_IGNORE_TRANSFORM);
                if (loadErr != 0)
                {
                    HE_LOG_ERROR(he_scribe,
                        HE_MSG("Failed to load glyph outline for stroke storage estimation."),
                        HE_KV(glyph_index, glyphIndex),
                        HE_KV(error, loadErr));
                    return false;
                }

                if ((face->glyph->format != FT_GLYPH_FORMAT_OUTLINE) || (face->glyph->outline.n_points == 0))
                {
                    continue;
                }

                if (!counter.Build(face->glyph->outline))
                {
                    HE_LOG_ERROR(he_scribe,
                        HE_MSG("Failed to estimate glyph stroke storage."),
                        HE_KV(glyph_index, glyphIndex));
                    return false;
                }

                outPointCount += counter.GetPointCount();
                outCommandCount += counter.GetCommandCount();
            }

            return true;
        }

        CompiledFontPaletteColor ToPaletteColor(const FT_Color& color)
        {
            CompiledFontPaletteColor result{};
            result.red = static_cast<float>(color.red) / 255.0f;
            result.green = static_cast<float>(color.green) / 255.0f;
            result.blue = static_cast<float>(color.blue) / 255.0f;
            result.alpha = static_cast<float>(color.alpha) / 255.0f;
            return result;
        }

        float ColorAlphaToFloat(FT_F2Dot14 alpha)
        {
            return Clamp(static_cast<float>(alpha) / 16384.0f, 0.0f, 1.0f);
        }

        struct PaintColorInfo
        {
            uint32_t paletteEntryIndex{ 0 };
            FontFaceColorSource colorSource{ FontFaceColorSource::Palette };
            float alphaScale{ 1.0f };
        };

        bool TryResolveColorIndexInfo(PaintColorInfo& out, const FT_ColorIndex& colorIndex)
        {
            out.paletteEntryIndex = colorIndex.palette_index;
            out.colorSource = FontFaceColorSource::Palette;
            out.alphaScale = ColorAlphaToFloat(colorIndex.alpha);
            if (out.paletteEntryIndex == 0xFFFFu)
            {
                out.paletteEntryIndex = 0;
                out.colorSource = FontFaceColorSource::Foreground;
            }

            return true;
        }

        bool TryResolveGradientColorInfo(PaintColorInfo& out, FT_Face face, const FT_ColorLine& colorLine)
        {
            FT_ColorStopIterator iterator = colorLine.color_stop_iterator;
            FT_ColorStop bestStop{};
            bool hasBestStop = false;
            float bestDistance = 0.0f;

            FT_ColorStop stop{};
            while (FT_Get_Colorline_Stops(face, &stop, &iterator))
            {
                const float stopPosition = static_cast<float>(stop.stop_offset) / 65536.0f;
                const float distance = Abs(stopPosition - 0.5f);
                if (!hasBestStop || (distance < bestDistance))
                {
                    bestStop = stop;
                    hasBestStop = true;
                    bestDistance = distance;
                }
            }

            if (!hasBestStop)
            {
                return false;
            }

            return TryResolveColorIndexInfo(out, bestStop.color);
        }

        bool TryResolveV1PaintColorInfo(PaintColorInfo& out, FT_Face face, FT_OpaquePaint opaquePaint, uint32_t depth);

        bool TryAppendV1LayersFromPaint(
            Vector<CompiledColorGlyphLayerEntry>& out,
            FT_Face face,
            FT_OpaquePaint opaquePaint,
            const Affine2D& transform,
            uint32_t depth)
        {
            if (depth >= 64)
            {
                return false;
            }

            FT_COLR_Paint paint{};
            if (!FT_Get_Paint(face, opaquePaint, &paint))
            {
                return false;
            }

            switch (paint.format)
            {
                case FT_COLR_PAINTFORMAT_COLR_LAYERS:
                {
                    FT_LayerIterator iterator = paint.u.colr_layers.layer_iterator;
                    FT_OpaquePaint layerPaint{};
                    while (FT_Get_Paint_Layers(face, &iterator, &layerPaint))
                    {
                        if (!TryAppendV1LayersFromPaint(out, face, layerPaint, transform, depth + 1))
                        {
                            return false;
                        }
                    }

                    return true;
                }

                case FT_COLR_PAINTFORMAT_GLYPH:
                {
                    PaintColorInfo colorInfo{};
                    if (!TryResolveV1PaintColorInfo(colorInfo, face, paint.u.glyph.paint, depth + 1))
                    {
                        return false;
                    }

                    CompiledColorGlyphLayerEntry& layer = out.EmplaceBack();
                    layer.glyphIndex = paint.u.glyph.glyphID;
                    layer.paletteEntryIndex = colorInfo.paletteEntryIndex;
                    layer.colorSource = colorInfo.colorSource;
                    layer.alphaScale = colorInfo.alphaScale;
                    layer.transform00 = transform.m00;
                    layer.transform01 = transform.m01;
                    layer.transform10 = transform.m10;
                    layer.transform11 = transform.m11;
                    layer.transformTx = transform.tx;
                    layer.transformTy = transform.ty;
                    return true;
                }

                case FT_COLR_PAINTFORMAT_COLR_GLYPH:
                {
                    FT_OpaquePaint nestedPaint{};
                    if (!FT_Get_Color_Glyph_Paint(
                        face,
                        paint.u.colr_glyph.glyphID,
                        FT_COLOR_NO_ROOT_TRANSFORM,
                        &nestedPaint))
                    {
                        return false;
                    }

                    return TryAppendV1LayersFromPaint(out, face, nestedPaint, transform, depth + 1);
                }

                case FT_COLR_PAINTFORMAT_TRANSFORM:
                    return TryAppendV1LayersFromPaint(
                        out,
                        face,
                        paint.u.transform.paint,
                        ComposeAffine(transform, MakeTransformAffine(paint.u.transform.affine)),
                        depth + 1);

                case FT_COLR_PAINTFORMAT_TRANSLATE:
                    return TryAppendV1LayersFromPaint(
                        out,
                        face,
                        paint.u.translate.paint,
                        ComposeAffine(transform, MakeTranslateAffine(paint.u.translate.dx, paint.u.translate.dy)),
                        depth + 1);

                case FT_COLR_PAINTFORMAT_SCALE:
                    return TryAppendV1LayersFromPaint(
                        out,
                        face,
                        paint.u.scale.paint,
                        ComposeAffine(
                            transform,
                            MakeScaleAffine(
                                paint.u.scale.scale_x,
                                paint.u.scale.scale_y,
                                paint.u.scale.center_x,
                                paint.u.scale.center_y)),
                        depth + 1);

                case FT_COLR_PAINTFORMAT_ROTATE:
                    return TryAppendV1LayersFromPaint(
                        out,
                        face,
                        paint.u.rotate.paint,
                        ComposeAffine(
                            transform,
                            MakeRotateAffine(
                                paint.u.rotate.angle,
                                paint.u.rotate.center_x,
                                paint.u.rotate.center_y)),
                        depth + 1);

                case FT_COLR_PAINTFORMAT_SKEW:
                    return TryAppendV1LayersFromPaint(
                        out,
                        face,
                        paint.u.skew.paint,
                        ComposeAffine(
                            transform,
                            MakeSkewAffine(
                                paint.u.skew.x_skew_angle,
                                paint.u.skew.y_skew_angle,
                                paint.u.skew.center_x,
                                paint.u.skew.center_y)),
                        depth + 1);

                case FT_COLR_PAINTFORMAT_COMPOSITE:
                    return TryAppendV1LayersFromPaint(out, face, paint.u.composite.backdrop_paint, transform, depth + 1)
                        && TryAppendV1LayersFromPaint(out, face, paint.u.composite.source_paint, transform, depth + 1);

                default:
                    return false;
            }
        }

        bool TryResolveV1PaintColorInfo(PaintColorInfo& out, FT_Face face, FT_OpaquePaint opaquePaint, uint32_t depth)
        {
            if (depth >= 64)
            {
                return false;
            }

            FT_COLR_Paint paint{};
            if (!FT_Get_Paint(face, opaquePaint, &paint))
            {
                return false;
            }

            switch (paint.format)
            {
                case FT_COLR_PAINTFORMAT_SOLID:
                    return TryResolveColorIndexInfo(out, paint.u.solid.color);

                case FT_COLR_PAINTFORMAT_LINEAR_GRADIENT:
                    return TryResolveGradientColorInfo(out, face, paint.u.linear_gradient.colorline);

                case FT_COLR_PAINTFORMAT_RADIAL_GRADIENT:
                    return TryResolveGradientColorInfo(out, face, paint.u.radial_gradient.colorline);

                case FT_COLR_PAINTFORMAT_SWEEP_GRADIENT:
                    return TryResolveGradientColorInfo(out, face, paint.u.sweep_gradient.colorline);

                case FT_COLR_PAINTFORMAT_TRANSFORM:
                    return TryResolveV1PaintColorInfo(out, face, paint.u.transform.paint, depth + 1);

                case FT_COLR_PAINTFORMAT_TRANSLATE:
                    return TryResolveV1PaintColorInfo(out, face, paint.u.translate.paint, depth + 1);

                case FT_COLR_PAINTFORMAT_SCALE:
                    return TryResolveV1PaintColorInfo(out, face, paint.u.scale.paint, depth + 1);

                case FT_COLR_PAINTFORMAT_ROTATE:
                    return TryResolveV1PaintColorInfo(out, face, paint.u.rotate.paint, depth + 1);

                case FT_COLR_PAINTFORMAT_SKEW:
                    return TryResolveV1PaintColorInfo(out, face, paint.u.skew.paint, depth + 1);

                default:
                    return false;
            }
        }

        bool BuildCompiledFontPaintData(
            CompiledFontPaintData& out,
            Vector<CompiledGlyphRenderEntry>& glyphs,
            FT_Face face,
            uint32_t glyphCount)
        {
            out = {};
            out.colorGlyphs.Resize(glyphCount);

            if (!FT_HAS_COLOR(face))
            {
                return true;
            }

            FT_Palette_Data paletteData{};
            const FT_Error paletteDataErr = FT_Palette_Data_Get(face, &paletteData);
            if (paletteDataErr != 0)
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Failed to read CPAL palette data for scribe font compile."),
                    HE_KV(error, paletteDataErr));
                return false;
            }

            if ((paletteData.num_palettes == 0) || (paletteData.num_palette_entries == 0))
            {
                return true;
            }

            out.palettes.Resize(paletteData.num_palettes);
            for (uint32_t paletteIndex = 0; paletteIndex < paletteData.num_palettes; ++paletteIndex)
            {
                FT_Color* paletteEntries = nullptr;
                const FT_Error paletteSelectErr = FT_Palette_Select(
                    face,
                    static_cast<FT_UShort>(paletteIndex),
                    &paletteEntries);
                if ((paletteSelectErr != 0) || (paletteEntries == nullptr))
                {
                    HE_LOG_ERROR(he_scribe,
                        HE_MSG("Failed to select CPAL palette for scribe font compile."),
                        HE_KV(palette_index, paletteIndex),
                        HE_KV(error, paletteSelectErr));
                    return false;
                }

                CompiledFontPalette& palette = out.palettes[paletteIndex];
                const uint16_t paletteFlags = paletteData.palette_flags != nullptr ? paletteData.palette_flags[paletteIndex] : 0u;
                const bool forLight = (paletteFlags & 0x0001u) != 0u;
                const bool forDark = (paletteFlags & 0x0002u) != 0u;
                palette.background = forLight && forDark
                    ? FontFacePaletteBackground::Any
                    : forLight
                        ? FontFacePaletteBackground::Light
                        : forDark
                            ? FontFacePaletteBackground::Dark
                            : FontFacePaletteBackground::Unspecified;
                palette.colors.Resize(paletteData.num_palette_entries);
                for (uint32_t colorIndex = 0; colorIndex < paletteData.num_palette_entries; ++colorIndex)
                {
                    palette.colors[colorIndex] = ToPaletteColor(paletteEntries[colorIndex]);
                }
            }

            for (uint32_t glyphIndex = 0; glyphIndex < glyphCount; ++glyphIndex)
            {
                FT_LayerIterator iterator{};
                iterator.p = nullptr;

                const uint32_t firstLayer = out.layers.Size();
                FT_UInt layerGlyphIndex = 0;
                FT_UInt colorIndex = 0;
                while (FT_Get_Color_Glyph_Layer(
                    face,
                    static_cast<FT_UInt>(glyphIndex),
                    &layerGlyphIndex,
                    &colorIndex,
                    &iterator))
                {
                    CompiledColorGlyphLayerEntry& layer = out.layers.EmplaceBack();
                    layer.glyphIndex = layerGlyphIndex;
                    layer.paletteEntryIndex = colorIndex;
                    layer.colorSource = FontFaceColorSource::Palette;
                    layer.transform00 = 1.0f;
                    layer.transform01 = 0.0f;
                    layer.transform10 = 0.0f;
                    layer.transform11 = 1.0f;
                    layer.transformTx = 0.0f;
                    layer.transformTy = 0.0f;

                    if (colorIndex == 0xFFFFu)
                    {
                        layer.paletteEntryIndex = 0;
                        layer.colorSource = FontFaceColorSource::Foreground;
                    }
                }

                const uint32_t layerCount = out.layers.Size() - firstLayer;
                uint32_t resolvedLayerCount = layerCount;
                if (resolvedLayerCount == 0)
                {
                    FT_OpaquePaint opaquePaint{};
                    if (FT_Get_Color_Glyph_Paint(
                        face,
                        static_cast<FT_UInt>(glyphIndex),
                        FT_COLOR_NO_ROOT_TRANSFORM,
                        &opaquePaint))
                    {
                        if (TryAppendV1LayersFromPaint(out.layers, face, opaquePaint, {}, 0))
                        {
                            resolvedLayerCount = out.layers.Size() - firstLayer;
                        }
                        else
                        {
                            out.layers.Resize(firstLayer);
                        }
                    }
                }

                if (resolvedLayerCount > 0)
                {
                    CompiledColorGlyphEntry& colorGlyph = out.colorGlyphs[glyphIndex];
                    colorGlyph.firstLayer = firstLayer;
                    colorGlyph.layerCount = resolvedLayerCount;
                    glyphs[glyphIndex].hasColorLayers = true;
                }
            }

            return true;
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
        if (!BuildCompiledFontPaintData(out.paint, out.glyphs, ftFace, glyphCount))
        {
            return false;
        }
        out.bandOverlapEpsilon = Max(1.0f, static_cast<float>(ftFace->units_per_EM) / 1024.0f);

        uint32_t estimatedStrokePointCount = 0;
        uint32_t estimatedStrokeCommandCount = 0;
        if (!EstimateCompiledStrokeStorage(
                estimatedStrokePointCount,
                estimatedStrokeCommandCount,
                ftFace,
                glyphCount,
                out.bandOverlapEpsilon))
        {
            return false;
        }
        out.strokePoints.Reserve(estimatedStrokePointCount);
        out.strokeCommands.Reserve(estimatedStrokeCommandCount);

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
            const int32_t advanceX = static_cast<int32_t>(ftFace->glyph->advance.x);
            const int32_t advanceY = static_cast<int32_t>(ftFace->glyph->advance.y);
            glyphEntry.advanceX = advanceX;
            glyphEntry.advanceY = advanceY;
            glyphEntry.fillRule = FillRule::NonZero;

            if ((ftFace->glyph->format != FT_GLYPH_FORMAT_OUTLINE) || (ftFace->glyph->outline.n_points == 0))
            {
                continue;
            }

            GlyphOutlineBuilder outlineBuilder(out.bandOverlapEpsilon);
            Vector<CurveData> curves{};
            Vector<StrokeSourcePoint> strokePoints{};
            Vector<StrokeSourceCommand> strokeCommands{};
            if (!outlineBuilder.Build(curves, strokePoints, strokeCommands, ftFace->glyph->outline))
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

            CompiledGlyphRenderEntry geometryEntry{};
            PackedBandStats bandStats{};
            CompiledShapeBuildOptions shapeOptions{};
            shapeOptions.bandOverlapEpsilon = out.bandOverlapEpsilon;
            if (!BuildCompiledShapeGeometry(
                    geometryEntry,
                    out.curveTexels,
                    out.bandTexels,
                    out.strokePoints,
                    out.strokeCommands,
                    bandStats,
                    Span<const CurveData>(curves.Data(), curves.Size()),
                    Span<const StrokeSourcePoint>(strokePoints.Data(), strokePoints.Size()),
                    Span<const StrokeSourceCommand>(strokeCommands.Data(), strokeCommands.Size()),
                    FillRule::NonZero,
                    shapeOptions))
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Failed to build shared glyph geometry for scribe font compile."),
                    HE_KV(face_index, faceIndex),
                    HE_KV(glyph_index, glyphIndex));
                return false;
            }

            geometryEntry.advanceX = advanceX;
            geometryEntry.advanceY = advanceY;
            geometryEntry.hasGeometry = true;
            geometryEntry.hasColorLayers = glyphEntry.hasColorLayers;
            glyphEntry = geometryEntry;

            out.bandHeaderCount += bandStats.headerCount;
            out.emittedBandPayloadTexelCount += bandStats.emittedPayloadTexelCount;
            out.reusedBandCount += bandStats.reusedBandCount;
            out.reusedBandPayloadTexelCount += bandStats.reusedPayloadTexelCount;
        }

        out.curveTextureWidth = curve_compile::CurveTextureWidth;
        PadCurveTexture(out.curveTexels, out.curveTextureWidth, out.curveTextureHeight);
        PadBandTexture(out.bandTexels, out.bandTextureWidth, out.bandTextureHeight);
        return true;
    }
}
