// Copyright Chad Engler

#include "font_compile_geometry.h"
#include "font_import_utils.h"

#include "he/scribe/compiled_font.h"
#include "he/scribe/layout_engine.h"
#include "he/scribe/runtime_blob.h"

#include "he/core/file.h"
#include "he/core/test.h"

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
