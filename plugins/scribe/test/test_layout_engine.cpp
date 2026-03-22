// Copyright Chad Engler

#include "he/scribe/layout_engine.h"
#include "he/scribe/packed_data.h"
#include "he/scribe/runtime_blob.h"

#include "he/core/file.h"
#include "he/core/test.h"

using namespace he;
using namespace he::scribe;

namespace
{
    constexpr const char* TestIconAccount = "\xf3\xb0\x80\x84";

    bool ResolveFontPath(String& out, const char* fileName)
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

    bool BuildLoadedFontFaceFromFile(
        const char* fileName,
        Vector<schema::Word>& storage,
        LoadedFontFaceBlob& out,
        bool hasColorGlyphs = false)
    {
        String path;
        if (!ResolveFontPath(path, fileName))
        {
            return false;
        }

        Vector<uint8_t> fontBytes;
        if (!File::ReadAll(fontBytes, path.Data()))
        {
            return false;
        }

        schema::Builder shapingBuilder;
        FontFaceShapingData::Builder shaping = shapingBuilder.AddStruct<FontFaceShapingData>();
        shaping.SetFaceIndex(0);
        shaping.SetSourceFormat(FontSourceFormat::TrueType);
        shaping.SetSourceBytes(shapingBuilder.AddBlob(Span<const uint8_t>(fontBytes)));
        shapingBuilder.SetRoot(shaping);

        schema::Builder metadataBuilder;
        FontFaceImportMetadata::Builder metadata = metadataBuilder.AddStruct<FontFaceImportMetadata>();
        metadata.SetFaceIndex(0);
        metadata.SetSourceFormat(FontSourceFormat::TrueType);
        metadata.InitFamilyName(fileName);
        metadata.InitStyleName("Regular");
        metadata.InitPostscriptName(fileName);
        metadata.SetGlyphCount(0);
        metadata.SetIsScalable(true);
        metadata.SetHasColorGlyphs(hasColorGlyphs);
        metadata.SetHasKerning(true);
        metadata.SetHasHorizontalLayout(true);
        metadata.SetHasVerticalLayout(false);

        FontFaceMetrics::Builder metrics = metadata.InitMetrics();
        metrics.SetUnitsPerEm(1000);
        metrics.SetAscender(800);
        metrics.SetDescender(-200);
        metrics.SetLineHeight(1200);
        metrics.SetMaxAdvanceWidth(1200);
        metrics.SetMaxAdvanceHeight(1200);
        metrics.SetCapHeight(700);
        metadataBuilder.SetRoot(metadata);

        schema::Builder renderBuilder;
        FontFaceRenderData::Builder render = renderBuilder.AddStruct<FontFaceRenderData>();
        render.SetCurveTextureWidth(1);
        render.SetCurveTextureHeight(1);
        render.SetBandTextureWidth(ScribeBandTextureWidth);
        render.SetBandTextureHeight(1);
        render.SetBandOverlapEpsilon(1.0f);
        render.InitGlyphs(0);
        renderBuilder.SetRoot(render);

        static const PackedCurveTexel CurveTexel = PackCurveTexel(0.0f, 0.0f, 0.0f, 0.0f);
        static PackedBandTexel BandTexels[ScribeBandTextureWidth]{};

        schema::Builder paintBuilder;
        FontFacePaintData::Builder paint = paintBuilder.AddStruct<FontFacePaintData>();
        paint.SetDefaultPaletteIndex(0);
        paint.InitPalettes(0);
        paint.InitColorGlyphs(0);
        paint.InitLayers(0);
        paintBuilder.SetRoot(paint);

        schema::Builder rootBuilder;
        CompiledFontFaceBlob::Builder root = rootBuilder.AddStruct<CompiledFontFaceBlob>();
        RuntimeBlobHeader::Builder header = root.InitHeader();
        header.SetFormatVersion(RuntimeBlobFormatVersion);
        header.SetKind(RuntimeBlobKind::FontFace);
        header.SetFlags(0);
        root.SetShapingData(rootBuilder.AddBlob(Span<const schema::Word>(shapingBuilder).AsBytes()));
        root.SetCurveData(rootBuilder.AddBlob(Span<const PackedCurveTexel>(&CurveTexel, 1).AsBytes()));
        root.SetBandData(rootBuilder.AddBlob(Span<const PackedBandTexel>(BandTexels).AsBytes()));
        root.SetPaintData(rootBuilder.AddBlob(Span<const schema::Word>(paintBuilder).AsBytes()));
        root.SetMetadataData(rootBuilder.AddBlob(Span<const schema::Word>(metadataBuilder).AsBytes()));
        root.SetRenderData(rootBuilder.AddBlob(Span<const schema::Word>(renderBuilder).AsBytes()));
        rootBuilder.SetRoot(root);

        storage = Span<const schema::Word>(rootBuilder);
        return LoadCompiledFontFaceBlob(out, storage);
    }
}

HE_TEST(scribe, layout_engine, shape_combining_cluster)
{
    Vector<schema::Word> fontStorage;
    LoadedFontFaceBlob font{};
    HE_ASSERT(BuildLoadedFontFaceFromFile("NotoSans-Regular.ttf", fontStorage, font));

    LayoutEngine engine;
    LayoutResult layout;
    const String text = "A\xCC\x81 cafe";

    HE_EXPECT(engine.LayoutText(layout, Span<const LoadedFontFaceBlob>(&font, 1), text));
    HE_EXPECT_EQ(layout.lines.Size(), 1u);
    HE_EXPECT_GE(layout.glyphs.Size(), 1u);
    HE_EXPECT_GE(layout.clusters.Size(), 1u);
    HE_EXPECT_EQ(layout.clusters[0].textByteStart, 0u);
    HE_EXPECT_EQ(layout.clusters[0].textByteEnd, 3u);
}

HE_TEST(scribe, layout_engine, uses_fallback_face)
{
    Vector<schema::Word> primaryStorage;
    Vector<schema::Word> fallbackStorage;
    LoadedFontFaceBlob primary{};
    LoadedFontFaceBlob fallback{};
    HE_ASSERT(BuildLoadedFontFaceFromFile("NotoSans-Regular.ttf", primaryStorage, primary));
    HE_ASSERT(BuildLoadedFontFaceFromFile("materialdesignicons.ttf", fallbackStorage, fallback));

    const LoadedFontFaceBlob faces[] = { primary, fallback };

    LayoutEngine engine;
    LayoutResult layout;
    String text = "A";
    text += TestIconAccount;
    text += "B";

    HE_EXPECT(engine.LayoutText(layout, faces, text));
    HE_EXPECT_GT(layout.fallbackGlyphCount, 0u);

    bool sawFallbackCluster = false;
    for (const TextCluster& cluster : layout.clusters)
    {
        if (cluster.fontFaceIndex == 1)
        {
            sawFallbackCluster = true;
            break;
        }
    }

    HE_EXPECT(sawFallbackCluster);
}

HE_TEST(scribe, layout_engine, prefers_color_face_for_emoji_clusters)
{
    Vector<schema::Word> primaryStorage;
    Vector<schema::Word> colorStorage;
    LoadedFontFaceBlob primary{};
    LoadedFontFaceBlob colorFace{};
    HE_ASSERT(BuildLoadedFontFaceFromFile("NotoSans-Regular.ttf", primaryStorage, primary, false));
    HE_ASSERT(BuildLoadedFontFaceFromFile("NotoSans-Regular.ttf", colorStorage, colorFace, true));

    const LoadedFontFaceBlob faces[] = { primary, colorFace };

    LayoutEngine engine;
    LayoutResult layout;
    const String text = "A\xEF\xB8\x8F";

    HE_EXPECT(engine.LayoutText(layout, faces, text));
    HE_EXPECT_EQ(layout.clusters.Size(), 1u);
    HE_EXPECT_EQ(layout.clusters[0].fontFaceIndex, 1u);
    HE_EXPECT_GT(layout.fallbackGlyphCount, 0u);
}

HE_TEST(scribe, layout_engine, wraps_and_hit_tests)
{
    Vector<schema::Word> fontStorage;
    LoadedFontFaceBlob font{};
    HE_ASSERT(BuildLoadedFontFaceFromFile("NotoSans-Regular.ttf", fontStorage, font));

    LayoutEngine engine;
    LayoutResult layout;
    LayoutOptions options{};
    options.maxWidth = 35.0f;
    options.wrap = true;

    const String text = "wrap wrap wrap";
    HE_EXPECT(engine.LayoutText(layout, Span<const LoadedFontFaceBlob>(&font, 1), text, options));
    HE_EXPECT_GT(layout.lines.Size(), 1u);

    HitTestResult hit{};
    HE_EXPECT(engine.HitTest(layout, { 0.0f, layout.lines[0].baselineY }, hit));
    HE_EXPECT_EQ(hit.lineIndex, 0u);
    HE_EXPECT_EQ(hit.clusterIndex, layout.lines[0].clusterStart);
    HE_EXPECT(!hit.isTrailingEdge);

    HE_EXPECT(engine.HitTest(layout, { layout.lines[0].width + 100.0f, layout.lines[0].baselineY }, hit));
    HE_EXPECT_EQ(hit.lineIndex, 0u);
    HE_EXPECT(hit.isTrailingEdge);
}

HE_TEST(scribe, layout_engine, rtl_line_direction)
{
    Vector<schema::Word> fontStorage;
    LoadedFontFaceBlob font{};
    HE_ASSERT(BuildLoadedFontFaceFromFile("NotoSans-Regular.ttf", fontStorage, font));

    LayoutEngine engine;
    LayoutResult layout;
    LayoutOptions options{};
    options.direction = TextDirection::RightToLeft;

    const String text = "abc";
    HE_EXPECT(engine.LayoutText(layout, Span<const LoadedFontFaceBlob>(&font, 1), text, options));
    HE_EXPECT_EQ(layout.lines.Size(), 1u);
    HE_EXPECT_GE(layout.clusters.Size(), 3u);
    HE_EXPECT_GT(layout.clusters[0].x0, layout.clusters[layout.clusters.Size() - 1].x0);
}
