// Copyright Chad Engler

#include "image_compile_geometry.h"

#include "curve_compile_utils.h"

#include "he/core/log.h"
#include "he/core/math.h"
#include "he/core/string.h"
#include "he/core/string_ops.h"
#include "he/core/string_view.h"
#include "he/core/utils.h"

#include <algorithm>

namespace he::scribe::editor
{
    namespace
    {
        constexpr uint32_t CurveTextureWidth = curve_compile::CurveTextureWidth;
        constexpr uint32_t MaxBandCount = curve_compile::MaxBandCount;
        constexpr uint32_t MaxCubicSubdivisionDepth = curve_compile::MaxCubicSubdivisionDepth;
        constexpr float DefaultBandOverlapEpsilon = 1.0f / 1024.0f;
        constexpr float DegenerateLineLengthSq = curve_compile::DegenerateLineLengthSq;
        constexpr float DegenerateCurveExtent = curve_compile::DegenerateCurveExtent;
        using curve_compile::Affine2D;
        using curve_compile::CurveData;
        using curve_compile::CurveRef;
        using curve_compile::Point2;

        struct ParsedShape
        {
            Vector<CurveData> curves{};
            Vector<CompiledOutlinePoint> outlinePoints{};
            Vector<CompiledOutlineCommand> outlineCommands{};
            FillRule fillRule{ FillRule::NonZero };
            Vec4f color{ 0.0f, 0.0f, 0.0f, 1.0f };
            float minX{ 0.0f };
            float minY{ 0.0f };
            float maxX{ 0.0f };
            float maxY{ 0.0f };
        };

        struct ParsedDefinition
        {
            String id{};
            ParsedShape shape{};
        };

        struct ParsedImage
        {
            Vector<ParsedShape> shapes{};
            float viewBoxMinX{ 0.0f };
            float viewBoxMinY{ 0.0f };
            float viewBoxWidth{ 0.0f };
            float viewBoxHeight{ 0.0f };
            float boundsMinX{ 0.0f };
            float boundsMinY{ 0.0f };
            float boundsMaxX{ 0.0f };
            float boundsMaxY{ 0.0f };
            bool hasViewBox{ false };
        };

        struct StyleState
        {
            Vec4f fill{ 0.0f, 0.0f, 0.0f, 1.0f };
            FillRule fillRule{ FillRule::NonZero };
            bool fillNone{ false };
        };

        struct ParseState
        {
            Affine2D transform{};
            StyleState style{};
            bool inDefinitions{ false };
            bool suppressOutput{ false };
        };

        struct Attribute
        {
            StringView name{};
            StringView value{};
        };

        float DegreesToRadians(float degrees)
        {
            return degrees * 0.017453292519943295769f;
        }

        void RecomputeShapeBounds(ParsedShape& shape)
        {
            if (shape.curves.IsEmpty())
            {
                shape.minX = 0.0f;
                shape.minY = 0.0f;
                shape.maxX = 0.0f;
                shape.maxY = 0.0f;
                return;
            }

            shape.minX = shape.curves[0].minX;
            shape.minY = shape.curves[0].minY;
            shape.maxX = shape.curves[0].maxX;
            shape.maxY = shape.curves[0].maxY;
            for (uint32_t curveIndex = 1; curveIndex < shape.curves.Size(); ++curveIndex)
            {
                const CurveData& curve = shape.curves[curveIndex];
                shape.minX = Min(shape.minX, curve.minX);
                shape.minY = Min(shape.minY, curve.minY);
                shape.maxX = Max(shape.maxX, curve.maxX);
                shape.maxY = Max(shape.maxY, curve.maxY);
            }
        }

        void TransformCurveBounds(CurveData& curve)
        {
            if (curve.isLineLike)
            {
                curve.minX = Min(curve.p1.x, curve.p3.x);
                curve.minY = Min(curve.p1.y, curve.p3.y);
                curve.maxX = Max(curve.p1.x, curve.p3.x);
                curve.maxY = Max(curve.p1.y, curve.p3.y);
                return;
            }

            curve.minX = Min(curve.p1.x, Min(curve.p2.x, curve.p3.x));
            curve.minY = Min(curve.p1.y, Min(curve.p2.y, curve.p3.y));
            curve.maxX = Max(curve.p1.x, Max(curve.p2.x, curve.p3.x));
            curve.maxY = Max(curve.p1.y, Max(curve.p2.y, curve.p3.y));
        }

        ParsedShape CloneTransformedShape(const ParsedShape& source, const ParseState& state)
        {
            ParsedShape shape = source;
            shape.fillRule = state.style.fillRule;
            shape.color = state.style.fill;
            for (CurveData& curve : shape.curves)
            {
                curve.p1 = TransformPoint(state.transform, curve.p1);
                curve.p2 = TransformPoint(state.transform, curve.p2);
                curve.p3 = TransformPoint(state.transform, curve.p3);
                TransformCurveBounds(curve);
            }

            for (CompiledOutlinePoint& point : shape.outlinePoints)
            {
                const Point2 transformed = TransformPoint(state.transform, { point.x, point.y });
                point.x = transformed.x;
                point.y = transformed.y;
            }

            RecomputeShapeBounds(shape);
            return shape;
        }

        const Attribute* FindAttribute(Span<const Attribute> attrs, StringView name)
        {
            for (const Attribute& attr : attrs)
            {
                if (attr.name.EqualToI(name))
                {
                    return &attr;
                }
            }

            return nullptr;
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

        Affine2D MakeTranslateAffine(float tx, float ty)
        {
            Affine2D result{};
            result.tx = tx;
            result.ty = ty;
            return result;
        }

        Affine2D MakeScaleAffine(float sx, float sy)
        {
            Affine2D result{};
            result.m00 = sx;
            result.m11 = sy;
            return result;
        }

        Affine2D MakeRotateAffine(float degrees)
        {
            const float radians = DegreesToRadians(degrees);
            float s = 0.0f;
            float c = 1.0f;
            SinCos(radians, s, c);

            Affine2D result{};
            result.m00 = c;
            result.m01 = -s;
            result.m10 = s;
            result.m11 = c;
            return result;
        }

        Affine2D MakeRotateAffine(float degrees, float cx, float cy)
        {
            return ComposeAffine(
                MakeTranslateAffine(cx, cy),
                ComposeAffine(
                    MakeRotateAffine(degrees),
                    MakeTranslateAffine(-cx, -cy)));
        }

        Affine2D MakeSkewXAffine(float degrees)
        {
            Affine2D result{};
            result.m01 = Tan(DegreesToRadians(degrees));
            return result;
        }

        Affine2D MakeSkewYAffine(float degrees)
        {
            Affine2D result{};
            result.m10 = Tan(DegreesToRadians(degrees));
            return result;
        }

        [[maybe_unused]] float SvgDistanceToLineSq(const Point2& point, const Point2& a, const Point2& b)
        {
            return curve_compile::DistanceToLineSq(point, a, b);
        }

        void SvgAppendCurveTexels(Vector<PackedCurveTexel>& out, const CurveData& curve)
        {
            curve_compile::AppendCurveTexels(out, curve);
        }

        class CurveBuilder final
        {
        public:
            explicit CurveBuilder(float flatteningTolerance)
                : m_builder(flatteningTolerance)
            {
            }

            void SetTransform(const Affine2D& transform)
            {
                m_builder.SetTransform(transform);
            }

            void AddLine(const Point2& from, const Point2& to)
            {
                m_builder.AddLine(from, to);
            }

            void AddQuadratic(const Point2& p1, const Point2& p2, const Point2& p3)
            {
                m_builder.AddQuadratic(p1, p2, p3);
            }

            void AddCubic(const Point2& p0, const Point2& p1, const Point2& p2, const Point2& p3)
            {
                m_builder.AddCubic(p0, p1, p2, p3);
            }

            Vector<CurveData>& Curves() { return m_builder.Curves(); }

        private:
            curve_compile::CurveBuilder m_builder;
        };

        [[maybe_unused]] void ZeroCounts(Vector<uint32_t>& counts)
        {
            for (uint32_t i = 0; i < counts.Size(); ++i)
            {
                counts[i] = 0;
            }
        }

        [[maybe_unused]] void GetBandRange(
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

        uint32_t SvgChooseBandCount(
            const Vector<CurveData>& curves,
            bool horizontalBands,
            float boundsMin,
            float boundsMax,
            float epsilon)
        {
            return curve_compile::ChooseBandCount(curves, horizontalBands, boundsMin, boundsMax, epsilon);
        }

        void SvgBuildBandRefs(
            Vector<Vector<CurveRef>>& outBands,
            const Vector<CurveData>& curves,
            bool horizontalBands,
            float boundsMin,
            float boundsMax,
            uint32_t bandCount,
            float epsilon)
        {
            curve_compile::BuildBandRefs(outBands, curves, horizontalBands, boundsMin, boundsMax, bandCount, epsilon);
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

        bool IsWhitespace(char c)
        {
            return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n') || (c == '\f');
        }

        bool IsNameChar(char c)
        {
            return ((c >= 'a') && (c <= 'z'))
                || ((c >= 'A') && (c <= 'Z'))
                || ((c >= '0') && (c <= '9'))
                || (c == '_')
                || (c == '-')
                || (c == ':');
        }

        StringView TrimView(StringView value)
        {
            uint32_t begin = 0;
            uint32_t end = value.Size();
            while ((begin < end) && IsWhitespace(value[begin]))
            {
                ++begin;
            }

            while ((end > begin) && IsWhitespace(value[end - 1]))
            {
                --end;
            }

            return { value.Data() + begin, end - begin };
        }

        bool ParseFloat(StringView text, float& out)
        {
            const char* begin = text.Data();
            const char* end = text.Data() + text.Size();
            const char* parseEnd = end;
            return StrToFloat(out, begin, &parseEnd) && (parseEnd == end);
        }

        bool ParseNumberList(Vector<float>& out, StringView text)
        {
            out.Clear();
            const char* cur = text.Data();
            const char* endText = text.Data() + text.Size();

            while (cur < endText)
            {
                while ((cur < endText) && (IsWhitespace(*cur) || (*cur == ',')))
                {
                    ++cur;
                }

                if (cur >= endText)
                {
                    break;
                }

                float value = 0.0f;
                const char* parseEnd = endText;
                if (!StrToFloat(value, cur, &parseEnd) || (parseEnd == cur))
                {
                    return false;
                }

                out.PushBack(value);
                cur = parseEnd;
            }

            return true;
        }

        bool ParseColor(Vec4f& out, StringView text)
        {
            const StringView trimmed = TrimView(text);
            if (trimmed.IsEmpty())
            {
                return false;
            }

            if (trimmed.EqualToI("none"))
            {
                out = { 0.0f, 0.0f, 0.0f, 0.0f };
                return true;
            }

            if (trimmed[0] == '#')
            {
                auto HexValue = [](char c) -> int32_t
                {
                    if ((c >= '0') && (c <= '9'))
                    {
                        return c - '0';
                    }

                    if ((c >= 'a') && (c <= 'f'))
                    {
                        return 10 + (c - 'a');
                    }

                    if ((c >= 'A') && (c <= 'F'))
                    {
                        return 10 + (c - 'A');
                    }

                    return -1;
                };

                auto ParseByte = [&](char a, char b, float& value) -> bool
                {
                    const int32_t high = HexValue(a);
                    const int32_t low = HexValue(b);
                    if ((high < 0) || (low < 0))
                    {
                        return false;
                    }

                    value = static_cast<float>((high << 4) | low) / 255.0f;
                    return true;
                };

                if ((trimmed.Size() == 7) || (trimmed.Size() == 9))
                {
                    out.w = 1.0f;
                    return ParseByte(trimmed[1], trimmed[2], out.x)
                        && ParseByte(trimmed[3], trimmed[4], out.y)
                        && ParseByte(trimmed[5], trimmed[6], out.z)
                        && ((trimmed.Size() == 7) || ParseByte(trimmed[7], trimmed[8], out.w));
                }
            }

            if (trimmed.EqualToI("black"))
            {
                out = { 0.0f, 0.0f, 0.0f, 1.0f };
                return true;
            }

            if (trimmed.EqualToI("white"))
            {
                out = { 1.0f, 1.0f, 1.0f, 1.0f };
                return true;
            }

            return false;
        }

        bool ParseTransform(Affine2D& out, StringView text)
        {
            out = {};

            const char* cur = text.Data();
            const char* end = text.Data() + text.Size();
            Vector<float> values{};

            while (cur < end)
            {
                while ((cur < end) && IsWhitespace(*cur))
                {
                    ++cur;
                }

                if (cur >= end)
                {
                    break;
                }

                const char* nameBegin = cur;
                while ((cur < end) && IsNameChar(*cur))
                {
                    ++cur;
                }

                const StringView name{ nameBegin, static_cast<uint32_t>(cur - nameBegin) };
                while ((cur < end) && IsWhitespace(*cur))
                {
                    ++cur;
                }

                if ((cur >= end) || (*cur != '('))
                {
                    return false;
                }

                ++cur;
                const char* argsBegin = cur;
                int32_t depth = 1;
                while ((cur < end) && (depth > 0))
                {
                    if (*cur == '(')
                    {
                        ++depth;
                    }
                    else if (*cur == ')')
                    {
                        --depth;
                    }

                    if (depth > 0)
                    {
                        ++cur;
                    }
                }

                if (depth != 0)
                {
                    return false;
                }

                const StringView args{ argsBegin, static_cast<uint32_t>(cur - argsBegin) };
                ++cur;

                if (!ParseNumberList(values, args))
                {
                    return false;
                }

                Affine2D next{};
                if (name.EqualToI("translate"))
                {
                    if (values.IsEmpty() || (values.Size() > 2))
                    {
                        return false;
                    }

                    next = MakeTranslateAffine(values[0], values.Size() > 1 ? values[1] : 0.0f);
                }
                else if (name.EqualToI("scale"))
                {
                    if (values.IsEmpty() || (values.Size() > 2))
                    {
                        return false;
                    }

                    next = MakeScaleAffine(values[0], values.Size() > 1 ? values[1] : values[0]);
                }
                else if (name.EqualToI("rotate"))
                {
                    if ((values.Size() != 1) && (values.Size() != 3))
                    {
                        return false;
                    }

                    next = (values.Size() == 1)
                        ? MakeRotateAffine(values[0])
                        : MakeRotateAffine(values[0], values[1], values[2]);
                }
                else if (name.EqualToI("skewX"))
                {
                    if (values.Size() != 1)
                    {
                        return false;
                    }

                    next = MakeSkewXAffine(values[0]);
                }
                else if (name.EqualToI("skewY"))
                {
                    if (values.Size() != 1)
                    {
                        return false;
                    }

                    next = MakeSkewYAffine(values[0]);
                }
                else if (name.EqualToI("matrix"))
                {
                    if (values.Size() != 6)
                    {
                        return false;
                    }

                    next.m00 = values[0];
                    next.m10 = values[1];
                    next.m01 = values[2];
                    next.m11 = values[3];
                    next.tx = values[4];
                    next.ty = values[5];
                }
                else
                {
                    return false;
                }

                out = ComposeAffine(out, next);
            }

            return true;
        }

        void ApplyStyleProperty(StyleState& style, StringView name, StringView value)
        {
            const StringView trimmedName = TrimView(name);
            const StringView trimmedValue = TrimView(value);

            if (trimmedName.EqualToI("fill"))
            {
                Vec4f color{};
                if (ParseColor(color, trimmedValue))
                {
                    style.fill = color;
                    style.fillNone = (color.w == 0.0f);
                }
            }
            else if (trimmedName.EqualToI("fill-rule"))
            {
                if (trimmedValue.EqualToI("evenodd"))
                {
                    style.fillRule = FillRule::EvenOdd;
                }
                else if (trimmedValue.EqualToI("nonzero"))
                {
                    style.fillRule = FillRule::NonZero;
                }
            }
            else if (trimmedName.EqualToI("opacity") || trimmedName.EqualToI("fill-opacity"))
            {
                float alpha = 1.0f;
                if (ParseFloat(trimmedValue, alpha))
                {
                    style.fill.w *= Clamp(alpha, 0.0f, 1.0f);
                    style.fillNone = (style.fill.w <= 0.0f);
                }
            }
        }

        void ApplyPresentationAttributes(ParseState& state, Span<const Attribute> attrs)
        {
            for (const Attribute& attr : attrs)
            {
                if (attr.name.EqualToI("transform"))
                {
                    Affine2D transform{};
                    if (ParseTransform(transform, attr.value))
                    {
                        state.transform = ComposeAffine(state.transform, transform);
                    }
                }
                else if (attr.name.EqualToI("style"))
                {
                    const char* cur = attr.value.Data();
                    const char* end = attr.value.Data() + attr.value.Size();
                    while (cur < end)
                    {
                        const char* nameBegin = cur;
                        while ((cur < end) && (*cur != ':') && (*cur != ';'))
                        {
                            ++cur;
                        }

                        const StringView name{ nameBegin, static_cast<uint32_t>(cur - nameBegin) };
                        if ((cur >= end) || (*cur != ':'))
                        {
                            while ((cur < end) && (*cur != ';'))
                            {
                                ++cur;
                            }
                        }
                        else
                        {
                            ++cur;
                            const char* valueBegin = cur;
                            while ((cur < end) && (*cur != ';'))
                            {
                                ++cur;
                            }

                            const StringView value{ valueBegin, static_cast<uint32_t>(cur - valueBegin) };
                            ApplyStyleProperty(state.style, name, value);
                        }

                        if ((cur < end) && (*cur == ';'))
                        {
                            ++cur;
                        }
                    }
                }
                else
                {
                    ApplyStyleProperty(state.style, attr.name, attr.value);
                }
            }
        }

        class SvgParser final
        {
        public:
            bool Parse(ParsedImage& out, StringView text, float flatteningTolerance);

        private:
            bool ParseContainer(ParsedImage& out, const ParseState& inherited, StringView closingTag);
            void SkipWhitespace();
            void SkipTrivia();
            bool SkipMarkup();
            bool Consume(char c);
            StringView ParseName();
            bool ParseAttributes(Vector<Attribute>& out, bool& outSelfClosing);
            void ParseSvgAttributes(ParsedImage& out, Span<const Attribute> attrs);
            bool ParsePathElement(ParsedImage& out, const ParseState& state, Span<const Attribute> attrs);
            bool ParseUseElement(ParsedImage& out, const ParseState& state, Span<const Attribute> attrs);
            bool ParsePathData(
                CurveBuilder& builder,
                Vector<CompiledOutlinePoint>& outPoints,
                Vector<CompiledOutlineCommand>& outCommands,
                StringView text);
            bool SkipToClosingTag(StringView tagName);
            bool SkipElement(StringView tagName);
            const ParsedShape* FindDefinition(StringView id) const;

        private:
            StringView m_text{};
            const char* m_cur{ nullptr };
            const char* m_end{ nullptr };
            float m_flatteningTolerance{ 0.25f };
            Vector<ParsedDefinition> m_definitions{};
        };

        bool SvgParser::Parse(ParsedImage& out, StringView text, float flatteningTolerance)
        {
            m_text = text;
            m_cur = text.Data();
            m_end = text.Data() + text.Size();
            m_flatteningTolerance = Max(flatteningTolerance, 0.01f);
            out = {};
            m_definitions.Clear();

            ParseState rootState{};
            if (!ParseContainer(out, rootState, {}))
            {
                return false;
            }

            if (out.shapes.IsEmpty())
            {
                HE_LOG_ERROR(he_scribe, HE_MSG("SVG parser did not find any filled path geometry."));
                return false;
            }

            if (!out.hasViewBox)
            {
                out.viewBoxMinX = out.boundsMinX;
                out.viewBoxMinY = out.boundsMinY;
                out.viewBoxWidth = Max(out.boundsMaxX - out.boundsMinX, 1.0f);
                out.viewBoxHeight = Max(out.boundsMaxY - out.boundsMinY, 1.0f);
                out.hasViewBox = true;
            }

            return true;
        }

        bool SvgParser::ParseContainer(ParsedImage& out, const ParseState& inherited, StringView closingTag)
        {
            while (true)
            {
                SkipTrivia();
                if (m_cur >= m_end)
                {
                    return closingTag.IsEmpty();
                }

                if ((m_end - m_cur) >= 2 && (m_cur[0] == '<') && (m_cur[1] == '/'))
                {
                    m_cur += 2;
                    const StringView tagName = ParseName();
                    if (tagName.IsEmpty())
                    {
                        return false;
                    }

                    SkipWhitespace();
                    if (!Consume('>'))
                    {
                        return false;
                    }

                    return tagName.EqualToI(closingTag);
                }

                if (!Consume('<'))
                {
                    return false;
                }

                if ((m_cur < m_end) && ((*m_cur == '!') || (*m_cur == '?')))
                {
                    if (!SkipMarkup())
                    {
                        return false;
                    }
                    continue;
                }

                const StringView tagName = ParseName();
                if (tagName.IsEmpty())
                {
                    return false;
                }

                Vector<Attribute> attrs{};
                bool selfClosing = false;
                if (!ParseAttributes(attrs, selfClosing))
                {
                    return false;
                }

                ParseState state = inherited;
                ApplyPresentationAttributes(state, attrs);

                if (tagName.EqualToI("svg"))
                {
                    ParseSvgAttributes(out, attrs);
                }
                else if (tagName.EqualToI("defs"))
                {
                    state.inDefinitions = true;
                    state.suppressOutput = true;
                }
                else if (tagName.EqualToI("clipPath")
                    || tagName.EqualToI("mask")
                    || tagName.EqualToI("filter")
                    || tagName.EqualToI("symbol"))
                {
                    state.suppressOutput = true;
                }

                if (tagName.EqualToI("path"))
                {
                    if (!ParsePathElement(out, state, attrs))
                    {
                        return false;
                    }

                    if (!selfClosing && !SkipToClosingTag(tagName))
                    {
                        return false;
                    }
                }
                else if (tagName.EqualToI("use"))
                {
                    if (!ParseUseElement(out, state, attrs))
                    {
                        return false;
                    }

                    if (!selfClosing && !SkipToClosingTag(tagName))
                    {
                        return false;
                    }
                }
                else if (tagName.EqualToI("text")
                    || tagName.EqualToI("tspan")
                    || tagName.EqualToI("title")
                    || tagName.EqualToI("desc")
                    || tagName.EqualToI("metadata")
                    || tagName.EqualToI("style")
                    || tagName.EqualToI("script"))
                {
                    if (!selfClosing && !SkipElement(tagName))
                    {
                        return false;
                    }
                }
                else if (!selfClosing)
                {
                    if (!ParseContainer(out, state, tagName))
                    {
                        return false;
                    }
                }
            }
        }

        void SvgParser::SkipWhitespace()
        {
            while ((m_cur < m_end) && IsWhitespace(*m_cur))
            {
                ++m_cur;
            }
        }

        void SvgParser::SkipTrivia()
        {
            while (true)
            {
                SkipWhitespace();
                if (((m_end - m_cur) >= 4)
                    && (m_cur[0] == '<')
                    && (m_cur[1] == '!')
                    && (m_cur[2] == '-')
                    && (m_cur[3] == '-'))
                {
                    m_cur += 4;
                    while (((m_end - m_cur) >= 3)
                        && !((m_cur[0] == '-') && (m_cur[1] == '-') && (m_cur[2] == '>')))
                    {
                        ++m_cur;
                    }

                    if ((m_end - m_cur) < 3)
                    {
                        return;
                    }

                    m_cur += 3;
                    continue;
                }

                break;
            }
        }

        bool SvgParser::SkipMarkup()
        {
            while ((m_cur < m_end) && (*m_cur != '>'))
            {
                ++m_cur;
            }

            return Consume('>');
        }

        bool SvgParser::Consume(char c)
        {
            if ((m_cur < m_end) && (*m_cur == c))
            {
                ++m_cur;
                return true;
            }

            return false;
        }

        StringView SvgParser::ParseName()
        {
            const char* begin = m_cur;
            while ((m_cur < m_end) && IsNameChar(*m_cur))
            {
                ++m_cur;
            }

            return { begin, static_cast<uint32_t>(m_cur - begin) };
        }

        bool SvgParser::ParseAttributes(Vector<Attribute>& out, bool& outSelfClosing)
        {
            out.Clear();
            outSelfClosing = false;

            while (true)
            {
                SkipWhitespace();
                if (m_cur >= m_end)
                {
                    return false;
                }

                if (*m_cur == '/')
                {
                    ++m_cur;
                    outSelfClosing = true;
                    return Consume('>');
                }

                if (*m_cur == '>')
                {
                    ++m_cur;
                    return true;
                }

                const StringView name = ParseName();
                if (name.IsEmpty())
                {
                    return false;
                }

                SkipWhitespace();
                if (!Consume('='))
                {
                    return false;
                }

                SkipWhitespace();
                if ((m_cur >= m_end) || ((*m_cur != '"') && (*m_cur != '\'')))
                {
                    return false;
                }

                const char quote = *m_cur++;
                const char* valueBegin = m_cur;
                while ((m_cur < m_end) && (*m_cur != quote))
                {
                    ++m_cur;
                }

                if (m_cur >= m_end)
                {
                    return false;
                }

                const StringView value{ valueBegin, static_cast<uint32_t>(m_cur - valueBegin) };
                ++m_cur;

                Attribute& attr = out.EmplaceBack();
                attr.name = name;
                attr.value = value;
            }
        }

        void SvgParser::ParseSvgAttributes(ParsedImage& out, Span<const Attribute> attrs)
        {
            for (const Attribute& attr : attrs)
            {
                if (attr.name.EqualToI("viewBox"))
                {
                    Vector<float> values{};
                    if (ParseNumberList(values, attr.value) && (values.Size() == 4))
                    {
                        out.viewBoxMinX = values[0];
                        out.viewBoxMinY = values[1];
                        out.viewBoxWidth = Max(values[2], 1.0f);
                        out.viewBoxHeight = Max(values[3], 1.0f);
                        out.hasViewBox = true;
                    }
                }
                else if (!out.hasViewBox && attr.name.EqualToI("width"))
                {
                    ParseFloat(attr.value, out.viewBoxWidth);
                }
                else if (!out.hasViewBox && attr.name.EqualToI("height"))
                {
                    ParseFloat(attr.value, out.viewBoxHeight);
                }
            }
        }

        bool SvgParser::ParsePathElement(ParsedImage& out, const ParseState& state, Span<const Attribute> attrs)
        {
            StringView d{};
            StringView id{};
            for (const Attribute& attr : attrs)
            {
                if (attr.name.EqualToI("d"))
                {
                    d = attr.value;
                }
                else if (attr.name.EqualToI("id"))
                {
                    id = attr.value;
                }
            }

            if (d.IsEmpty())
            {
                return true;
            }

            CurveBuilder builder(m_flatteningTolerance);
            builder.SetTransform(state.transform);
            Vector<CompiledOutlinePoint> outlinePoints{};
            Vector<CompiledOutlineCommand> outlineCommands{};
            if (!ParsePathData(builder, outlinePoints, outlineCommands, d))
            {
                HE_LOG_ERROR(he_scribe, HE_MSG("Failed to parse SVG path data for scribe image compile."));
                return false;
            }

            Vector<CurveData>& curves = builder.Curves();
            if (curves.IsEmpty())
            {
                return true;
            }

            ParsedShape shape{};
            shape.curves = Move(curves);
            shape.outlinePoints = Move(outlinePoints);
            shape.outlineCommands = Move(outlineCommands);
            shape.fillRule = state.style.fillRule;
            shape.color = state.style.fill;
            RecomputeShapeBounds(shape);

            if (state.inDefinitions)
            {
                if (!id.IsEmpty())
                {
                    ParsedDefinition& definition = m_definitions.EmplaceBack();
                    definition.id = String(id);
                    definition.shape = Move(shape);
                }

                return true;
            }

            if (state.suppressOutput || state.style.fillNone || (state.style.fill.w <= 0.0f))
            {
                return true;
            }

            ParsedShape& outShape = out.shapes.EmplaceBack();
            outShape = Move(shape);
            if (out.shapes.Size() == 1)
            {
                out.boundsMinX = outShape.minX;
                out.boundsMinY = outShape.minY;
                out.boundsMaxX = outShape.maxX;
                out.boundsMaxY = outShape.maxY;
            }
            else
            {
                out.boundsMinX = Min(out.boundsMinX, outShape.minX);
                out.boundsMinY = Min(out.boundsMinY, outShape.minY);
                out.boundsMaxX = Max(out.boundsMaxX, outShape.maxX);
                out.boundsMaxY = Max(out.boundsMaxY, outShape.maxY);
            }

            return true;
        }

        bool SvgParser::ParseUseElement(ParsedImage& out, const ParseState& state, Span<const Attribute> attrs)
        {
            if (state.suppressOutput || state.style.fillNone || (state.style.fill.w <= 0.0f))
            {
                return true;
            }

            const Attribute* hrefAttr = FindAttribute(attrs, "href");
            if (!hrefAttr)
            {
                hrefAttr = FindAttribute(attrs, "xlink:href");
            }

            if (!hrefAttr || hrefAttr->value.IsEmpty())
            {
                return true;
            }

            StringView refId = hrefAttr->value;
            if ((refId.Size() > 0) && (refId[0] == '#'))
            {
                refId = refId.Substring(1);
            }

            const ParsedShape* definition = FindDefinition(refId);
            if (!definition)
            {
                return true;
            }

            ParseState instanceState = state;

            float x = 0.0f;
            float y = 0.0f;
            if (const Attribute* xAttr = FindAttribute(attrs, "x"))
            {
                ParseFloat(xAttr->value, x);
            }

            if (const Attribute* yAttr = FindAttribute(attrs, "y"))
            {
                ParseFloat(yAttr->value, y);
            }

            if ((x != 0.0f) || (y != 0.0f))
            {
                Affine2D translation{};
                translation.tx = x;
                translation.ty = y;
                instanceState.transform = ComposeAffine(instanceState.transform, translation);
            }

            ParsedShape& outShape = out.shapes.EmplaceBack();
            outShape = CloneTransformedShape(*definition, instanceState);
            if (out.shapes.Size() == 1)
            {
                out.boundsMinX = outShape.minX;
                out.boundsMinY = outShape.minY;
                out.boundsMaxX = outShape.maxX;
                out.boundsMaxY = outShape.maxY;
            }
            else
            {
                out.boundsMinX = Min(out.boundsMinX, outShape.minX);
                out.boundsMinY = Min(out.boundsMinY, outShape.minY);
                out.boundsMaxX = Max(out.boundsMaxX, outShape.maxX);
                out.boundsMaxY = Max(out.boundsMaxY, outShape.maxY);
            }

            return true;
        }

        bool SvgParser::ParsePathData(
            CurveBuilder& builder,
            Vector<CompiledOutlinePoint>& outPoints,
            Vector<CompiledOutlineCommand>& outCommands,
            StringView text)
        {
            const char* cur = text.Data();
            const char* end = text.Data() + text.Size();

            outPoints.Clear();
            outCommands.Clear();

            Point2 current{};
            Point2 subpathStart{};
            char command = 0;
            bool hasCurrent = false;
            bool subpathClosed = false;

            auto SkipSeparators = [&]()
            {
                while ((cur < end) && (IsWhitespace(*cur) || (*cur == ',')))
                {
                    ++cur;
                }
            };

            auto ReadFloatValue = [&](float& value) -> bool
            {
                SkipSeparators();
                if (cur >= end)
                {
                    return false;
                }

                const char* parseEnd = end;
                if (!StrToFloat(value, cur, &parseEnd) || (parseEnd == cur))
                {
                    return false;
                }

                cur = parseEnd;
                return true;
            };

            auto ReadPoint = [&](bool relative, Point2& point) -> bool
            {
                if (!ReadFloatValue(point.x) || !ReadFloatValue(point.y))
                {
                    return false;
                }

                if (relative)
                {
                    point.x += current.x;
                    point.y += current.y;
                }

                return true;
            };

            auto AppendOutlinePoint = [&](const Point2& point)
            {
                CompiledOutlinePoint& dst = outPoints.EmplaceBack();
                dst.x = point.x;
                dst.y = point.y;
            };

            auto AppendOutlineCommand = [&](OutlineCommandType type, const Point2* points, uint32_t pointCount)
            {
                CompiledOutlineCommand& commandEntry = outCommands.EmplaceBack();
                commandEntry.type = type;
                commandEntry.firstPoint = outPoints.Size();
                for (uint32_t pointIndex = 0; pointIndex < pointCount; ++pointIndex)
                {
                    AppendOutlinePoint(points[pointIndex]);
                }
            };

            auto AppendClose = [&]()
            {
                CompiledOutlineCommand& commandEntry = outCommands.EmplaceBack();
                commandEntry.type = OutlineCommandType::Close;
                commandEntry.firstPoint = outPoints.Size();
            };

            while (true)
            {
                SkipSeparators();
                if (cur >= end)
                {
                    break;
                }

                if (((*cur >= 'A') && (*cur <= 'Z')) || ((*cur >= 'a') && (*cur <= 'z')))
                {
                    command = *cur++;
                }
                else if (command == 0)
                {
                    return false;
                }

                const bool relative = (command >= 'a') && (command <= 'z');
                const char upper = static_cast<char>(command & ~0x20);
                switch (upper)
                {
                    case 'M':
                    {
                        Point2 point{};
                        if (!ReadPoint(relative, point))
                        {
                            return false;
                        }

                        if (hasCurrent && !subpathClosed)
                        {
                            // Leave the previous contour open when a new move starts without a close.
                        }

                        AppendOutlineCommand(OutlineCommandType::MoveTo, &point, 1);
                        current = point;
                        subpathStart = point;
                        hasCurrent = true;
                        subpathClosed = false;
                        command = relative ? 'l' : 'L';
                        break;
                    }

                    case 'L':
                    {
                        Point2 point{};
                        if (!ReadPoint(relative, point))
                        {
                            return false;
                        }

                        builder.AddLine(current, point);
                        AppendOutlineCommand(OutlineCommandType::LineTo, &point, 1);
                        current = point;
                        hasCurrent = true;
                        break;
                    }

                    case 'H':
                    {
                        float value = 0.0f;
                        if (!ReadFloatValue(value))
                        {
                            return false;
                        }

                        Point2 point = current;
                        point.x = relative ? (current.x + value) : value;
                        builder.AddLine(current, point);
                        AppendOutlineCommand(OutlineCommandType::LineTo, &point, 1);
                        current = point;
                        hasCurrent = true;
                        break;
                    }

                    case 'V':
                    {
                        float value = 0.0f;
                        if (!ReadFloatValue(value))
                        {
                            return false;
                        }

                        Point2 point = current;
                        point.y = relative ? (current.y + value) : value;
                        builder.AddLine(current, point);
                        AppendOutlineCommand(OutlineCommandType::LineTo, &point, 1);
                        current = point;
                        hasCurrent = true;
                        break;
                    }

                    case 'Q':
                    {
                        Point2 control{};
                        Point2 point{};
                        if (!ReadPoint(relative, control) || !ReadPoint(relative, point))
                        {
                            return false;
                        }

                        builder.AddQuadratic(current, control, point);
                        const Point2 points[] = { control, point };
                        AppendOutlineCommand(OutlineCommandType::QuadraticTo, points, HE_LENGTH_OF(points));
                        current = point;
                        hasCurrent = true;
                        break;
                    }

                    case 'C':
                    {
                        Point2 c1{};
                        Point2 c2{};
                        Point2 point{};
                        if (!ReadPoint(relative, c1)
                            || !ReadPoint(relative, c2)
                            || !ReadPoint(relative, point))
                        {
                            return false;
                        }

                        builder.AddCubic(current, c1, c2, point);
                        const Point2 points[] = { c1, c2, point };
                        AppendOutlineCommand(OutlineCommandType::CubicTo, points, HE_LENGTH_OF(points));
                        current = point;
                        hasCurrent = true;
                        break;
                    }

                    case 'Z':
                    {
                        if (hasCurrent)
                        {
                            builder.AddLine(current, subpathStart);
                            AppendClose();
                            current = subpathStart;
                            subpathClosed = true;
                        }
                        break;
                    }

                    default:
                        return false;
                }
            }

            return true;
        }

        bool SvgParser::SkipToClosingTag(StringView tagName)
        {
            while (m_cur < m_end)
            {
                if (((m_end - m_cur) >= 2) && (m_cur[0] == '<') && (m_cur[1] == '/'))
                {
                    const char* save = m_cur + 2;
                    const char* cur = save;
                    while ((cur < m_end) && IsNameChar(*cur))
                    {
                        ++cur;
                    }

                    const StringView name{ save, static_cast<uint32_t>(cur - save) };
                    if (!name.EqualToI(tagName))
                    {
                        return false;
                    }

                    m_cur = cur;
                    SkipWhitespace();
                    return Consume('>');
                }

                ++m_cur;
            }

            return false;
        }

        bool SvgParser::SkipElement(StringView tagName)
        {
            uint32_t depth = 1;
            while (m_cur < m_end)
            {
                if (*m_cur != '<')
                {
                    ++m_cur;
                    continue;
                }

                ++m_cur;
                if ((m_cur < m_end) && ((*m_cur == '!') || (*m_cur == '?')))
                {
                    if (!SkipMarkup())
                    {
                        return false;
                    }

                    continue;
                }

                const bool isClosing = Consume('/');
                const StringView nestedTag = ParseName();
                if (nestedTag.IsEmpty())
                {
                    return false;
                }

                Vector<Attribute> attrs{};
                bool selfClosing = false;
                if (!ParseAttributes(attrs, selfClosing))
                {
                    return false;
                }

                if (isClosing)
                {
                    if (nestedTag.EqualToI(tagName))
                    {
                        --depth;
                        if (depth == 0)
                        {
                            return true;
                        }
                    }
                }
                else if (!selfClosing && nestedTag.EqualToI(tagName))
                {
                    ++depth;
                }
            }

            return false;
        }

        const ParsedShape* SvgParser::FindDefinition(StringView id) const
        {
            for (const ParsedDefinition& definition : m_definitions)
            {
                const StringView definitionId(definition.id.Data(), definition.id.Size());
                if (definitionId.EqualToI(id))
                {
                    return &definition.shape;
                }
            }

            return nullptr;
        }

        bool BuildCompiledShape(
            CompiledVectorShapeRenderEntry& outShape,
            Vector<PackedCurveTexel>& outCurveTexels,
            Vector<PackedBandTexel>& outBandTexels,
            Vector<CompiledOutlinePoint>& outOutlinePoints,
            Vector<CompiledOutlineCommand>& outOutlineCommands,
            PackedBandStats& outBandStats,
            const ParsedShape& shape,
            float epsilon)
        {
            outShape = {};
            outBandStats = {};
            if (shape.curves.IsEmpty())
            {
                return false;
            }

            outShape.boundsMinX = shape.minX;
            outShape.boundsMinY = shape.minY;
            outShape.boundsMaxX = shape.maxX;
            outShape.boundsMaxY = shape.maxY;
            outShape.fillRule = shape.fillRule;
            outShape.firstOutlineCommand = outOutlineCommands.Size();
            outShape.outlineCommandCount = shape.outlineCommands.Size();
            const uint32_t pointBase = outOutlinePoints.Size();

            if (!shape.outlinePoints.IsEmpty())
            {
                outOutlinePoints.Insert(outOutlinePoints.Size(), shape.outlinePoints.Data(), shape.outlinePoints.Size());
            }

            if (!shape.outlineCommands.IsEmpty())
            {
                Vector<CompiledOutlineCommand> commands = shape.outlineCommands;
                for (CompiledOutlineCommand& command : commands)
                {
                    if (command.type != OutlineCommandType::Close)
                    {
                        command.firstPoint += pointBase;
                    }
                }

                outOutlineCommands.Insert(outOutlineCommands.Size(), commands.Data(), commands.Size());
            }

            Vector<CurveData> curves = shape.curves;
            for (uint32_t curveIndex = 0; curveIndex < curves.Size(); ++curveIndex)
            {
                CurveData& curve = curves[curveIndex];
                curve.curveTexelIndex = outCurveTexels.Size();
                SvgAppendCurveTexels(outCurveTexels, curve);
            }

            const uint32_t bandCountX = SvgChooseBandCount(curves, false, shape.minX, shape.maxX, epsilon);
            const uint32_t bandCountY = SvgChooseBandCount(curves, true, shape.minY, shape.maxY, epsilon);

            Vector<Vector<CurveRef>> xBands{};
            Vector<Vector<CurveRef>> yBands{};
            SvgBuildBandRefs(xBands, curves, false, shape.minX, shape.maxX, bandCountX, epsilon);
            SvgBuildBandRefs(yBands, curves, true, shape.minY, shape.maxY, bandCountY, epsilon);

            outShape.bandScaleX = ComputeBandScale(shape.minX, shape.maxX, bandCountX);
            outShape.bandScaleY = ComputeBandScale(shape.minY, shape.maxY, bandCountY);
            outShape.bandOffsetX = -shape.minX * outShape.bandScaleX;
            outShape.bandOffsetY = -shape.minY * outShape.bandScaleY;
            outShape.glyphBandLocX = outBandTexels.Size() % ScribeBandTextureWidth;
            outShape.glyphBandLocY = outBandTexels.Size() / ScribeBandTextureWidth;
            outShape.bandMaxX = bandCountX > 0 ? bandCountX - 1 : 0;
            outShape.bandMaxY = bandCountY > 0 ? bandCountY - 1 : 0;

            const uint32_t glyphBandStart = outBandTexels.Size();
            const PackedBandStats bandStats = AppendPackedBands(
                outBandTexels,
                glyphBandStart,
                yBands,
                xBands);
            outBandStats = bandStats;

            return true;
        }
    }

    bool BuildCompiledVectorImageData(
        CompiledVectorImageData& out,
        Span<const uint8_t> sourceBytes,
        float flatteningTolerance)
    {
        out = {};
        if (sourceBytes.IsEmpty())
        {
            return false;
        }

        const StringView sourceText(reinterpret_cast<const char*>(sourceBytes.Data()), sourceBytes.Size());
        ParsedImage parsed{};
        SvgParser parser{};
        if (!parser.Parse(parsed, sourceText, flatteningTolerance))
        {
            return false;
        }

        out.bandOverlapEpsilon = DefaultBandOverlapEpsilon;
        out.curveTextureWidth = CurveTextureWidth;
        out.bandTextureWidth = ScribeBandTextureWidth;
        out.viewBoxMinX = parsed.viewBoxMinX;
        out.viewBoxMinY = parsed.viewBoxMinY;
        out.viewBoxWidth = parsed.viewBoxWidth;
        out.viewBoxHeight = parsed.viewBoxHeight;
        out.boundsMinX = parsed.boundsMinX;
        out.boundsMinY = parsed.boundsMinY;
        out.boundsMaxX = parsed.boundsMaxX;
        out.boundsMaxY = parsed.boundsMaxY;

        out.shapes.Reserve(parsed.shapes.Size());
        out.layers.Reserve(parsed.shapes.Size());
        for (uint32_t shapeIndex = 0; shapeIndex < parsed.shapes.Size(); ++shapeIndex)
        {
            const ParsedShape& shape = parsed.shapes[shapeIndex];
            CompiledVectorShapeRenderEntry& compiledShape = out.shapes.EmplaceBack();
            PackedBandStats bandStats{};
            if (!BuildCompiledShape(
                compiledShape,
                out.curveTexels,
                out.bandTexels,
                out.outlinePoints,
                out.outlineCommands,
                bandStats,
                shape,
                out.bandOverlapEpsilon))
            {
                return false;
            }
            out.bandHeaderCount += bandStats.headerCount;
            out.emittedBandPayloadTexelCount += bandStats.emittedPayloadTexelCount;
            out.reusedBandCount += bandStats.reusedBandCount;
            out.reusedBandPayloadTexelCount += bandStats.reusedPayloadTexelCount;

            CompiledVectorImageLayerEntry& layer = out.layers.EmplaceBack();
            layer.shapeIndex = shapeIndex;
            layer.red = shape.color.x;
            layer.green = shape.color.y;
            layer.blue = shape.color.z;
            layer.alpha = shape.color.w;
        }

        PadCurveTexture(out.curveTexels, out.curveTextureWidth, out.curveTextureHeight);
        PadBandTexture(out.bandTexels, out.bandTextureWidth, out.bandTextureHeight);
        return true;
    }
}
