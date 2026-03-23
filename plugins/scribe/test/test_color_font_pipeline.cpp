// Copyright Chad Engler

#include "font_compile_geometry.h"
#include "font_import_utils.h"

#include "he/scribe/compiled_font.h"
#include "he/scribe/layout_engine.h"
#include "he/scribe/retained_text.h"
#include "he/scribe/renderer.h"
#include "he/scribe/runtime_blob.h"

#include "he/core/file.h"
#include "he/core/test.h"
#include "he/rhi/device.h"
#include "he/rhi/instance.h"

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
        LoadedFontFaceBlob& out,
        const Vector<uint8_t>& fontBytes,
        const char* displayName)
    {
        FontFaceInfo faceInfo{};
        if (!InspectFontFace(fontBytes, 0, FontSourceFormat::TrueType, faceInfo))
        {
            return false;
        }

        CompiledFontRenderData renderData{};
        if (!BuildCompiledFontRenderData(renderData, fontBytes, 0))
        {
            return false;
        }

        schema::Builder shapingBuilder;
        FontFaceShapingData::Builder shaping = shapingBuilder.AddStruct<FontFaceShapingData>();
        shaping.SetFaceIndex(faceInfo.faceIndex);
        shaping.SetSourceFormat(faceInfo.sourceFormat);
        shaping.SetSourceBytes(shapingBuilder.AddBlob(fontBytes));
        shapingBuilder.SetRoot(shaping);

        schema::Builder metadataBuilder;
        FontFaceImportMetadata::Builder metadata = metadataBuilder.AddStruct<FontFaceImportMetadata>();
        FillFontFaceImportMetadata(metadata, faceInfo);
        if (metadata.GetFamilyName().Size() == 0)
        {
            metadata.InitFamilyName(displayName);
        }
        metadataBuilder.SetRoot(metadata);

        schema::Builder renderBuilder;
        FontFaceRenderData::Builder render = renderBuilder.AddStruct<FontFaceRenderData>();
        render.SetCurveTextureWidth(renderData.curveTextureWidth);
        render.SetCurveTextureHeight(renderData.curveTextureHeight);
        render.SetBandTextureWidth(renderData.bandTextureWidth);
        render.SetBandTextureHeight(renderData.bandTextureHeight);
        render.SetBandOverlapEpsilon(renderData.bandOverlapEpsilon);

        auto glyphs = render.InitGlyphs(renderData.glyphs.Size());
        for (uint32_t glyphIndex = 0; glyphIndex < renderData.glyphs.Size(); ++glyphIndex)
        {
            const CompiledGlyphRenderEntry& srcGlyph = renderData.glyphs[glyphIndex];
            FontFaceGlyphRenderData::Builder dstGlyph = glyphs[glyphIndex];
            dstGlyph.SetAdvanceX(srcGlyph.advanceX);
            dstGlyph.SetAdvanceY(srcGlyph.advanceY);
            dstGlyph.SetBoundsMinX(srcGlyph.boundsMinX);
            dstGlyph.SetBoundsMinY(srcGlyph.boundsMinY);
            dstGlyph.SetBoundsMaxX(srcGlyph.boundsMaxX);
            dstGlyph.SetBoundsMaxY(srcGlyph.boundsMaxY);
            dstGlyph.SetBandScaleX(srcGlyph.bandScaleX);
            dstGlyph.SetBandScaleY(srcGlyph.bandScaleY);
            dstGlyph.SetBandOffsetX(srcGlyph.bandOffsetX);
            dstGlyph.SetBandOffsetY(srcGlyph.bandOffsetY);
            dstGlyph.SetGlyphBandLocX(srcGlyph.glyphBandLocX);
            dstGlyph.SetGlyphBandLocY(srcGlyph.glyphBandLocY);
            dstGlyph.SetBandMaxX(srcGlyph.bandMaxX);
            dstGlyph.SetBandMaxY(srcGlyph.bandMaxY);
            dstGlyph.SetFillRule(srcGlyph.fillRule);
            dstGlyph.SetFlags(srcGlyph.flags);
        }
        renderBuilder.SetRoot(render);

        schema::Builder paintBuilder;
        FontFacePaintData::Builder paint = paintBuilder.AddStruct<FontFacePaintData>();
        paint.SetDefaultPaletteIndex(renderData.paint.defaultPaletteIndex);

        auto palettes = paint.InitPalettes(renderData.paint.palettes.Size());
        for (uint32_t paletteIndex = 0; paletteIndex < renderData.paint.palettes.Size(); ++paletteIndex)
        {
            const CompiledFontPalette& srcPalette = renderData.paint.palettes[paletteIndex];
            FontFacePalette::Builder dstPalette = palettes[paletteIndex];
            dstPalette.SetFlags(srcPalette.flags);

            auto colors = dstPalette.InitColors(srcPalette.colors.Size());
            for (uint32_t colorIndex = 0; colorIndex < srcPalette.colors.Size(); ++colorIndex)
            {
                const CompiledFontPaletteColor& srcColor = srcPalette.colors[colorIndex];
                FontFacePaletteColor::Builder dstColor = colors[colorIndex];
                dstColor.SetRed(srcColor.red);
                dstColor.SetGreen(srcColor.green);
                dstColor.SetBlue(srcColor.blue);
                dstColor.SetAlpha(srcColor.alpha);
            }
        }

        auto colorGlyphs = paint.InitColorGlyphs(renderData.paint.colorGlyphs.Size());
        for (uint32_t glyphIndex = 0; glyphIndex < renderData.paint.colorGlyphs.Size(); ++glyphIndex)
        {
            const CompiledColorGlyphEntry& srcColorGlyph = renderData.paint.colorGlyphs[glyphIndex];
            FontFaceColorGlyph::Builder dstColorGlyph = colorGlyphs[glyphIndex];
            dstColorGlyph.SetFirstLayer(srcColorGlyph.firstLayer);
            dstColorGlyph.SetLayerCount(srcColorGlyph.layerCount);
        }

        auto layers = paint.InitLayers(renderData.paint.layers.Size());
        for (uint32_t layerIndex = 0; layerIndex < renderData.paint.layers.Size(); ++layerIndex)
        {
            const CompiledColorGlyphLayerEntry& srcLayer = renderData.paint.layers[layerIndex];
            FontFaceColorGlyphLayer::Builder dstLayer = layers[layerIndex];
            dstLayer.SetGlyphIndex(srcLayer.glyphIndex);
            dstLayer.SetPaletteEntryIndex(srcLayer.paletteEntryIndex);
            dstLayer.SetFlags(srcLayer.flags);
            dstLayer.SetAlphaScale(srcLayer.alphaScale);
            dstLayer.SetTransform00(srcLayer.transform00);
            dstLayer.SetTransform01(srcLayer.transform01);
            dstLayer.SetTransform10(srcLayer.transform10);
            dstLayer.SetTransform11(srcLayer.transform11);
            dstLayer.SetTransformTx(srcLayer.transformTx);
            dstLayer.SetTransformTy(srcLayer.transformTy);
        }
        paintBuilder.SetRoot(paint);

        schema::Builder rootBuilder;
        CompiledFontFaceBlob::Builder root = rootBuilder.AddStruct<CompiledFontFaceBlob>();
        RuntimeBlobHeader::Builder header = root.InitHeader();
        header.SetFormatVersion(RuntimeBlobFormatVersion);
        header.SetKind(RuntimeBlobKind::FontFace);
        header.SetFlags(0);
        root.SetShapingData(rootBuilder.AddBlob(Span<const schema::Word>(shapingBuilder).AsBytes()));
        root.SetCurveData(rootBuilder.AddBlob(Span<const PackedCurveTexel>(renderData.curveTexels.Data(), renderData.curveTexels.Size()).AsBytes()));
        root.SetBandData(rootBuilder.AddBlob(Span<const PackedBandTexel>(renderData.bandTexels.Data(), renderData.bandTexels.Size()).AsBytes()));
        root.SetPaintData(rootBuilder.AddBlob(Span<const schema::Word>(paintBuilder).AsBytes()));
        root.SetMetadataData(rootBuilder.AddBlob(Span<const schema::Word>(metadataBuilder).AsBytes()));
        root.SetRenderData(rootBuilder.AddBlob(Span<const schema::Word>(renderBuilder).AsBytes()));
        rootBuilder.SetRoot(root);

        storage = Span<const schema::Word>(rootBuilder);
        return LoadCompiledFontFaceBlob(out, storage);
    }

    bool BuildRetainedTextFromTemporaryFaceCopy(
        RetainedTextModel& out,
        Span<const LoadedFontFaceBlob> fontFaces,
        const char* text,
        float fontSize,
        bool darkBackgroundPreferred = true)
    {
        Vector<LoadedFontFaceBlob> temporaryFaces{};
        temporaryFaces.Resize(fontFaces.Size(), DefaultInit);
        for (uint32_t fontIndex = 0; fontIndex < fontFaces.Size(); ++fontIndex)
        {
            temporaryFaces[fontIndex] = fontFaces[fontIndex];
        }

        LayoutEngine engine;
        LayoutResult layout;
        LayoutOptions options{};
        options.fontSize = fontSize;
        options.maxWidth = 4096.0f;
        options.wrap = false;
        options.direction = TextDirection::Auto;
        if (!engine.LayoutText(
            layout,
            Span<const LoadedFontFaceBlob>(temporaryFaces.Data(), temporaryFaces.Size()),
            text,
            options))
        {
            return false;
        }

        RetainedTextBuildDesc desc{};
        desc.fontFaces = Span<const LoadedFontFaceBlob>(temporaryFaces.Data(), temporaryFaces.Size());
        desc.layout = &layout;
        desc.fontSize = fontSize;
        desc.darkBackgroundPreferred = darkBackgroundPreferred;
        return out.Build(desc);
    }

    struct NullRendererHarness
    {
        rhi::Instance* instance{ nullptr };
        rhi::Device* device{ nullptr };
        Renderer renderer{};

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

            if (!renderer.Initialize(*device, rhi::Format::BGRA8Unorm_sRGB))
            {
                Terminate();
                return false;
            }

            return true;
        }

        void Terminate()
        {
            renderer.Terminate();

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

HE_TEST(scribe, color_font_pipeline, resolves_compiled_layers_from_runtime_blob)
{
    static constexpr const char* ColorFontPath = "C:/Windows/Fonts/seguiemj.ttf";
    if (!File::Exists(ColorFontPath))
    {
        return;
    }

    Vector<uint8_t> fontBytes;
    HE_ASSERT(ReadFontFile(fontBytes, ColorFontPath));

    Vector<schema::Word> storage;
    LoadedFontFaceBlob font{};
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
    LoadedFontFaceBlob repoFont{};
    LoadedFontFaceBlob colorFont{};
    HE_ASSERT(BuildLoadedCompiledFontFace(repoStorage, repoFont, repoFontBytes, "NotoSans-Regular.ttf"));
    HE_ASSERT(BuildLoadedCompiledFontFace(colorStorage, colorFont, colorFontBytes, "seguiemj.ttf"));

    const LoadedFontFaceBlob faces[] =
    {
        repoFont,
        colorFont,
    };

    LayoutEngine engine;
    LayoutResult layout;
    LayoutOptions options{};
    options.fontSize = 24.0f;
    options.maxWidth = 1024.0f;
    options.wrap = false;
    options.direction = TextDirection::Auto;

    HE_ASSERT(engine.LayoutText(
        layout,
        Span<const LoadedFontFaceBlob>(faces),
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
    LoadedFontFaceBlob repoFont{};
    LoadedFontFaceBlob colorFont{};
    HE_ASSERT(BuildLoadedCompiledFontFace(repoStorage, repoFont, repoFontBytes, "NotoSans-Regular.ttf"));
    HE_ASSERT(BuildLoadedCompiledFontFace(colorStorage, colorFont, colorFontBytes, "seguiemj.ttf"));

    const LoadedFontFaceBlob faces[] =
    {
        repoFont,
        colorFont,
    };

    LayoutEngine engine;
    LayoutResult layout;
    LayoutOptions options{};
    options.fontSize = 24.0f;
    options.maxWidth = 1024.0f;
    options.wrap = false;
    options.direction = TextDirection::Auto;

    HE_ASSERT(engine.LayoutText(
        layout,
        Span<const LoadedFontFaceBlob>(faces),
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
    LoadedFontFaceBlob font{};
    HE_ASSERT(BuildLoadedCompiledFontFace(storage, font, fontBytes, "Noto Sans"));

    LayoutEngine engine;
    LayoutResult layout;
    LayoutOptions options{};
    options.fontSize = 28.0f;
    options.wrap = false;
    HE_ASSERT(engine.LayoutText(layout, Span<const LoadedFontFaceBlob>(&font, 1), "Retained text", options));

    RetainedTextModel retainedText;
    RetainedTextBuildDesc desc{};
    desc.fontFaces = Span<const LoadedFontFaceBlob>(&font, 1);
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
    LoadedFontFaceBlob sans{};
    LoadedFontFaceBlob mono{};
    HE_ASSERT(BuildLoadedCompiledFontFace(sansStorage, sans, sansBytes, "Noto Sans"));
    HE_ASSERT(BuildLoadedCompiledFontFace(monoStorage, mono, monoBytes, "Noto Mono"));

    const LoadedFontFaceBlob faces[] = { sans, mono };
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
    layoutDesc.fontFaces = Span<const LoadedFontFaceBlob>(faces, HE_LENGTH_OF(faces));
    layoutDesc.text = text;
    layoutDesc.styles = Span<const TextStyle>(styles, HE_LENGTH_OF(styles));
    layoutDesc.styleSpans = Span<const TextStyleSpan>(spans, HE_LENGTH_OF(spans));
    layoutDesc.options.fontSize = 28.0f;
    layoutDesc.options.wrap = false;

    LayoutEngine engine;
    LayoutResult layout;
    HE_ASSERT(engine.LayoutStyledText(layout, layoutDesc));

    RetainedTextModel retainedText;
    RetainedTextBuildDesc retainedDesc{};
    retainedDesc.fontFaces = Span<const LoadedFontFaceBlob>(faces, HE_LENGTH_OF(faces));
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

HE_TEST(scribe, retained_text, emits_decoration_quads_for_styled_runs)
{
    String repoFontPath;
    HE_ASSERT(ResolveRepoFontPath(repoFontPath, "NotoSans-Regular.ttf"));

    Vector<uint8_t> fontBytes;
    HE_ASSERT(ReadFontFile(fontBytes, repoFontPath.Data()));

    Vector<schema::Word> storage;
    LoadedFontFaceBlob font{};
    HE_ASSERT(BuildLoadedCompiledFontFace(storage, font, fontBytes, "Noto Sans"));

    const LoadedFontFaceBlob faces[] = { font };
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
    layoutDesc.fontFaces = Span<const LoadedFontFaceBlob>(faces, HE_LENGTH_OF(faces));
    layoutDesc.text = text;
    layoutDesc.styles = Span<const TextStyle>(styles, HE_LENGTH_OF(styles));
    layoutDesc.styleSpans = Span<const TextStyleSpan>(spans, HE_LENGTH_OF(spans));
    layoutDesc.options.fontSize = 28.0f;
    layoutDesc.options.wrap = false;

    LayoutEngine engine;
    LayoutResult layout;
    HE_ASSERT(engine.LayoutStyledText(layout, layoutDesc));

    RetainedTextModel retainedText;
    RetainedTextBuildDesc retainedDesc{};
    retainedDesc.fontFaces = Span<const LoadedFontFaceBlob>(faces, HE_LENGTH_OF(faces));
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
    LoadedFontFaceBlob font{};
    HE_ASSERT(BuildLoadedCompiledFontFace(storage, font, fontBytes, "Noto Sans"));

    const LoadedFontFaceBlob faces[] = { font };
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
    layoutDesc.fontFaces = Span<const LoadedFontFaceBlob>(faces, HE_LENGTH_OF(faces));
    layoutDesc.text = text;
    layoutDesc.styles = Span<const TextStyle>(styles, HE_LENGTH_OF(styles));
    layoutDesc.styleSpans = Span<const TextStyleSpan>(spans, HE_LENGTH_OF(spans));
    layoutDesc.options.fontSize = 40.0f;
    layoutDesc.options.wrap = false;

    LayoutEngine engine;
    LayoutResult layout;
    HE_ASSERT(engine.LayoutStyledText(layout, layoutDesc));

    RetainedTextModel retainedText;
    RetainedTextBuildDesc retainedDesc{};
    retainedDesc.fontFaces = Span<const LoadedFontFaceBlob>(faces, HE_LENGTH_OF(faces));
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
    LoadedFontFaceBlob font{};
    HE_ASSERT(BuildLoadedCompiledFontFace(storage, font, fontBytes, "Segoe UI Emoji"));

    LayoutEngine engine;
    LayoutResult layout;
    LayoutOptions options{};
    options.fontSize = 44.0f;
    options.wrap = false;
    HE_ASSERT(engine.LayoutText(layout, Span<const LoadedFontFaceBlob>(&font, 1), "🙂😀🎨", options));

    RetainedTextModel retainedText;
    RetainedTextBuildDesc desc{};
    desc.fontFaces = Span<const LoadedFontFaceBlob>(&font, 1);
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
    LoadedFontFaceBlob font{};
    HE_ASSERT(BuildLoadedCompiledFontFace(storage, font, fontBytes, "Noto Sans"));

    RetainedTextModel retainedText;
    HE_ASSERT(BuildRetainedTextFromTemporaryFaceCopy(
        retainedText,
        Span<const LoadedFontFaceBlob>(&font, 1),
        "Retained text",
        28.0f));

    HE_EXPECT_GT(retainedText.GetDrawCount(), 0u);
    for (const RetainedTextDraw& draw : retainedText.GetDraws())
    {
        HE_EXPECT_NE_PTR(retainedText.GetFontFace(draw.fontFaceIndex), nullptr);
    }

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
    LoadedFontFaceBlob repoFont{};
    LoadedFontFaceBlob colorFont{};
    HE_ASSERT(BuildLoadedCompiledFontFace(repoStorage, repoFont, repoFontBytes, "NotoSans-Regular.ttf"));
    HE_ASSERT(BuildLoadedCompiledFontFace(colorStorage, colorFont, colorFontBytes, "seguiemj.ttf"));

    const LoadedFontFaceBlob faces[] =
    {
        repoFont,
        colorFont,
    };

    RetainedTextModel retainedText;
    HE_ASSERT(BuildRetainedTextFromTemporaryFaceCopy(
        retainedText,
        Span<const LoadedFontFaceBlob>(faces, HE_LENGTH_OF(faces)),
        "\xF0\x9F\x99\x82 \xF0\x9F\x98\x80 \xF0\x9F\x8E\xA8 \xF0\x9F\x8C\x88 \xE2\x9C\xA8",
        44.0f));

    bool sawFallbackFaceDraw = false;
    bool sawPaletteColorDraw = false;
    for (const RetainedTextDraw& draw : retainedText.GetDraws())
    {
        HE_EXPECT_NE_PTR(retainedText.GetFontFace(draw.fontFaceIndex), nullptr);
        sawFallbackFaceDraw |= draw.fontFaceIndex == 1u;
        sawPaletteColorDraw |= (draw.flags & RetainedTextDrawFlagUseForegroundColor) == 0;
    }

    HE_EXPECT(sawFallbackFaceDraw);
    HE_EXPECT(sawPaletteColorDraw);

    NullRendererHarness harness;
    HE_ASSERT(harness.Initialize());
    HE_EXPECT(harness.renderer.PrepareRetainedText(retainedText));

    RetainedTextInstanceDesc instance{};
    harness.renderer.QueueRetainedText(retainedText, instance);
}
