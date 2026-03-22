// Copyright Chad Engler

#include "he/scribe/compiled_font.h"
#include "he/scribe/packed_data.h"
#include "he/scribe/runtime_blob.h"

#include "he/core/test.h"

using namespace he;
using namespace he::scribe;

namespace
{
    Span<const uint8_t> BuildFontFaceShapingBytes(schema::Builder& builder)
    {
        FontFaceShapingData::Builder shaping = builder.AddStruct<FontFaceShapingData>();
        shaping.SetFaceIndex(2);
        shaping.SetSourceFormat(FontSourceFormat::TrueType);
        shaping.SetSourceBytes(builder.AddBlob({ reinterpret_cast<const uint8_t*>("abc"), 3 }));
        builder.SetRoot(shaping);
        return Span<const schema::Word>(builder).AsBytes();
    }

    Span<const uint8_t> BuildFontFaceMetadataBytes(schema::Builder& builder)
    {
        FontFaceImportMetadata::Builder metadata = builder.AddStruct<FontFaceImportMetadata>();
        metadata.SetFaceIndex(2);
        metadata.SetSourceFormat(FontSourceFormat::TrueType);
        metadata.InitFamilyName("Unit Test Family");
        metadata.InitStyleName("Regular");
        metadata.InitPostscriptName("UnitTestFamily-Regular");
        metadata.SetGlyphCount(42);

        FontFaceMetrics::Builder metrics = metadata.InitMetrics();
        metrics.SetUnitsPerEm(1000);
        metrics.SetAscender(800);
        metrics.SetDescender(-200);
        metrics.SetLineHeight(1200);
        metrics.SetMaxAdvanceWidth(1400);
        metrics.SetMaxAdvanceHeight(1200);
        metrics.SetCapHeight(700);

        metadata.SetIsScalable(true);
        metadata.SetHasColorGlyphs(false);
        metadata.SetHasKerning(true);
        metadata.SetHasHorizontalLayout(true);
        metadata.SetHasVerticalLayout(false);
        builder.SetRoot(metadata);
        return Span<const schema::Word>(builder).AsBytes();
    }

    Span<const uint8_t> BuildFontFaceRenderBytes(schema::Builder& builder)
    {
        FontFaceRenderData::Builder render = builder.AddStruct<FontFaceRenderData>();
        render.SetCurveTextureWidth(8);
        render.SetCurveTextureHeight(1);
        render.SetBandTextureWidth(ScribeBandTextureWidth);
        render.SetBandTextureHeight(1);
        render.SetBandOverlapEpsilon(1.0f);

        auto glyphs = render.InitGlyphs(2);

        {
            FontFaceGlyphRenderData::Builder glyph = glyphs[0];
            glyph.SetAdvanceX(900);
            glyph.SetAdvanceY(0);
            glyph.SetBoundsMinX(0.0f);
            glyph.SetBoundsMinY(0.0f);
            glyph.SetBoundsMaxX(1.0f);
            glyph.SetBoundsMaxY(1.0f);
            glyph.SetBandScaleX(0.0f);
            glyph.SetBandScaleY(0.0f);
            glyph.SetBandOffsetX(0.0f);
            glyph.SetBandOffsetY(0.0f);
            glyph.SetGlyphBandLocX(0);
            glyph.SetGlyphBandLocY(0);
            glyph.SetBandMaxX(0);
            glyph.SetBandMaxY(0);
            glyph.SetFillRule(FillRule::NonZero);
            glyph.SetFlags(CompiledFontGlyphFlagHasGeometry);
        }

        {
            FontFaceGlyphRenderData::Builder glyph = glyphs[1];
            glyph.SetAdvanceX(500);
            glyph.SetAdvanceY(0);
            glyph.SetBoundsMinX(0.0f);
            glyph.SetBoundsMinY(0.0f);
            glyph.SetBoundsMaxX(0.0f);
            glyph.SetBoundsMaxY(0.0f);
            glyph.SetBandScaleX(0.0f);
            glyph.SetBandScaleY(0.0f);
            glyph.SetBandOffsetX(0.0f);
            glyph.SetBandOffsetY(0.0f);
            glyph.SetGlyphBandLocX(0);
            glyph.SetGlyphBandLocY(0);
            glyph.SetBandMaxX(0);
            glyph.SetBandMaxY(0);
            glyph.SetFillRule(FillRule::NonZero);
            glyph.SetFlags(0);
        }

        builder.SetRoot(render);
        return Span<const schema::Word>(builder).AsBytes();
    }

    Span<const uint8_t> BuildFontFacePaintBytes(schema::Builder& builder)
    {
        FontFacePaintData::Builder paint = builder.AddStruct<FontFacePaintData>();
        paint.SetDefaultPaletteIndex(0);

        auto palettes = paint.InitPalettes(1);
        {
            FontFacePalette::Builder palette = palettes[0];
            palette.SetFlags(0x02u);

            auto colors = palette.InitColors(2);
            colors[0].SetRed(1.0f);
            colors[0].SetGreen(0.0f);
            colors[0].SetBlue(0.0f);
            colors[0].SetAlpha(1.0f);

            colors[1].SetRed(0.0f);
            colors[1].SetGreen(0.6f);
            colors[1].SetBlue(1.0f);
            colors[1].SetAlpha(1.0f);
        }

        auto colorGlyphs = paint.InitColorGlyphs(2);
        colorGlyphs[0].SetFirstLayer(0);
        colorGlyphs[0].SetLayerCount(2);
        colorGlyphs[1].SetFirstLayer(2);
        colorGlyphs[1].SetLayerCount(0);

        auto layers = paint.InitLayers(2);
        layers[0].SetGlyphIndex(0);
        layers[0].SetPaletteEntryIndex(0);
        layers[0].SetFlags(0);
        layers[0].SetAlphaScale(0.75f);
        layers[0].SetTransform00(1.0f);
        layers[0].SetTransform01(0.0f);
        layers[0].SetTransform10(0.0f);
        layers[0].SetTransform11(1.0f);
        layers[0].SetTransformTx(12.0f);
        layers[0].SetTransformTy(-6.0f);
        layers[1].SetGlyphIndex(0);
        layers[1].SetPaletteEntryIndex(0);
        layers[1].SetFlags(CompiledFontColorLayerFlagUseForeground);
        layers[1].SetAlphaScale(0.5f);
        layers[1].SetTransform00(0.5f);
        layers[1].SetTransform01(0.25f);
        layers[1].SetTransform10(-0.75f);
        layers[1].SetTransform11(1.5f);
        layers[1].SetTransformTx(3.0f);
        layers[1].SetTransformTy(9.0f);

        builder.SetRoot(paint);
        return Span<const schema::Word>(builder).AsBytes();
    }

    Span<const uint8_t> BuildCurveBytes()
    {
        static const PackedCurveTexel CurveTexels[] =
        {
            PackCurveTexel(1.0f, 1.0f, 1.0f, 0.5f),
            PackCurveTexel(1.0f, 0.0f, 0.0f, 0.0f),
            PackCurveTexel(0.0f, 1.0f, 0.5f, 1.0f),
            PackCurveTexel(1.0f, 1.0f, 0.0f, 0.0f),
            PackCurveTexel(1.0f, 0.0f, 0.5f, 0.0f),
            PackCurveTexel(0.0f, 0.0f, 0.0f, 0.0f),
            PackCurveTexel(0.0f, 0.0f, 0.0f, 0.5f),
            PackCurveTexel(0.0f, 1.0f, 0.0f, 0.0f),
        };
        return Span<const PackedCurveTexel>(CurveTexels).AsBytes();
    }

    Span<const uint8_t> BuildBandBytes()
    {
        static PackedBandTexel BandTexels[ScribeBandTextureWidth]{};
        static bool Initialized = false;
        if (!Initialized)
        {
            BandTexels[0] = { 4, 2 };
            BandTexels[1] = { 4, 6 };
            BandTexels[2] = { 0, 0 };
            BandTexels[3] = { 2, 0 };
            BandTexels[4] = { 4, 0 };
            BandTexels[5] = { 6, 0 };
            BandTexels[6] = { 2, 0 };
            BandTexels[7] = { 0, 0 };
            BandTexels[8] = { 6, 0 };
            BandTexels[9] = { 4, 0 };
            Initialized = true;
        }

        return Span<const PackedBandTexel>(BandTexels).AsBytes();
    }
}

HE_TEST(scribe, runtime_blob, load_compiled_font_face_success)
{
    schema::Builder shapingBuilder;
    const Span<const uint8_t> shapingBytes = BuildFontFaceShapingBytes(shapingBuilder);

    schema::Builder metadataBuilder;
    const Span<const uint8_t> metadataBytes = BuildFontFaceMetadataBytes(metadataBuilder);

    schema::Builder renderBuilder;
    const Span<const uint8_t> renderBytes = BuildFontFaceRenderBytes(renderBuilder);

    schema::Builder paintBuilder;
    const Span<const uint8_t> paintBytes = BuildFontFacePaintBytes(paintBuilder);

    schema::Builder rootBuilder;
    CompiledFontFaceBlob::Builder root = rootBuilder.AddStruct<CompiledFontFaceBlob>();

    RuntimeBlobHeader::Builder header = root.InitHeader();
    header.SetFormatVersion(RuntimeBlobFormatVersion);
    header.SetKind(RuntimeBlobKind::FontFace);
    header.SetFlags(0);

    root.SetShapingData(rootBuilder.AddBlob(shapingBytes));
    root.SetCurveData(rootBuilder.AddBlob(BuildCurveBytes()));
    root.SetBandData(rootBuilder.AddBlob(BuildBandBytes()));
    root.SetPaintData(rootBuilder.AddBlob(paintBytes));
    root.SetMetadataData(rootBuilder.AddBlob(metadataBytes));
    root.SetRenderData(rootBuilder.AddBlob(renderBytes));
    rootBuilder.SetRoot(root);

    LoadedFontFaceBlob loaded{};
    const bool ok = LoadCompiledFontFaceBlob(loaded, Span<const schema::Word>(rootBuilder));

    HE_EXPECT(ok);
    HE_EXPECT(loaded.root.IsValid());
    HE_EXPECT_EQ(loaded.metadata.GetGlyphCount(), 42u);
    HE_EXPECT_EQ(loaded.metadata.GetMetrics().GetUnitsPerEm(), 1000u);
    HE_EXPECT_EQ(loaded.metadata.GetMetrics().GetCapHeight(), 700);
    HE_EXPECT_EQ_STR(loaded.metadata.GetFamilyName().Data(), "Unit Test Family");
    HE_EXPECT_EQ(loaded.shaping.GetFaceIndex(), 2u);
    HE_EXPECT_EQ(loaded.shaping.GetSourceBytes().Size(), 3u);
    HE_EXPECT_EQ(loaded.render.GetGlyphs().Size(), 2u);
    HE_EXPECT_EQ(loaded.render.GetCurveTextureWidth(), 8u);
    HE_EXPECT_EQ(loaded.render.GetBandTextureWidth(), ScribeBandTextureWidth);
    HE_EXPECT(loaded.paint.IsValid());
    HE_EXPECT_EQ(loaded.paint.GetPalettes().Size(), 1u);
    HE_EXPECT_EQ(loaded.paint.GetColorGlyphs().Size(), 2u);
}

HE_TEST(scribe, runtime_blob, reject_wrong_font_face_version)
{
    schema::Builder shapingBuilder;
    const Span<const uint8_t> shapingBytes = BuildFontFaceShapingBytes(shapingBuilder);

    schema::Builder metadataBuilder;
    const Span<const uint8_t> metadataBytes = BuildFontFaceMetadataBytes(metadataBuilder);

    schema::Builder renderBuilder;
    const Span<const uint8_t> renderBytes = BuildFontFaceRenderBytes(renderBuilder);

    schema::Builder paintBuilder;
    const Span<const uint8_t> paintBytes = BuildFontFacePaintBytes(paintBuilder);

    schema::Builder rootBuilder;
    CompiledFontFaceBlob::Builder root = rootBuilder.AddStruct<CompiledFontFaceBlob>();

    RuntimeBlobHeader::Builder header = root.InitHeader();
    header.SetFormatVersion(RuntimeBlobFormatVersion + 1);
    header.SetKind(RuntimeBlobKind::FontFace);
    header.SetFlags(0);

    root.SetShapingData(rootBuilder.AddBlob(shapingBytes));
    root.SetCurveData(rootBuilder.AddBlob(BuildCurveBytes()));
    root.SetBandData(rootBuilder.AddBlob(BuildBandBytes()));
    root.SetPaintData(rootBuilder.AddBlob(paintBytes));
    root.SetMetadataData(rootBuilder.AddBlob(metadataBytes));
    root.SetRenderData(rootBuilder.AddBlob(renderBytes));
    rootBuilder.SetRoot(root);

    LoadedFontFaceBlob loaded{};
    HE_EXPECT(!LoadCompiledFontFaceBlob(loaded, Span<const schema::Word>(rootBuilder)));
}

HE_TEST(scribe, runtime_blob, build_compiled_glyph_resource_data)
{
    schema::Builder shapingBuilder;
    const Span<const uint8_t> shapingBytes = BuildFontFaceShapingBytes(shapingBuilder);

    schema::Builder metadataBuilder;
    const Span<const uint8_t> metadataBytes = BuildFontFaceMetadataBytes(metadataBuilder);

    schema::Builder renderBuilder;
    const Span<const uint8_t> renderBytes = BuildFontFaceRenderBytes(renderBuilder);

    schema::Builder paintBuilder;
    const Span<const uint8_t> paintBytes = BuildFontFacePaintBytes(paintBuilder);

    schema::Builder rootBuilder;
    CompiledFontFaceBlob::Builder root = rootBuilder.AddStruct<CompiledFontFaceBlob>();

    RuntimeBlobHeader::Builder header = root.InitHeader();
    header.SetFormatVersion(RuntimeBlobFormatVersion);
    header.SetKind(RuntimeBlobKind::FontFace);
    header.SetFlags(0);

    root.SetShapingData(rootBuilder.AddBlob(shapingBytes));
    root.SetCurveData(rootBuilder.AddBlob(BuildCurveBytes()));
    root.SetBandData(rootBuilder.AddBlob(BuildBandBytes()));
    root.SetPaintData(rootBuilder.AddBlob(paintBytes));
    root.SetMetadataData(rootBuilder.AddBlob(metadataBytes));
    root.SetRenderData(rootBuilder.AddBlob(renderBytes));
    rootBuilder.SetRoot(root);

    LoadedFontFaceBlob loaded{};
    HE_ASSERT(LoadCompiledFontFaceBlob(loaded, Span<const schema::Word>(rootBuilder)));

    CompiledGlyphResourceData glyph{};
    HE_EXPECT(BuildCompiledGlyphResourceData(glyph, loaded, 0));
    HE_EXPECT_EQ(glyph.createInfo.vertexCount, ScribeGlyphVertexCount);
    HE_EXPECT_EQ(glyph.createInfo.curveTexture.size.x, 8u);
    HE_EXPECT_EQ(glyph.createInfo.curveTexture.size.y, 1u);
    HE_EXPECT_EQ(glyph.createInfo.bandTexture.size.x, ScribeBandTextureWidth);
    HE_EXPECT_EQ(glyph.createInfo.bandTexture.size.y, 1u);
    HE_EXPECT_EQ(glyph.vertices[0].pos.x, 0.0f);
    HE_EXPECT_EQ(glyph.vertices[2].pos.y, 0.0f);
    HE_EXPECT(!BuildCompiledGlyphResourceData(glyph, loaded, 1));
}

HE_TEST(scribe, runtime_blob, resolve_compiled_color_glyph_layers)
{
    schema::Builder shapingBuilder;
    const Span<const uint8_t> shapingBytes = BuildFontFaceShapingBytes(shapingBuilder);

    schema::Builder metadataBuilder;
    const Span<const uint8_t> metadataBytes = BuildFontFaceMetadataBytes(metadataBuilder);

    schema::Builder renderBuilder;
    const Span<const uint8_t> renderBytes = BuildFontFaceRenderBytes(renderBuilder);

    schema::Builder paintBuilder;
    const Span<const uint8_t> paintBytes = BuildFontFacePaintBytes(paintBuilder);

    schema::Builder rootBuilder;
    CompiledFontFaceBlob::Builder root = rootBuilder.AddStruct<CompiledFontFaceBlob>();

    RuntimeBlobHeader::Builder header = root.InitHeader();
    header.SetFormatVersion(RuntimeBlobFormatVersion);
    header.SetKind(RuntimeBlobKind::FontFace);
    header.SetFlags(0);

    root.SetShapingData(rootBuilder.AddBlob(shapingBytes));
    root.SetCurveData(rootBuilder.AddBlob(BuildCurveBytes()));
    root.SetBandData(rootBuilder.AddBlob(BuildBandBytes()));
    root.SetPaintData(rootBuilder.AddBlob(paintBytes));
    root.SetMetadataData(rootBuilder.AddBlob(metadataBytes));
    root.SetRenderData(rootBuilder.AddBlob(renderBytes));
    rootBuilder.SetRoot(root);

    LoadedFontFaceBlob loaded{};
    HE_ASSERT(LoadCompiledFontFaceBlob(loaded, Span<const schema::Word>(rootBuilder)));

    Vector<CompiledColorGlyphLayer> layers{};
    HE_EXPECT(GetCompiledColorGlyphLayers(layers, loaded, 0, 0, { 0.5f, 0.25f, 0.75f, 1.0f }));
    HE_EXPECT_EQ(layers.Size(), 2u);
    HE_EXPECT_EQ(layers[0].glyphIndex, 0u);
    HE_EXPECT_EQ(layers[0].color.x, 1.0f);
    HE_EXPECT_EQ(layers[0].color.y, 0.0f);
    HE_EXPECT_EQ(layers[0].color.w, 0.75f);
    HE_EXPECT_EQ(layers[0].basisX.x, 1.0f);
    HE_EXPECT_EQ(layers[0].basisX.y, 0.0f);
    HE_EXPECT_EQ(layers[0].basisY.x, 0.0f);
    HE_EXPECT_EQ(layers[0].basisY.y, 1.0f);
    HE_EXPECT_EQ(layers[0].offset.x, 12.0f);
    HE_EXPECT_EQ(layers[0].offset.y, -6.0f);
    HE_EXPECT_EQ(layers[1].color.x, 0.5f);
    HE_EXPECT_EQ(layers[1].color.y, 0.25f);
    HE_EXPECT_EQ(layers[1].color.w, 0.5f);
    HE_EXPECT_EQ(layers[1].basisX.x, 0.5f);
    HE_EXPECT_EQ(layers[1].basisX.y, -0.75f);
    HE_EXPECT_EQ(layers[1].basisY.x, 0.25f);
    HE_EXPECT_EQ(layers[1].basisY.y, 1.5f);
    HE_EXPECT_EQ(layers[1].offset.x, 3.0f);
    HE_EXPECT_EQ(layers[1].offset.y, 9.0f);
}
