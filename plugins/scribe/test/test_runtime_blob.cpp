// Copyright Chad Engler

#include "he/scribe/compiled_font.h"
#include "he/scribe/packed_data.h"
#include "he/scribe/schema_types.h"

#include "he/core/test.h"

using namespace he;
using namespace he::scribe;

namespace
{
    constexpr float kStrokePointScale = 1.0f / 256.0f;

    void FillFontFaceShapingData(FontFaceShapingData::Builder shaping, schema::Builder& builder)
    {
        shaping.SetFaceIndex(2);
        shaping.SetSourceBytes(builder.AddBlob({ reinterpret_cast<const uint8_t*>("abc"), 3 }));
    }

    void FillFontFaceMetadata(FontFaceRuntimeMetadata::Builder metadata)
    {
        metadata.SetGlyphCount(42);
        metadata.SetUnitsPerEm(1000);
        metadata.SetAscender(800);
        metadata.SetDescender(-200);
        metadata.SetLineHeight(1200);
        metadata.SetCapHeight(700);
        metadata.SetHasColorGlyphs(false);
    }

    void FillFontFaceFillData(FontFaceFillData::Builder fill)
    {
        fill.SetCurveTextureWidth(8);
        fill.SetCurveTextureHeight(1);
        fill.SetBandTextureWidth(ScribeBandTextureWidth);
        fill.SetBandTextureHeight(1);
        fill.SetBandOverlapEpsilon(1.0f);

        schema::List<FontFaceGlyphRenderData>::Builder glyphs = fill.InitGlyphs(2);

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
            glyph.SetHasGeometry(true);
            glyph.SetHasColorLayers(false);
            glyph.SetFirstStrokeCommand(0);
            glyph.SetStrokeCommandCount(5);
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
            glyph.SetHasGeometry(false);
            glyph.SetHasColorLayers(false);
            glyph.SetFirstStrokeCommand(5);
            glyph.SetStrokeCommandCount(0);
        }
    }

    void FillFontFaceStrokeData(FontFaceStrokeData::Builder stroke)
    {
        stroke.SetPointScale(kStrokePointScale);

        schema::List<StrokePoint>::Builder points = stroke.InitPoints(4);
        points[0].SetX(0);
        points[0].SetY(0);
        points[1].SetX(256);
        points[1].SetY(0);
        points[2].SetX(256);
        points[2].SetY(256);
        points[3].SetX(0);
        points[3].SetY(256);

        schema::List<StrokeCommand>::Builder commands = stroke.InitCommands(5);
        commands[0].SetType(StrokeCommandType::MoveTo);
        commands[0].SetFirstPoint(0);
        commands[1].SetType(StrokeCommandType::LineTo);
        commands[1].SetFirstPoint(1);
        commands[2].SetType(StrokeCommandType::LineTo);
        commands[2].SetFirstPoint(2);
        commands[3].SetType(StrokeCommandType::LineTo);
        commands[3].SetFirstPoint(3);
        commands[4].SetType(StrokeCommandType::Close);
        commands[4].SetFirstPoint(4);
    }

    void FillFontFacePaintData(FontFacePaintData::Builder paint)
    {
        paint.SetDefaultPaletteIndex(0);

        schema::List<FontFacePalette>::Builder palettes = paint.InitPalettes(1);
        {
            FontFacePalette::Builder palette = palettes[0];
            palette.SetBackground(FontFacePaletteBackground::Dark);

            schema::List<FontFacePaletteColor>::Builder colors = palette.InitColors(2);
            colors[0].SetRed(1.0f);
            colors[0].SetGreen(0.0f);
            colors[0].SetBlue(0.0f);
            colors[0].SetAlpha(1.0f);

            colors[1].SetRed(0.0f);
            colors[1].SetGreen(0.6f);
            colors[1].SetBlue(1.0f);
            colors[1].SetAlpha(1.0f);
        }

        schema::List<FontFaceColorGlyph>::Builder colorGlyphs = paint.InitColorGlyphs(2);
        colorGlyphs[0].SetFirstLayer(0);
        colorGlyphs[0].SetLayerCount(2);
        colorGlyphs[1].SetFirstLayer(2);
        colorGlyphs[1].SetLayerCount(0);

        schema::List<FontFaceColorGlyphLayer>::Builder layers = paint.InitLayers(2);
        layers[0].SetGlyphIndex(0);
        layers[0].SetPaletteEntryIndex(0);
        layers[0].SetColorSource(FontFaceColorSource::Palette);
        layers[0].SetAlphaScale(0.75f);
        layers[0].SetTransform00(1.0f);
        layers[0].SetTransform01(0.0f);
        layers[0].SetTransform10(0.0f);
        layers[0].SetTransform11(1.0f);
        layers[0].SetTransformTx(12.0f);
        layers[0].SetTransformTy(-6.0f);
        layers[1].SetGlyphIndex(0);
        layers[1].SetPaletteEntryIndex(0);
        layers[1].SetColorSource(FontFaceColorSource::Foreground);
        layers[1].SetAlphaScale(0.5f);
        layers[1].SetTransform00(0.5f);
        layers[1].SetTransform01(0.25f);
        layers[1].SetTransform10(-0.75f);
        layers[1].SetTransform11(1.5f);
        layers[1].SetTransformTx(3.0f);
        layers[1].SetTransformTy(9.0f);
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

HE_TEST(scribe, runtime_resource, load_compiled_font_face_success)
{
    schema::Builder rootBuilder;
    FontFaceResource::Builder root = rootBuilder.AddStruct<FontFaceResource>();

    FillFontFaceShapingData(root.GetShaping(), rootBuilder);
    root.GetFill().SetCurveData(rootBuilder.AddBlob(BuildCurveBytes()));
    root.GetFill().SetBandData(rootBuilder.AddBlob(BuildBandBytes()));
    FillFontFacePaintData(root.GetPaint());
    FillFontFaceMetadata(root.GetMetadata());
    FillFontFaceFillData(root.GetFill());
    FillFontFaceStrokeData(root.GetStroke());
    rootBuilder.SetRoot(root);

    const FontFaceResourceReader loaded = schema::ReadRoot<FontFaceResource>(Span<const schema::Word>(rootBuilder).Data());
    const bool ok = loaded.IsValid();
    const FontFaceRuntimeMetadata::Reader metadata = loaded.GetMetadata();
    const FontFaceShapingData::Reader shaping = loaded.GetShaping();
    const FontFaceFillData::Reader fill = loaded.GetFill();
    const FontFacePaintData::Reader paint = loaded.GetPaint();

    HE_EXPECT(ok);
    HE_EXPECT(loaded.IsValid());
    HE_EXPECT_EQ(metadata.GetGlyphCount(), 42u);
    HE_EXPECT_EQ(metadata.GetUnitsPerEm(), 1000u);
    HE_EXPECT_EQ(metadata.GetCapHeight(), 700);
    HE_EXPECT_EQ(shaping.GetFaceIndex(), 2u);
    HE_EXPECT_EQ(shaping.GetSourceBytes().Size(), 3u);
    HE_EXPECT_EQ(fill.GetGlyphs().Size(), 2u);
    HE_EXPECT_EQ(fill.GetGlyphs()[0].GetStrokeCommandCount(), 5u);
    HE_EXPECT_EQ(fill.GetCurveTextureWidth(), 8u);
    HE_EXPECT_EQ(fill.GetBandTextureWidth(), ScribeBandTextureWidth);
    HE_EXPECT_EQ(loaded.GetStroke().GetPoints().Size(), 4u);
    HE_EXPECT_EQ(loaded.GetStroke().GetCommands().Size(), 5u);
    HE_EXPECT_EQ(loaded.GetStroke().GetPointScale(), kStrokePointScale);
    HE_EXPECT(paint.IsValid());
    HE_EXPECT_EQ(paint.GetPalettes().Size(), 1u);
    HE_EXPECT_EQ(paint.GetColorGlyphs().Size(), 2u);
}

HE_TEST(scribe, runtime_resource, reject_font_face_with_mismatched_render_payload_size)
{
    schema::Builder rootBuilder;
    FontFaceResource::Builder root = rootBuilder.AddStruct<FontFaceResource>();

    FillFontFaceShapingData(root.GetShaping(), rootBuilder);
    root.GetFill().SetCurveData(rootBuilder.AddBlob(Span<const uint8_t>{}));
    root.GetFill().SetBandData(rootBuilder.AddBlob(BuildBandBytes()));
    FillFontFacePaintData(root.GetPaint());
    FillFontFaceMetadata(root.GetMetadata());
    FillFontFaceFillData(root.GetFill());
    FillFontFaceStrokeData(root.GetStroke());
    rootBuilder.SetRoot(root);

    const FontFaceResourceReader loaded = schema::ReadRoot<FontFaceResource>(Span<const schema::Word>(rootBuilder).Data());
    HE_EXPECT(loaded.IsValid());
}

HE_TEST(scribe, runtime_resource, build_compiled_glyph_resource_data)
{
    schema::Builder rootBuilder;
    FontFaceResource::Builder root = rootBuilder.AddStruct<FontFaceResource>();

    FillFontFaceShapingData(root.GetShaping(), rootBuilder);
    root.GetFill().SetCurveData(rootBuilder.AddBlob(BuildCurveBytes()));
    root.GetFill().SetBandData(rootBuilder.AddBlob(BuildBandBytes()));
    FillFontFacePaintData(root.GetPaint());
    FillFontFaceMetadata(root.GetMetadata());
    FillFontFaceFillData(root.GetFill());
    FillFontFaceStrokeData(root.GetStroke());
    rootBuilder.SetRoot(root);

    const FontFaceResourceReader loaded = schema::ReadRoot<FontFaceResource>(Span<const schema::Word>(rootBuilder).Data());
    HE_ASSERT(loaded.IsValid());

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

HE_TEST(scribe, runtime_resource, build_compiled_stroked_glyph_resource_data)
{
    schema::Builder rootBuilder;
    FontFaceResource::Builder root = rootBuilder.AddStruct<FontFaceResource>();

    FillFontFaceShapingData(root.GetShaping(), rootBuilder);
    root.GetFill().SetCurveData(rootBuilder.AddBlob(BuildCurveBytes()));
    root.GetFill().SetBandData(rootBuilder.AddBlob(BuildBandBytes()));
    FillFontFacePaintData(root.GetPaint());
    FillFontFaceMetadata(root.GetMetadata());
    FillFontFaceFillData(root.GetFill());
    FillFontFaceStrokeData(root.GetStroke());
    rootBuilder.SetRoot(root);

    const FontFaceResourceReader loaded = schema::ReadRoot<FontFaceResource>(Span<const schema::Word>(rootBuilder).Data());
    HE_ASSERT(loaded.IsValid());

    CompiledStrokedGlyphResourceData glyph{};
    StrokeStyle stroke{};
    stroke.width = 0.5f;
    stroke.joinStyle = StrokeJoinStyle::Round;
    stroke.capStyle = StrokeCapStyle::Round;
    stroke.miterLimit = 4.0f;

    HE_EXPECT(BuildCompiledStrokedGlyphResourceData(glyph, loaded, 0, stroke));
    HE_EXPECT_EQ(glyph.createInfo.vertexCount, ScribeGlyphVertexCount);
    HE_EXPECT_GT(glyph.createInfo.curveTexture.size.x, 0u);
    HE_EXPECT_GT(glyph.createInfo.curveTexture.size.y, 0u);
    HE_EXPECT_EQ(glyph.createInfo.bandTexture.size.x, ScribeBandTextureWidth);
    HE_EXPECT_GT(glyph.createInfo.bandTexture.size.y, 0u);
    HE_EXPECT_LT(glyph.vertices[0].pos.x, 0.0f);
    HE_EXPECT_GT(glyph.vertices[2].pos.x, 1.0f);
}

HE_TEST(scribe, runtime_resource, resolve_compiled_color_glyph_layers)
{
    schema::Builder rootBuilder;
    FontFaceResource::Builder root = rootBuilder.AddStruct<FontFaceResource>();

    FillFontFaceShapingData(root.GetShaping(), rootBuilder);
    root.GetFill().SetCurveData(rootBuilder.AddBlob(BuildCurveBytes()));
    root.GetFill().SetBandData(rootBuilder.AddBlob(BuildBandBytes()));
    FillFontFacePaintData(root.GetPaint());
    FillFontFaceMetadata(root.GetMetadata());
    FillFontFaceFillData(root.GetFill());
    rootBuilder.SetRoot(root);

    const FontFaceResourceReader loaded = schema::ReadRoot<FontFaceResource>(Span<const schema::Word>(rootBuilder).Data());
    HE_ASSERT(loaded.IsValid());

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
