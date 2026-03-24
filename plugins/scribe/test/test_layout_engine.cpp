// Copyright Chad Engler

#include "he/scribe/layout_engine.h"
#include "he/scribe/packed_data.h"
#include "he/scribe/schema_types.h"

#include "he/core/file.h"
#include "he/core/test.h"

using namespace he;
using namespace he::scribe;

namespace
{
    constexpr const char* TestIconAccount = "\xf3\xb0\x80\x84";
    constexpr const char* FeatureFontCandidates[] =
    {
        "C:/Windows/Fonts/cambria.ttc",
        "C:/Windows/Fonts/calibri.ttf",
        "C:/Windows/Fonts/Candara.ttf",
        "C:/Windows/Fonts/SitkaVF.ttf",
        "C:/Windows/Fonts/Gabriola.ttf",
    };

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
        FontFaceResourceReader& out,
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

        static const PackedCurveTexel CurveTexel = PackCurveTexel(0.0f, 0.0f, 0.0f, 0.0f);
        static PackedBandTexel BandTexels[ScribeBandTextureWidth]{};

        schema::Builder rootBuilder;
        FontFaceResource::Builder root = rootBuilder.AddStruct<FontFaceResource>();
        FontFaceShapingData::Builder shaping = root.GetShaping();
        shaping.SetFaceIndex(0);
        shaping.SetSourceBytes(rootBuilder.AddBlob(Span<const uint8_t>(fontBytes)));

        FontFaceRuntimeMetadata::Builder metadata = root.GetMetadata();
        metadata.SetGlyphCount(0);
        metadata.SetUnitsPerEm(1000);
        metadata.SetAscender(800);
        metadata.SetDescender(-200);
        metadata.SetLineHeight(1200);
        metadata.SetCapHeight(700);
        metadata.SetHasColorGlyphs(hasColorGlyphs);

        FontFaceRenderData::Builder render = root.GetRender();
        render.SetCurveTextureWidth(1);
        render.SetCurveTextureHeight(1);
        render.SetBandTextureWidth(ScribeBandTextureWidth);
        render.SetBandTextureHeight(1);
        render.SetBandOverlapEpsilon(1.0f);
        render.InitGlyphs(0);

        FontFacePaintData::Builder paint = root.GetPaint();
        paint.SetDefaultPaletteIndex(0);
        paint.InitPalettes(0);
        paint.InitColorGlyphs(0);
        paint.InitLayers(0);

        root.SetCurveData(rootBuilder.AddBlob(Span<const PackedCurveTexel>(&CurveTexel, 1).AsBytes()));
        root.SetBandData(rootBuilder.AddBlob(Span<const PackedBandTexel>(BandTexels).AsBytes()));
        rootBuilder.SetRoot(root);

        storage = Span<const schema::Word>(rootBuilder);
        out = schema::ReadRoot<FontFaceResource>(storage.Data());
        return out.IsValid();
    }

    bool BuildLoadedFontFaceFromCandidates(
        Span<const char* const> candidates,
        Vector<schema::Word>& storage,
        FontFaceResourceReader& out)
    {
        for (const char* candidate : candidates)
        {
            if (BuildLoadedFontFaceFromFile(candidate, storage, out))
            {
                return true;
            }
        }

        return false;
    }

    bool GlyphSequenceDiffers(const LayoutResult& a, const LayoutResult& b)
    {
        if (a.glyphs.Size() != b.glyphs.Size())
        {
            return true;
        }

        for (uint32_t glyphIndex = 0; glyphIndex < a.glyphs.Size(); ++glyphIndex)
        {
            const ShapedGlyph& lhs = a.glyphs[glyphIndex];
            const ShapedGlyph& rhs = b.glyphs[glyphIndex];
            if ((lhs.glyphIndex != rhs.glyphIndex)
                || (lhs.fontFaceIndex != rhs.fontFaceIndex)
                || (lhs.position.x != rhs.position.x)
                || (lhs.advance.x != rhs.advance.x))
            {
                return true;
            }
        }

        return false;
    }
}

HE_TEST(scribe, layout_engine, shape_combining_cluster)
{
    Vector<schema::Word> fontStorage;
    FontFaceResourceReader font{};
    HE_ASSERT(BuildLoadedFontFaceFromFile("NotoSans-Regular.ttf", fontStorage, font));

    LayoutEngine engine;
    LayoutResult layout;
    const String text = "A\xCC\x81 cafe";

    HE_EXPECT(engine.LayoutText(layout, Span<const FontFaceResourceReader>(&font, 1), text));
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
    FontFaceResourceReader primary{};
    FontFaceResourceReader fallback{};
    HE_ASSERT(BuildLoadedFontFaceFromFile("NotoSans-Regular.ttf", primaryStorage, primary));
    HE_ASSERT(BuildLoadedFontFaceFromFile("materialdesignicons.ttf", fallbackStorage, fallback));

    const FontFaceResourceReader faces[] = { primary, fallback };

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
    FontFaceResourceReader primary{};
    FontFaceResourceReader colorFace{};
    HE_ASSERT(BuildLoadedFontFaceFromFile("NotoSans-Regular.ttf", primaryStorage, primary, false));
    HE_ASSERT(BuildLoadedFontFaceFromFile("NotoSans-Regular.ttf", colorStorage, colorFace, true));

    const FontFaceResourceReader faces[] = { primary, colorFace };

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
    FontFaceResourceReader font{};
    HE_ASSERT(BuildLoadedFontFaceFromFile("NotoSans-Regular.ttf", fontStorage, font));

    LayoutEngine engine;
    LayoutResult layout;
    LayoutOptions options{};
    options.maxWidth = 35.0f;
    options.wrap = true;

    const String text = "wrap wrap wrap";
    HE_EXPECT(engine.LayoutText(layout, Span<const FontFaceResourceReader>(&font, 1), text, options));
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
    FontFaceResourceReader font{};
    HE_ASSERT(BuildLoadedFontFaceFromFile("NotoSans-Regular.ttf", fontStorage, font));

    LayoutEngine engine;
    LayoutResult layout;
    LayoutOptions options{};
    options.direction = TextDirection::RightToLeft;

    const String text = "abc";
    HE_EXPECT(engine.LayoutText(layout, Span<const FontFaceResourceReader>(&font, 1), text, options));
    HE_EXPECT_EQ(layout.lines.Size(), 1u);
    HE_EXPECT_GE(layout.clusters.Size(), 3u);
    HE_EXPECT_GT(layout.clusters[0].x0, layout.clusters[layout.clusters.Size() - 1].x0);
}

HE_TEST(scribe, layout_engine, trailing_newline_paragraph)
{
    Vector<schema::Word> fontStorage;
    FontFaceResourceReader font{};
    HE_ASSERT(BuildLoadedFontFaceFromFile("NotoSans-Regular.ttf", fontStorage, font));

    LayoutEngine engine;
    LayoutResult layout;
    const String text = "emoji page line\n";

    HE_EXPECT(engine.LayoutText(layout, Span<const FontFaceResourceReader>(&font, 1), text));
    HE_EXPECT_GE(layout.lines.Size(), 2u);
    HE_EXPECT_GE(layout.clusters.Size(), 1u);
}

HE_TEST(scribe, layout_engine, textsub_demo_has_no_hidden_line_start_glyphs)
{
    Vector<schema::Word> fontStorage;
    FontFaceResourceReader font{};
    HE_ASSERT(BuildLoadedFontFaceFromFile("NotoSans-Regular.ttf", fontStorage, font));

    LayoutEngine engine;
    LayoutResult layout;
    const String text = "TextSub1 Sub2\nTextSup1 Sup2";

    HE_ASSERT(engine.LayoutText(layout, Span<const FontFaceResourceReader>(&font, 1), text));
    HE_EXPECT_EQ(layout.lines.Size(), 2u);
    HE_EXPECT_GE(layout.clusters.Size(), 4u);
    HE_EXPECT_GE(layout.glyphs.Size(), 8u);

    const TextLine& line0 = layout.lines[0];
    const TextLine& line1 = layout.lines[1];
    HE_EXPECT_GT(line0.clusterCount, 0u);
    HE_EXPECT_GT(line1.clusterCount, 0u);

    const TextCluster& line0Cluster0 = layout.clusters[line0.clusterStart];
    const TextCluster& line1Cluster0 = layout.clusters[line1.clusterStart];
    HE_EXPECT_EQ(line0Cluster0.textByteStart, 0u);
    HE_EXPECT_EQ(line1Cluster0.textByteStart, 14u);
    HE_EXPECT_GT(line0Cluster0.glyphCount, 0u);
    HE_EXPECT_GT(line1Cluster0.glyphCount, 0u);

    const ShapedGlyph& line0Glyph0 = layout.glyphs[line0Cluster0.glyphStart];
    const ShapedGlyph& line1Glyph0 = layout.glyphs[line1Cluster0.glyphStart];
    HE_EXPECT_EQ(line0Glyph0.textByteStart, 0u);
    HE_EXPECT_EQ(line1Glyph0.textByteStart, 14u);
    HE_EXPECT_NE(line0Glyph0.glyphIndex, 0u);
    HE_EXPECT_NE(line1Glyph0.glyphIndex, 0u);
}

HE_TEST(scribe, layout_engine, styled_face_override)
{
    Vector<schema::Word> sansStorage;
    Vector<schema::Word> monoStorage;
    FontFaceResourceReader sans{};
    FontFaceResourceReader mono{};
    HE_ASSERT(BuildLoadedFontFaceFromFile("NotoSans-Regular.ttf", sansStorage, sans));
    HE_ASSERT(BuildLoadedFontFaceFromFile("NotoMono-Regular.ttf", monoStorage, mono));

    const FontFaceResourceReader faces[] = { sans, mono };

    const String text = "alpha beta";
    const uint32_t betaStart = 6;
    const uint32_t betaEnd = 10;

    TextStyle styles[2]{};
    styles[1].fontFaceIndex = 1;

    const TextStyleSpan spans[] =
    {
        { betaStart, betaEnd, 1 }
    };

    StyledTextLayoutDesc desc{};
    desc.fontFaces = Span<const FontFaceResourceReader>(faces, HE_LENGTH_OF(faces));
    desc.text = text;
    desc.styles = Span<const TextStyle>(styles, HE_LENGTH_OF(styles));
    desc.styleSpans = Span<const TextStyleSpan>(spans, HE_LENGTH_OF(spans));

    LayoutEngine engine;
    LayoutResult layout;
    HE_EXPECT(engine.LayoutStyledText(layout, desc));

    bool sawMonoCluster = false;
    for (const TextCluster& cluster : layout.clusters)
    {
        if ((cluster.textByteStart >= betaStart) && (cluster.textByteEnd <= betaEnd))
        {
            HE_EXPECT_EQ(cluster.styleIndex, 1u);
            HE_EXPECT_EQ(cluster.fontFaceIndex, 1u);
            sawMonoCluster = true;
        }
    }

    HE_EXPECT(sawMonoCluster);
}

HE_TEST(scribe, layout_engine, feature_flags_control_ligatures)
{
    Vector<schema::Word> fontStorage;
    FontFaceResourceReader font{};
    if (!BuildLoadedFontFaceFromCandidates(FeatureFontCandidates, fontStorage, font))
    {
        return;
    }

    const FontFaceResourceReader faces[] = { font };
    const String text = "office";

    LayoutEngine engine;
    LayoutResult defaultLayout;
    HE_ASSERT(engine.LayoutText(defaultLayout, Span<const FontFaceResourceReader>(faces, 1), text));

    const TextFeatureSetting features[] =
    {
        { MakeOpenTypeFeatureTag('l', 'i', 'g', 'a'), 0 },
        { MakeOpenTypeFeatureTag('c', 'l', 'i', 'g'), 0 },
    };

    TextStyle styles[2]{};
    styles[1].firstFeature = 0;
    styles[1].featureCount = HE_LENGTH_OF(features);

    const TextStyleSpan spans[] =
    {
        { 0, static_cast<uint32_t>(text.Size()), 1 }
    };

    StyledTextLayoutDesc desc{};
    desc.fontFaces = Span<const FontFaceResourceReader>(faces, 1);
    desc.text = text;
    desc.styles = Span<const TextStyle>(styles, HE_LENGTH_OF(styles));
    desc.styleSpans = Span<const TextStyleSpan>(spans, HE_LENGTH_OF(spans));
    desc.features = Span<const TextFeatureSetting>(features, HE_LENGTH_OF(features));

    LayoutResult noLigaLayout;
    HE_ASSERT(engine.LayoutStyledText(noLigaLayout, desc));

    HE_EXPECT(GlyphSequenceDiffers(noLigaLayout, defaultLayout));
}

HE_TEST(scribe, layout_engine, feature_flags_control_kerning_and_tracking)
{
    Vector<schema::Word> fontStorage;
    FontFaceResourceReader font{};
    if (!BuildLoadedFontFaceFromCandidates(FeatureFontCandidates, fontStorage, font))
    {
        return;
    }

    const FontFaceResourceReader faces[] = { font };
    const String text = "AVATAR";

    LayoutEngine engine;
    LayoutResult defaultLayout;
    HE_ASSERT(engine.LayoutText(defaultLayout, Span<const FontFaceResourceReader>(faces, 1), text));

    const TextFeatureSetting features[] =
    {
        { MakeOpenTypeFeatureTag('k', 'e', 'r', 'n'), 0 },
    };

    TextStyle styles[3]{};
    styles[1].firstFeature = 0;
    styles[1].featureCount = HE_LENGTH_OF(features);
    styles[2].trackingEm = 0.08f;

    const TextStyleSpan kernOffSpans[] =
    {
        { 0, static_cast<uint32_t>(text.Size()), 1 }
    };
    const TextStyleSpan trackingSpans[] =
    {
        { 0, static_cast<uint32_t>(text.Size()), 2 }
    };

    StyledTextLayoutDesc kernOffDesc{};
    kernOffDesc.fontFaces = Span<const FontFaceResourceReader>(faces, 1);
    kernOffDesc.text = text;
    kernOffDesc.styles = Span<const TextStyle>(styles, HE_LENGTH_OF(styles));
    kernOffDesc.styleSpans = Span<const TextStyleSpan>(kernOffSpans, HE_LENGTH_OF(kernOffSpans));
    kernOffDesc.features = Span<const TextFeatureSetting>(features, HE_LENGTH_OF(features));

    LayoutResult kernOffLayout;
    HE_ASSERT(engine.LayoutStyledText(kernOffLayout, kernOffDesc));

    StyledTextLayoutDesc trackingDesc{};
    trackingDesc.fontFaces = Span<const FontFaceResourceReader>(faces, 1);
    trackingDesc.text = text;
    trackingDesc.styles = Span<const TextStyle>(styles, HE_LENGTH_OF(styles));
    trackingDesc.styleSpans = Span<const TextStyleSpan>(trackingSpans, HE_LENGTH_OF(trackingSpans));

    LayoutResult trackingLayout;
    HE_ASSERT(engine.LayoutStyledText(trackingLayout, trackingDesc));

    HE_EXPECT_GT(kernOffLayout.width, defaultLayout.width);
    HE_EXPECT_GT(trackingLayout.width, defaultLayout.width);
}

HE_TEST(scribe, layout_engine, feature_flags_enable_small_caps_and_case_forms)
{
    Vector<schema::Word> fontStorage;
    FontFaceResourceReader font{};
    if (!BuildLoadedFontFaceFromCandidates(FeatureFontCandidates, fontStorage, font))
    {
        return;
    }

    const FontFaceResourceReader faces[] = { font };
    LayoutEngine engine;

    {
        const String text = "Harvest Engine";
        LayoutResult defaultLayout;
        HE_ASSERT(engine.LayoutText(defaultLayout, Span<const FontFaceResourceReader>(faces, 1), text));

        const TextFeatureSetting features[] =
        {
            { MakeOpenTypeFeatureTag('s', 'm', 'c', 'p'), 1 },
        };

        TextStyle styles[2]{};
        styles[1].firstFeature = 0;
        styles[1].featureCount = HE_LENGTH_OF(features);

        const TextStyleSpan spans[] =
        {
            { 0, static_cast<uint32_t>(text.Size()), 1 }
        };

        StyledTextLayoutDesc desc{};
        desc.fontFaces = Span<const FontFaceResourceReader>(faces, 1);
        desc.text = text;
        desc.styles = Span<const TextStyle>(styles, HE_LENGTH_OF(styles));
        desc.styleSpans = Span<const TextStyleSpan>(spans, HE_LENGTH_OF(spans));
        desc.features = Span<const TextFeatureSetting>(features, HE_LENGTH_OF(features));

        LayoutResult smallCapsLayout;
        HE_ASSERT(engine.LayoutStyledText(smallCapsLayout, desc));
        HE_EXPECT(GlyphSequenceDiffers(smallCapsLayout, defaultLayout));
    }

    {
        const String text = "[(ALL-CAPS)]";
        LayoutResult defaultLayout;
        HE_ASSERT(engine.LayoutText(defaultLayout, Span<const FontFaceResourceReader>(faces, 1), text));

        const TextFeatureSetting features[] =
        {
            { MakeOpenTypeFeatureTag('c', 'a', 's', 'e'), 1 },
        };

        TextStyle styles[2]{};
        styles[1].firstFeature = 0;
        styles[1].featureCount = HE_LENGTH_OF(features);

        const TextStyleSpan spans[] =
        {
            { 0, static_cast<uint32_t>(text.Size()), 1 }
        };

        StyledTextLayoutDesc desc{};
        desc.fontFaces = Span<const FontFaceResourceReader>(faces, 1);
        desc.text = text;
        desc.styles = Span<const TextStyle>(styles, HE_LENGTH_OF(styles));
        desc.styleSpans = Span<const TextStyleSpan>(spans, HE_LENGTH_OF(spans));
        desc.features = Span<const TextFeatureSetting>(features, HE_LENGTH_OF(features));

        LayoutResult caseLayout;
        HE_ASSERT(engine.LayoutStyledText(caseLayout, desc));
        HE_EXPECT(GlyphSequenceDiffers(caseLayout, defaultLayout));
    }
}

HE_TEST(scribe, layout_engine, feature_flags_enable_numeric_forms)
{
    Vector<schema::Word> fontStorage;
    FontFaceResourceReader font{};
    if (!BuildLoadedFontFaceFromCandidates(FeatureFontCandidates, fontStorage, font))
    {
        return;
    }

    const FontFaceResourceReader faces[] = { font };
    LayoutEngine engine;

    {
        const String text = "1st 2nd 3rd 4th";
        LayoutResult defaultLayout;
        HE_ASSERT(engine.LayoutText(defaultLayout, Span<const FontFaceResourceReader>(faces, 1), text));

        const TextFeatureSetting features[] =
        {
            { MakeOpenTypeFeatureTag('o', 'r', 'd', 'n'), 1 },
        };

        TextStyle styles[2]{};
        styles[1].firstFeature = 0;
        styles[1].featureCount = HE_LENGTH_OF(features);

        const TextStyleSpan spans[] =
        {
            { 0, static_cast<uint32_t>(text.Size()), 1 }
        };

        StyledTextLayoutDesc desc{};
        desc.fontFaces = Span<const FontFaceResourceReader>(faces, 1);
        desc.text = text;
        desc.styles = Span<const TextStyle>(styles, HE_LENGTH_OF(styles));
        desc.styleSpans = Span<const TextStyleSpan>(spans, HE_LENGTH_OF(spans));
        desc.features = Span<const TextFeatureSetting>(features, HE_LENGTH_OF(features));

        LayoutResult ordinalLayout;
        HE_ASSERT(engine.LayoutStyledText(ordinalLayout, desc));
        HE_EXPECT(GlyphSequenceDiffers(ordinalLayout, defaultLayout));
    }

    {
        const String text = "123/456";
        LayoutResult defaultLayout;
        HE_ASSERT(engine.LayoutText(defaultLayout, Span<const FontFaceResourceReader>(faces, 1), text));

        const TextFeatureSetting features[] =
        {
            { MakeOpenTypeFeatureTag('f', 'r', 'a', 'c'), 1 },
        };

        TextStyle styles[2]{};
        styles[1].firstFeature = 0;
        styles[1].featureCount = HE_LENGTH_OF(features);

        const TextStyleSpan spans[] =
        {
            { 0, static_cast<uint32_t>(text.Size()), 1 }
        };

        StyledTextLayoutDesc desc{};
        desc.fontFaces = Span<const FontFaceResourceReader>(faces, 1);
        desc.text = text;
        desc.styles = Span<const TextStyle>(styles, HE_LENGTH_OF(styles));
        desc.styleSpans = Span<const TextStyleSpan>(spans, HE_LENGTH_OF(spans));
        desc.features = Span<const TextFeatureSetting>(features, HE_LENGTH_OF(features));

        LayoutResult fractionLayout;
        HE_ASSERT(engine.LayoutStyledText(fractionLayout, desc));
        HE_EXPECT(GlyphSequenceDiffers(fractionLayout, defaultLayout));
    }

    {
        const String text = "0123456789";
        LayoutResult defaultLayout;
        HE_ASSERT(engine.LayoutText(defaultLayout, Span<const FontFaceResourceReader>(faces, 1), text));

        const TextFeatureSetting features[] =
        {
            { MakeOpenTypeFeatureTag('o', 'n', 'u', 'm'), 1 },
            { MakeOpenTypeFeatureTag('p', 'n', 'u', 'm'), 1 },
        };

        TextStyle styles[2]{};
        styles[1].firstFeature = 0;
        styles[1].featureCount = HE_LENGTH_OF(features);

        const TextStyleSpan spans[] =
        {
            { 0, static_cast<uint32_t>(text.Size()), 1 }
        };

        StyledTextLayoutDesc desc{};
        desc.fontFaces = Span<const FontFaceResourceReader>(faces, 1);
        desc.text = text;
        desc.styles = Span<const TextStyle>(styles, HE_LENGTH_OF(styles));
        desc.styleSpans = Span<const TextStyleSpan>(spans, HE_LENGTH_OF(spans));
        desc.features = Span<const TextFeatureSetting>(features, HE_LENGTH_OF(features));

        LayoutResult figureLayout;
        HE_ASSERT(engine.LayoutStyledText(figureLayout, desc));
        HE_EXPECT(GlyphSequenceDiffers(figureLayout, defaultLayout));
    }
}
