// Copyright Chad Engler

#include "he/scribe/layout_engine.h"

#include "he/core/limits.h"
#include "he/core/log.h"
#include "he/core/utf8.h"
#include "he/core/utils.h"

#include <hb-ot.h>
#include <hb.h>

namespace he::scribe
{
    namespace
    {
        enum class ClusterDirection : uint8_t
        {
            LeftToRight,
            RightToLeft,
        };

        struct FontContext
        {
            hb_blob_t* blob{ nullptr };
            hb_face_t* face{ nullptr };
            hb_font_t* font{ nullptr };
            float unitScale{ 0.0f };
            float ascent{ 0.0f };
            float descent{ 0.0f };
            float lineHeight{ 0.0f };
            bool hasColorGlyphs{ false };
            bool hasSourceBytes{ false };
        };

        struct SourceCluster
        {
            uint32_t textByteStart{ 0 };
            uint32_t textByteEnd{ 0 };
            uint32_t fontFaceIndex{ 0 };
            hb_script_t script{ HB_SCRIPT_UNKNOWN };
            ClusterDirection direction{ ClusterDirection::LeftToRight };
            bool isWhitespace{ false };
        };

        struct RunInfo
        {
            uint32_t sourceClusterStart{ 0 };
            uint32_t sourceClusterCount{ 0 };
            uint32_t layoutClusterStart{ 0 };
            uint32_t layoutClusterCount{ 0 };
            uint32_t textByteStart{ 0 };
            uint32_t textByteEnd{ 0 };
            uint32_t fontFaceIndex{ 0 };
            uint32_t paragraphIndex{ 0 };
            hb_script_t script{ HB_SCRIPT_UNKNOWN };
            ClusterDirection direction{ ClusterDirection::LeftToRight };
        };

        struct GlyphClusterSpan
        {
            uint32_t clusterByteStart{ 0 };
            uint32_t glyphStart{ 0 };
            uint32_t glyphCount{ 0 };
            float advance{ 0.0f };
        };

        struct ParagraphInfo
        {
            uint32_t clusterStart{ 0 };
            uint32_t clusterCount{ 0 };
            float ascent{ 0.0f };
            float descent{ 0.0f };
            float lineHeight{ 0.0f };
            TextDirection direction{ TextDirection::LeftToRight };
        };

        class HbBufferOwner
        {
        public:
            HbBufferOwner() noexcept
                : m_buffer(hb_buffer_create())
            {}

            ~HbBufferOwner() noexcept
            {
                if (m_buffer)
                {
                    hb_buffer_destroy(m_buffer);
                }
            }

            HbBufferOwner(const HbBufferOwner&) = delete;
            HbBufferOwner& operator=(const HbBufferOwner&) = delete;

            hb_buffer_t* Get() const noexcept { return m_buffer; }
            bool IsValid() const noexcept { return (m_buffer != nullptr) && (m_buffer != hb_buffer_get_empty()); }

        private:
            hb_buffer_t* m_buffer{ nullptr };
        };

        bool IsStrongScript(hb_script_t script)
        {
            return (script != HB_SCRIPT_INVALID)
                && (script != HB_SCRIPT_UNKNOWN)
                && (script != HB_SCRIPT_COMMON)
                && (script != HB_SCRIPT_INHERITED);
        }

        bool IsVariationSelector(uint32_t codePoint)
        {
            return ((codePoint >= 0xFE00u) && (codePoint <= 0xFE0Fu))
                || ((codePoint >= 0xE0100u) && (codePoint <= 0xE01EFu));
        }

        bool IsEmojiModifier(uint32_t codePoint)
        {
            return (codePoint >= 0x1F3FBu) && (codePoint <= 0x1F3FFu);
        }

        bool IsLikelyEmojiCodePoint(uint32_t codePoint)
        {
            return ((codePoint >= 0x2600u) && (codePoint <= 0x27BFu))
                || ((codePoint >= 0x1F000u) && (codePoint <= 0x1FAFFu));
        }

        bool IsJoinControl(uint32_t codePoint)
        {
            return (codePoint == 0x200Cu) || (codePoint == 0x200Du);
        }

        bool IsClusterContinuation(hb_unicode_funcs_t* unicodeFuncs, uint32_t codePoint)
        {
            const hb_unicode_general_category_t category = hb_unicode_general_category(unicodeFuncs, codePoint);
            const hb_unicode_combining_class_t combiningClass = hb_unicode_combining_class(unicodeFuncs, codePoint);

            return (combiningClass != HB_UNICODE_COMBINING_CLASS_NOT_REORDERED)
                || (category == HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK)
                || (category == HB_UNICODE_GENERAL_CATEGORY_SPACING_MARK)
                || (category == HB_UNICODE_GENERAL_CATEGORY_ENCLOSING_MARK)
                || IsVariationSelector(codePoint)
                || IsEmojiModifier(codePoint)
                || IsJoinControl(codePoint);
        }

        bool IsCoverageIgnorable(uint32_t codePoint)
        {
            return IsVariationSelector(codePoint)
                || IsJoinControl(codePoint)
                || (codePoint == 0x00ADu);
        }

        bool ClusterPrefersColorGlyphs(StringView text)
        {
            bool sawEmojiCodePoint = false;

            for (UTF8Iterator it(text);; ++it)
            {
                const uint32_t codePoint = *it;
                if (codePoint == InvalidCodePoint)
                {
                    break;
                }

                if ((codePoint == 0xFE0Fu) || IsEmojiModifier(codePoint) || IsJoinControl(codePoint))
                {
                    return true;
                }

                if (IsLikelyEmojiCodePoint(codePoint))
                {
                    sawEmojiCodePoint = true;
                }
            }

            return sawEmojiCodePoint;
        }

        ClusterDirection ToClusterDirection(hb_script_t script)
        {
            return hb_script_get_horizontal_direction(script) == HB_DIRECTION_RTL
                ? ClusterDirection::RightToLeft
                : ClusterDirection::LeftToRight;
        }

        TextDirection ToTextDirection(ClusterDirection direction)
        {
            return direction == ClusterDirection::RightToLeft
                ? TextDirection::RightToLeft
                : TextDirection::LeftToRight;
        }

        hb_direction_t ToHbDirection(ClusterDirection direction)
        {
            return direction == ClusterDirection::RightToLeft
                ? HB_DIRECTION_RTL
                : HB_DIRECTION_LTR;
        }

        bool IsAllWhitespace(StringView text)
        {
            if (text.IsEmpty())
            {
                return false;
            }

            for (UTF8Iterator it(text);; ++it)
            {
                const uint32_t codePoint = *it;
                if (codePoint == InvalidCodePoint)
                {
                    break;
                }

                if (!UTF8IsWhitespace(codePoint))
                {
                    return false;
                }
            }

            return true;
        }

        bool BuildFontContexts(Vector<FontContext>& out, Span<const LoadedFontFaceBlob> faces, const LayoutOptions& options)
        {
            out.Clear();
            out.Resize(faces.Size());

            for (uint32_t i = 0; i < faces.Size(); ++i)
            {
                FontContext& ctx = out[i];
                const auto sourceBytes = faces[i].shaping.GetSourceBytes();
                const uint32_t unitsPerEm = Max(faces[i].metadata.GetMetrics().GetUnitsPerEm(), 1u);

                ctx.unitScale = options.fontSize / static_cast<float>(unitsPerEm);
                ctx.ascent = static_cast<float>(faces[i].metadata.GetMetrics().GetAscender()) * ctx.unitScale;
                ctx.descent = static_cast<float>(Abs(faces[i].metadata.GetMetrics().GetDescender())) * ctx.unitScale;
                ctx.lineHeight = static_cast<float>(faces[i].metadata.GetMetrics().GetLineHeight()) * ctx.unitScale;
                ctx.hasColorGlyphs = faces[i].metadata.IsValid() && faces[i].metadata.GetHasColorGlyphs();
                if (ctx.lineHeight <= 0.0f)
                {
                    ctx.lineHeight = ctx.ascent + ctx.descent;
                }

                if (sourceBytes.IsEmpty())
                {
                    continue;
                }

                ctx.blob = hb_blob_create(
                    reinterpret_cast<const char*>(sourceBytes.Data()),
                    static_cast<unsigned int>(sourceBytes.Size()),
                    HB_MEMORY_MODE_READONLY,
                    nullptr,
                    nullptr);
                if (!ctx.blob)
                {
                    return false;
                }

                ctx.face = hb_face_create(ctx.blob, faces[i].shaping.GetFaceIndex());
                ctx.font = hb_font_create(ctx.face);
                hb_ot_font_set_funcs(ctx.font);
                hb_font_set_scale(ctx.font, static_cast<int32_t>(unitsPerEm), static_cast<int32_t>(unitsPerEm));
                ctx.hasSourceBytes = true;
            }

            return true;
        }

        void DestroyFontContexts(Vector<FontContext>& contexts)
        {
            for (FontContext& ctx : contexts)
            {
                if (ctx.font)
                {
                    hb_font_destroy(ctx.font);
                }

                if (ctx.face)
                {
                    hb_face_destroy(ctx.face);
                }

                if (ctx.blob)
                {
                    hb_blob_destroy(ctx.blob);
                }

                ctx = {};
            }
        }

        uint32_t FindDefaultFaceIndex(Span<const FontContext> faces)
        {
            for (uint32_t i = 0; i < faces.Size(); ++i)
            {
                if (faces[i].hasSourceBytes)
                {
                    return i;
                }
            }

            return 0;
        }

        bool FaceHasClusterCoverage(const FontContext& face, StringView text)
        {
            if (!face.font || text.IsEmpty())
            {
                return false;
            }

            for (UTF8Iterator it(text);; ++it)
            {
                const uint32_t codePoint = *it;
                if (codePoint == InvalidCodePoint)
                {
                    break;
                }

                if (IsCoverageIgnorable(codePoint))
                {
                    continue;
                }

                hb_codepoint_t glyphId = 0;
                if (!hb_font_get_nominal_glyph(face.font, codePoint, &glyphId))
                {
                    return false;
                }
            }

            return true;
        }

        uint32_t ChooseClusterFaceIndex(
            Span<const FontContext> faces,
            StringView text,
            bool isWhitespace,
            uint32_t defaultFaceIndex,
            uint32_t previousFaceIndex)
        {
            if (isWhitespace)
            {
                if ((previousFaceIndex < faces.Size()) && faces[previousFaceIndex].hasSourceBytes)
                {
                    return previousFaceIndex;
                }

                return defaultFaceIndex;
            }

            if (ClusterPrefersColorGlyphs(text))
            {
                for (uint32_t i = 0; i < faces.Size(); ++i)
                {
                    if (faces[i].hasColorGlyphs && FaceHasClusterCoverage(faces[i], text))
                    {
                        return i;
                    }
                }
            }

            for (uint32_t i = 0; i < faces.Size(); ++i)
            {
                if (FaceHasClusterCoverage(faces[i], text))
                {
                    return i;
                }
            }

            return defaultFaceIndex;
        }

        TextDirection ResolveParagraphDirection(StringView paragraphText, const LayoutOptions& options)
        {
            if (options.direction != TextDirection::Auto)
            {
                return options.direction;
            }

            hb_unicode_funcs_t* unicodeFuncs = hb_unicode_funcs_get_default();
            for (UTF8Iterator it(paragraphText);; ++it)
            {
                const uint32_t codePoint = *it;
                if (codePoint == InvalidCodePoint)
                {
                    break;
                }

                const hb_script_t script = hb_unicode_script(unicodeFuncs, codePoint);
                if (IsStrongScript(script))
                {
                    return ToTextDirection(ToClusterDirection(script));
                }
            }

            return TextDirection::LeftToRight;
        }

        void BuildParagraphSourceClusters(
            Vector<SourceCluster>& out,
            StringView paragraphText,
            uint32_t paragraphByteOffset,
            TextDirection paragraphDirection)
        {
            out.Clear();

            hb_unicode_funcs_t* unicodeFuncs = hb_unicode_funcs_get_default();
            const char* cursor = paragraphText.Data();
            const char* end = paragraphText.End();
            bool previousWasJoiner = false;

            while (cursor != end)
            {
                uint32_t codePoint = InvalidCodePoint;
                const uint32_t byteCount = UTF8Decode(codePoint, cursor, Len(cursor, end));
                if ((codePoint == InvalidCodePoint) || (byteCount == 0))
                {
                    break;
                }

                const uint32_t relativeByteStart = Len(paragraphText.Begin(), cursor);
                const uint32_t relativeByteEnd = relativeByteStart + byteCount;
                const bool isWhitespace = UTF8IsWhitespace(codePoint);
                hb_script_t script = hb_unicode_script(unicodeFuncs, codePoint);

                const bool continueCluster = !out.IsEmpty()
                    && !isWhitespace
                    && !out.Back().isWhitespace
                    && (previousWasJoiner || IsClusterContinuation(unicodeFuncs, codePoint));

                if (continueCluster)
                {
                    SourceCluster& cluster = out.Back();
                    cluster.textByteEnd = paragraphByteOffset + relativeByteEnd;
                    if (IsStrongScript(script))
                    {
                        cluster.script = script;
                        cluster.direction = ToClusterDirection(script);
                    }
                }
                else
                {
                    SourceCluster& cluster = out.EmplaceBack();
                    cluster.textByteStart = paragraphByteOffset + relativeByteStart;
                    cluster.textByteEnd = paragraphByteOffset + relativeByteEnd;
                    cluster.script = script;
                    cluster.isWhitespace = isWhitespace;
                    cluster.direction = paragraphDirection == TextDirection::RightToLeft
                        ? ClusterDirection::RightToLeft
                        : ClusterDirection::LeftToRight;
                }

                previousWasJoiner = (codePoint == 0x200Du);
                cursor += byteCount;
            }

            hb_script_t nextStrongScript = HB_SCRIPT_UNKNOWN;
            for (uint32_t i = out.Size(); i > 0; --i)
            {
                SourceCluster& cluster = out[i - 1];
                if (!IsStrongScript(cluster.script))
                {
                    cluster.script = IsStrongScript(nextStrongScript) ? nextStrongScript : HB_SCRIPT_LATIN;
                }

                if (IsStrongScript(cluster.script))
                {
                    nextStrongScript = cluster.script;
                }
            }

            hb_script_t previousStrongScript = HB_SCRIPT_UNKNOWN;
            for (SourceCluster& cluster : out)
            {
                if (!IsStrongScript(cluster.script))
                {
                    cluster.script = IsStrongScript(previousStrongScript) ? previousStrongScript : HB_SCRIPT_LATIN;
                }

                if (IsStrongScript(cluster.script))
                {
                    previousStrongScript = cluster.script;
                    cluster.direction = ToClusterDirection(cluster.script);
                }
                else if (paragraphDirection != TextDirection::Auto)
                {
                    cluster.direction = paragraphDirection == TextDirection::RightToLeft
                        ? ClusterDirection::RightToLeft
                        : ClusterDirection::LeftToRight;
                }
            }
        }

        void AssignClusterFaces(Vector<SourceCluster>& clusters, Span<const FontContext> faces, StringView fullText)
        {
            const uint32_t defaultFaceIndex = FindDefaultFaceIndex(faces);
            uint32_t previousFaceIndex = defaultFaceIndex;
            for (SourceCluster& cluster : clusters)
            {
                const StringView clusterText = fullText.Substring(
                    cluster.textByteStart,
                    cluster.textByteEnd - cluster.textByteStart);
                cluster.fontFaceIndex = ChooseClusterFaceIndex(
                    faces,
                    clusterText,
                    cluster.isWhitespace,
                    defaultFaceIndex,
                    previousFaceIndex);
                previousFaceIndex = cluster.fontFaceIndex;
            }
        }

        void BuildRuns(Vector<RunInfo>& out, Span<const SourceCluster> clusters, uint32_t paragraphIndex)
        {
            out.Clear();
            if (clusters.IsEmpty())
            {
                return;
            }

            RunInfo current{};
            current.sourceClusterStart = 0;
            current.sourceClusterCount = 1;
            current.textByteStart = clusters[0].textByteStart;
            current.textByteEnd = clusters[0].textByteEnd;
            current.fontFaceIndex = clusters[0].fontFaceIndex;
            current.paragraphIndex = paragraphIndex;
            current.script = clusters[0].script;
            current.direction = clusters[0].direction;

            for (uint32_t i = 1; i < clusters.Size(); ++i)
            {
                const SourceCluster& cluster = clusters[i];
                if ((cluster.fontFaceIndex == current.fontFaceIndex)
                    && (cluster.script == current.script)
                    && (cluster.direction == current.direction))
                {
                    ++current.sourceClusterCount;
                    current.textByteEnd = cluster.textByteEnd;
                }
                else
                {
                    out.PushBack(current);
                    current = {};
                    current.sourceClusterStart = i;
                    current.sourceClusterCount = 1;
                    current.textByteStart = cluster.textByteStart;
                    current.textByteEnd = cluster.textByteEnd;
                    current.fontFaceIndex = cluster.fontFaceIndex;
                    current.paragraphIndex = paragraphIndex;
                    current.script = cluster.script;
                    current.direction = cluster.direction;
                }
            }

            out.PushBack(current);
        }

        uint32_t FindClusterTextEnd(Span<const SourceCluster> sourceClusters, uint32_t clusterByteStart, uint32_t defaultTextEnd)
        {
            for (uint32_t i = 0; i < sourceClusters.Size(); ++i)
            {
                const SourceCluster& sourceCluster = sourceClusters[i];
                if (sourceCluster.textByteStart == clusterByteStart)
                {
                    for (uint32_t j = i + 1; j < sourceClusters.Size(); ++j)
                    {
                        if (sourceClusters[j].textByteStart > clusterByteStart)
                        {
                            return sourceClusters[j].textByteStart;
                        }
                    }

                    return defaultTextEnd;
                }
            }

            return defaultTextEnd;
        }

        bool ShapeRun(
            LayoutResult& out,
            RunInfo& run,
            Span<const SourceCluster> runSourceClusters,
            Span<const FontContext> faces,
            StringView fullText)
        {
            if (run.fontFaceIndex >= faces.Size())
            {
                return false;
            }

            const FontContext& face = faces[run.fontFaceIndex];
            if (!face.font)
            {
                return false;
            }

            HbBufferOwner buffer;
            if (!buffer.IsValid())
            {
                return false;
            }

            hb_buffer_set_cluster_level(buffer.Get(), HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS);
            hb_buffer_set_direction(buffer.Get(), ToHbDirection(run.direction));
            hb_buffer_set_script(buffer.Get(), run.script);
            hb_buffer_add_utf8(
                buffer.Get(),
                fullText.Data(),
                static_cast<int32_t>(fullText.Size()),
                static_cast<unsigned int>(run.textByteStart),
                static_cast<int32_t>(run.textByteEnd - run.textByteStart));
            hb_shape(face.font, buffer.Get(), nullptr, 0);

            unsigned int glyphCount = 0;
            const hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buffer.Get(), &glyphCount);
            const hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buffer.Get(), &glyphCount);
            if (!infos || !positions)
            {
                return false;
            }

            Vector<GlyphClusterSpan> glyphClusterSpans{};

            run.layoutClusterStart = out.clusters.Size();

            const uint32_t glyphStartIndex = out.glyphs.Size();
            out.glyphs.Reserve(out.glyphs.Size() + glyphCount);

            for (uint32_t i = 0; i < glyphCount; ++i)
            {
                const hb_glyph_info_t& info = infos[i];
                const hb_glyph_position_t& position = positions[i];

                ShapedGlyph& glyph = out.glyphs.EmplaceBack();
                glyph.glyphIndex = info.codepoint;
                glyph.fontFaceIndex = run.fontFaceIndex;
                glyph.textByteStart = info.cluster;
                glyph.textByteEnd = FindClusterTextEnd(runSourceClusters, info.cluster, run.textByteEnd);
                glyph.offset = {
                    position.x_offset * face.unitScale,
                    -position.y_offset * face.unitScale
                };
                glyph.advance = {
                    position.x_advance * face.unitScale,
                    position.y_advance * face.unitScale
                };

                if (!glyphClusterSpans.IsEmpty() && (glyphClusterSpans.Back().clusterByteStart == info.cluster))
                {
                    GlyphClusterSpan& span = glyphClusterSpans.Back();
                    ++span.glyphCount;
                    span.advance += glyph.advance.x;
                }
                else
                {
                    GlyphClusterSpan& span = glyphClusterSpans.EmplaceBack();
                    span.clusterByteStart = info.cluster;
                    span.glyphStart = glyphStartIndex + i;
                    span.glyphCount = 1;
                    span.advance = glyph.advance.x;
                }
            }

            for (uint32_t sourceClusterIndex = 0; sourceClusterIndex < runSourceClusters.Size(); ++sourceClusterIndex)
            {
                const SourceCluster& sourceCluster = runSourceClusters[sourceClusterIndex];
                for (uint32_t spanIndex = 0; spanIndex < glyphClusterSpans.Size(); ++spanIndex)
                {
                    const GlyphClusterSpan& span = glyphClusterSpans[spanIndex];
                    if (span.clusterByteStart != sourceCluster.textByteStart)
                    {
                        continue;
                    }

                    TextCluster& cluster = out.clusters.EmplaceBack();
                    cluster.textByteStart = sourceCluster.textByteStart;
                    cluster.textByteEnd = FindClusterTextEnd(
                        runSourceClusters,
                        sourceCluster.textByteStart,
                        run.textByteEnd);
                    cluster.glyphStart = span.glyphStart;
                    cluster.glyphCount = span.glyphCount;
                    cluster.fontFaceIndex = run.fontFaceIndex;
                    cluster.advance = span.advance;
                    cluster.isWhitespace = sourceCluster.isWhitespace || IsAllWhitespace(fullText.Substring(
                        cluster.textByteStart,
                        cluster.textByteEnd - cluster.textByteStart));
                    break;
                }
            }

            run.layoutClusterCount = out.clusters.Size() - run.layoutClusterStart;
            return true;
        }

        void BuildLine(
            LayoutResult& out,
            const ParagraphInfo& paragraph,
            Span<const RunInfo> runs,
            uint32_t lineClusterStart,
            uint32_t lineClusterEnd,
            float& yCursor)
        {
            TextLine& line = out.lines.EmplaceBack();
            line.clusterStart = lineClusterStart;
            line.clusterCount = lineClusterEnd - lineClusterStart;
            line.ascent = paragraph.ascent;
            line.descent = paragraph.descent;
            line.height = Max(paragraph.lineHeight, paragraph.ascent + paragraph.descent);
            line.baselineY = yCursor + line.ascent;
            line.direction = paragraph.direction;

            for (uint32_t clusterIndex = lineClusterStart; clusterIndex < lineClusterEnd; ++clusterIndex)
            {
                line.width += out.clusters[clusterIndex].advance;
                line.glyphCount += out.clusters[clusterIndex].glyphCount;
            }

            if (line.clusterCount > 0)
            {
                line.glyphStart = out.clusters[lineClusterStart].glyphStart;
            }

            struct Slice
            {
                uint32_t start{ 0 };
                uint32_t end{ 0 };
                uint32_t runIndex{ 0 };
            };

            Vector<Slice> slices{};
            uint32_t clusterIndex = lineClusterStart;
            while (clusterIndex < lineClusterEnd)
            {
                uint32_t matchingRunIndex = 0;
                bool foundRun = false;
                for (uint32_t runIndex = 0; runIndex < runs.Size(); ++runIndex)
                {
                    const RunInfo& run = runs[runIndex];
                    const uint32_t runStart = run.layoutClusterStart;
                    const uint32_t runEnd = runStart + run.layoutClusterCount;
                    if ((clusterIndex >= runStart) && (clusterIndex < runEnd))
                    {
                        matchingRunIndex = runIndex;
                        foundRun = true;
                        break;
                    }
                }

                if (!foundRun)
                {
                    ++clusterIndex;
                    continue;
                }

                Slice& slice = slices.EmplaceBack();
                slice.start = clusterIndex;
                slice.runIndex = matchingRunIndex;
                ++clusterIndex;

                while (clusterIndex < lineClusterEnd)
                {
                    const RunInfo& run = runs[matchingRunIndex];
                    const uint32_t runStart = run.layoutClusterStart;
                    const uint32_t runEnd = runStart + run.layoutClusterCount;
                    if ((clusterIndex < runStart) || (clusterIndex >= runEnd))
                    {
                        break;
                    }

                    ++clusterIndex;
                }

                slice.end = clusterIndex;
            }

            float cursor = paragraph.direction == TextDirection::RightToLeft ? line.width : 0.0f;
            for (uint32_t sliceIter = 0; sliceIter < slices.Size(); ++sliceIter)
            {
                const uint32_t sliceIndex = paragraph.direction == TextDirection::RightToLeft
                    ? (slices.Size() - 1 - sliceIter)
                    : sliceIter;
                const Slice& slice = slices[sliceIndex];
                const RunInfo& run = runs[slice.runIndex];
                const bool isRtlRun = run.direction == ClusterDirection::RightToLeft;

                for (uint32_t localIndex = 0; localIndex < (slice.end - slice.start); ++localIndex)
                {
                    const uint32_t placedClusterIndex = isRtlRun
                        ? (slice.end - 1 - localIndex)
                        : (slice.start + localIndex);

                    TextCluster& cluster = out.clusters[placedClusterIndex];
                    if (paragraph.direction == TextDirection::RightToLeft)
                    {
                        cluster.x1 = cursor;
                        cluster.x0 = cluster.x1 - cluster.advance;
                        cursor = cluster.x0;
                    }
                    else
                    {
                        cluster.x0 = cursor;
                        cluster.x1 = cluster.x0 + cluster.advance;
                        cursor = cluster.x1;
                    }

                    cluster.lineIndex = out.lines.Size() - 1;

                    if (isRtlRun)
                    {
                        float glyphCursor = cluster.x1;
                        for (uint32_t glyphLocalIndex = 0; glyphLocalIndex < cluster.glyphCount; ++glyphLocalIndex)
                        {
                            ShapedGlyph& glyph = out.glyphs[cluster.glyphStart + glyphLocalIndex];
                            glyphCursor -= glyph.advance.x;
                            glyph.position = { glyphCursor + glyph.offset.x, line.baselineY + glyph.offset.y };
                            glyph.clusterIndex = placedClusterIndex;
                            glyph.lineIndex = out.lines.Size() - 1;
                        }
                    }
                    else
                    {
                        float glyphCursor = cluster.x0;
                        for (uint32_t glyphLocalIndex = 0; glyphLocalIndex < cluster.glyphCount; ++glyphLocalIndex)
                        {
                            ShapedGlyph& glyph = out.glyphs[cluster.glyphStart + glyphLocalIndex];
                            glyph.position = { glyphCursor + glyph.offset.x, line.baselineY + glyph.offset.y };
                            glyph.clusterIndex = placedClusterIndex;
                            glyph.lineIndex = out.lines.Size() - 1;
                            glyphCursor += glyph.advance.x;
                        }
                    }
                }
            }

            yCursor += line.height;
            out.width = Max(out.width, line.width);
            out.height = yCursor;
        }
    }

    void LayoutResult::Clear()
    {
        glyphs.Clear();
        clusters.Clear();
        lines.Clear();
        width = 0.0f;
        height = 0.0f;
        missingGlyphCount = 0;
        fallbackGlyphCount = 0;
    }

    bool LayoutEngine::LayoutText(
        LayoutResult& out,
        Span<const LoadedFontFaceBlob> faces,
        StringView text,
        const LayoutOptions& options) const
    {
        out.Clear();

        if (faces.IsEmpty() || !UTF8Validate(text.Data(), text.Size()))
        {
            return false;
        }

        Vector<FontContext> fontContexts{};
        if (!BuildFontContexts(fontContexts, faces, options))
        {
            DestroyFontContexts(fontContexts);
            return false;
        }

        const uint32_t defaultFaceIndex = FindDefaultFaceIndex(fontContexts);
        if ((defaultFaceIndex >= fontContexts.Size()) || !fontContexts[defaultFaceIndex].hasSourceBytes)
        {
            DestroyFontContexts(fontContexts);
            return false;
        }

        Vector<ParagraphInfo> paragraphs{};
        Vector<RunInfo> allRuns{};
        Vector<SourceCluster> sourceClusters{};
        Vector<RunInfo> runs{};

        uint32_t paragraphStart = 0;
        while (paragraphStart <= text.Size())
        {
            uint32_t paragraphEnd = paragraphStart;
            while ((paragraphEnd < text.Size()) && (text[paragraphEnd] != '\n'))
            {
                ++paragraphEnd;
            }

            uint32_t trimmedParagraphEnd = paragraphEnd;
            if ((trimmedParagraphEnd > paragraphStart) && (text[trimmedParagraphEnd - 1] == '\r'))
            {
                --trimmedParagraphEnd;
            }

            const char* paragraphData = (paragraphStart < text.Size())
                ? (text.Data() + paragraphStart)
                : "";
            const StringView paragraphText{ paragraphData, trimmedParagraphEnd - paragraphStart };
            ParagraphInfo& paragraph = paragraphs.EmplaceBack();
            paragraph.clusterStart = out.clusters.Size();
            paragraph.direction = ResolveParagraphDirection(paragraphText, options);
            paragraph.ascent = fontContexts[defaultFaceIndex].ascent;
            paragraph.descent = fontContexts[defaultFaceIndex].descent;
            paragraph.lineHeight = fontContexts[defaultFaceIndex].lineHeight * Max(options.lineHeightScale, 0.01f);

            if (!paragraphText.IsEmpty())
            {
                BuildParagraphSourceClusters(sourceClusters, paragraphText, paragraphStart, paragraph.direction);
                AssignClusterFaces(sourceClusters, fontContexts, text);
                BuildRuns(runs, sourceClusters, paragraphs.Size() - 1);

                for (const SourceCluster& cluster : sourceClusters)
                {
                    if (cluster.fontFaceIndex != defaultFaceIndex)
                    {
                        ++out.fallbackGlyphCount;
                    }

                    if (cluster.fontFaceIndex < fontContexts.Size())
                    {
                        paragraph.ascent = Max(paragraph.ascent, fontContexts[cluster.fontFaceIndex].ascent);
                        paragraph.descent = Max(paragraph.descent, fontContexts[cluster.fontFaceIndex].descent);
                        paragraph.lineHeight = Max(
                            paragraph.lineHeight,
                            fontContexts[cluster.fontFaceIndex].lineHeight * Max(options.lineHeightScale, 0.01f));
                    }
                }

                const uint32_t paragraphRunStart = allRuns.Size();
                for (uint32_t runIndex = 0; runIndex < runs.Size(); ++runIndex)
                {
                    allRuns.PushBack(runs[runIndex]);
                    RunInfo& run = allRuns.Back();
                    if (!ShapeRun(
                        out,
                        run,
                        Span<const SourceCluster>(sourceClusters.Data() + run.sourceClusterStart, run.sourceClusterCount),
                        fontContexts,
                        text))
                    {
                        DestroyFontContexts(fontContexts);
                        return false;
                    }
                }

                paragraph.clusterCount = out.clusters.Size() - paragraph.clusterStart;

                const uint32_t paragraphRunEnd = allRuns.Size();
                uint32_t lineStart = paragraph.clusterStart;
                uint32_t current = paragraph.clusterStart;
                float width = 0.0f;
                uint32_t lastWhitespaceBreak = Limits<uint32_t>::Max;

                while (current < (paragraph.clusterStart + paragraph.clusterCount))
                {
                    const TextCluster& cluster = out.clusters[current];
                    const bool exceedsWidth = options.wrap
                        && (options.maxWidth > 0.0f)
                        && (width > 0.0f)
                        && ((width + cluster.advance) > options.maxWidth);

                    if (exceedsWidth)
                    {
                        const uint32_t breakCluster = (lastWhitespaceBreak != Limits<uint32_t>::Max)
                            && (lastWhitespaceBreak >= lineStart)
                            ? (lastWhitespaceBreak + 1)
                            : current;

                        BuildLine(
                            out,
                            paragraph,
                            Span<const RunInfo>(allRuns.Data() + paragraphRunStart, paragraphRunEnd - paragraphRunStart),
                            lineStart,
                            breakCluster,
                            out.height);

                        lineStart = breakCluster;
                        current = breakCluster;
                        width = 0.0f;
                        lastWhitespaceBreak = Limits<uint32_t>::Max;
                        continue;
                    }

                    width += cluster.advance;
                    if (cluster.isWhitespace)
                    {
                        lastWhitespaceBreak = current;
                    }

                    ++current;
                }

                BuildLine(
                    out,
                    paragraph,
                    Span<const RunInfo>(allRuns.Data() + paragraphRunStart, paragraphRunEnd - paragraphRunStart),
                    lineStart,
                    paragraph.clusterStart + paragraph.clusterCount,
                    out.height);
            }
            else
            {
                paragraph.clusterCount = 0;
                BuildLine(out, paragraph, {}, 0, 0, out.height);
            }

            if (paragraphEnd == text.Size())
            {
                break;
            }

            paragraphStart = paragraphEnd + 1;
        }

        for (const ShapedGlyph& glyph : out.glyphs)
        {
            if ((glyph.glyphIndex == 0) && !out.clusters[glyph.clusterIndex].isWhitespace)
            {
                ++out.missingGlyphCount;
            }
        }

        DestroyFontContexts(fontContexts);
        return true;
    }

    bool LayoutEngine::HitTest(const LayoutResult& layout, const Vec2f& point, HitTestResult& out) const
    {
        out = {};

        if (layout.lines.IsEmpty())
        {
            return false;
        }

        uint32_t lineIndex = layout.lines.Size() - 1;
        for (uint32_t i = 0; i < layout.lines.Size(); ++i)
        {
            const TextLine& line = layout.lines[i];
            const float top = line.baselineY - line.ascent;
            const float bottom = line.baselineY + line.descent;
            if ((point.y >= top) && (point.y <= bottom))
            {
                lineIndex = i;
                out.isInside = true;
                break;
            }

            if (point.y < top)
            {
                lineIndex = i;
                break;
            }
        }

        const TextLine& line = layout.lines[lineIndex];
        out.lineIndex = lineIndex;

        if (line.clusterCount == 0)
        {
            out.clusterIndex = line.clusterStart;
            out.textByteIndex = 0;
            out.caretX = 0.0f;
            return true;
        }

        uint32_t clusterIndex = line.clusterStart;
        for (uint32_t i = 0; i < line.clusterCount; ++i)
        {
            const TextCluster& cluster = layout.clusters[line.clusterStart + i];
            const float midpoint = (cluster.x0 + cluster.x1) * 0.5f;
            if (point.x <= midpoint)
            {
                clusterIndex = line.clusterStart + i;
                break;
            }

            clusterIndex = line.clusterStart + i;
        }

        const TextCluster& cluster = layout.clusters[clusterIndex];
        const float midpoint = (cluster.x0 + cluster.x1) * 0.5f;

        out.clusterIndex = clusterIndex;
        out.isTrailingEdge = point.x > midpoint;
        out.textByteIndex = out.isTrailingEdge ? cluster.textByteEnd : cluster.textByteStart;
        out.caretX = out.isTrailingEdge ? cluster.x1 : cluster.x0;
        return true;
    }
}
