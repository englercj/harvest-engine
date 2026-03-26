// Copyright Chad Engler

@0x970edff4ba4b6a8c;

import "he/schema/schema.hsc";
import "he/assets/asset_types.hsc";
import "he/editor/schema/display_attributes.hsc";

namespace he.scribe;

alias Display = he.editor.Display;

enum FillRule
{
    NonZero @0;
    EvenOdd @1;
}

enum StrokeCommandType
{
    MoveTo @0;
    LineTo @1;
    QuadraticTo @2;
    Close @3;
}

struct StrokePoint
{
    x @0 :int32;
    y @1 :int32;
}

struct StrokeCommand
{
    type @0 :StrokeCommandType;
    firstPoint @1 :uint32;
}

struct ScribeFontFace $he.assets.AssetType $Display.ImportOnly $Display.Description("A compiled font face used by Scribe.")
{
    const AssetTypeName :String = "he.scribe.font_face";
    const ImportSourceResourceName :String = "he.scribe.font_face.import_source";
    const RuntimeResourceName :String = "he.scribe.font_face.runtime_resource";

    enum SourceFormat
    {
        Unknown @0;
        TrueType @1;
        OpenTypeCff @2;
        TrueTypeCollection @3;
        OpenTypeCollection @4;
    }

    enum PaletteBackground
    {
        Unspecified @0;
        Light @1;
        Dark @2;
        Any @3;
    }

    enum ColorSource
    {
        Palette @0;
        Foreground @1;
    }

    struct Metrics
    {
        unitsPerEm @0 :uint32;
        ascender @1 :int32;
        descender @2 :int32;
        lineHeight @3 :int32;
        capHeight @4 :int32;
    }

    struct GlyphRenderData
    {
        advanceX @0 :int32;
        advanceY @1 :int32;
        boundsMinX @2 :float32;
        boundsMinY @3 :float32;
        boundsMaxX @4 :float32;
        boundsMaxY @5 :float32;
        bandScaleX @6 :float32;
        bandScaleY @7 :float32;
        bandOffsetX @8 :float32;
        bandOffsetY @9 :float32;
        glyphBandLocX @10 :uint32;
        glyphBandLocY @11 :uint32;
        bandMaxX @12 :uint32;
        bandMaxY @13 :uint32;
        fillRule @14 :FillRule;
        hasGeometry @15 :bool;
        hasColorLayers @16 :bool;
        firstStrokeCommand @17 :uint32;
        strokeCommandCount @18 :uint32;
    }

    struct PaletteColor
    {
        red @0 :float32;
        green @1 :float32;
        blue @2 :float32;
        alpha @3 :float32;
    }

    struct Palette
    {
        background @0 :PaletteBackground = PaletteBackground.Unspecified;
        colors @1 :PaletteColor[];
    }

    struct ColorGlyphLayer
    {
        glyphIndex @0 :uint32;
        paletteEntryIndex @1 :uint32;
        colorSource @2 :ColorSource = ColorSource.Palette;
        alphaScale @3 :float32;
        transform00 @4 :float32;
        transform01 @5 :float32;
        transform10 @6 :float32;
        transform11 @7 :float32;
        transformTx @8 :float32;
        transformTy @9 :float32;
    }

    struct ColorGlyph
    {
        firstLayer @0 :uint32;
        layerCount @1 :uint32;
    }

    struct RuntimeResource
    {
        const ResourceName :String = "he.scribe.font_face.runtime_resource";

        shaping :group
        {
            faceIndex @0 :uint32;
            sourceBytes @1 :Blob;
        }

        paint :group
        {
            defaultPaletteIndex @2 :uint32;
            palettes @3 :Palette[];
            colorGlyphs @4 :ColorGlyph[];
            layers @5 :ColorGlyphLayer[];
        }

        metadata :group
        {
            glyphCount @6 :uint32;
            unitsPerEm @7 :uint32;
            ascender @8 :int32;
            descender @9 :int32;
            lineHeight @10 :int32;
            capHeight @11 :int32;
            hasColorGlyphs @12 :bool;
        }

        fill :group
        {
            curveData @13 :Blob;
            bandData @14 :Blob;
            curveTextureWidth @15 :uint32;
            curveTextureHeight @16 :uint32;
            bandTextureWidth @17 :uint32;
            bandTextureHeight @18 :uint32;
            bandOverlapEpsilon @19 :float32;
            glyphs @20 :GlyphRenderData[];
        }

        stroke :group
        {
            pointScale @21 :float32;
            points @22 :StrokePoint[];
            commands @23 :StrokeCommand[];
        }
    }

    struct ImportSourceResource
    {
        sourceBytes @0 :Blob;
    }

    faceIndex @0 :uint32 = 0;
    metrics @1 :Metrics;
    hasColorGlyphs @2 :bool = false;
}

struct ScribeFontFamily $he.assets.AssetType $Display.Description("A collection of Scribe font face assets.")
{
    const AssetTypeName :String = "he.scribe.font_family";
    const RuntimeResourceName :String = "he.scribe.font_family.runtime_resource";

    struct RuntimeResource
    {
        const ResourceName :String = "he.scribe.font_family.runtime_resource";

        faceAssets @0 :he.schema.Uuid[];
    }

    faces @0 :he.schema.Uuid[] $he.assets.AssetRef(ScribeFontFace.AssetTypeName);
}

struct ScribeImage $he.assets.AssetType $Display.ImportOnly $Display.Description("A retained vector image used by Scribe.")
{
    const AssetTypeName :String = "he.scribe.image";
    const ImportSourceResourceName :String = "he.scribe.image.import_source";
    const RuntimeResourceName :String = "he.scribe.vector_image.runtime_resource";

    struct ShapeRenderData
    {
        boundsMinX @0 :float32;
        boundsMinY @1 :float32;
        boundsMaxX @2 :float32;
        boundsMaxY @3 :float32;
        bandScaleX @4 :float32;
        bandScaleY @5 :float32;
        bandOffsetX @6 :float32;
        bandOffsetY @7 :float32;
        glyphBandLocX @8 :uint32;
        glyphBandLocY @9 :uint32;
        bandMaxX @10 :uint32;
        bandMaxY @11 :uint32;
        fillRule @12 :FillRule;
        firstStrokeCommand @13 :uint32;
        strokeCommandCount @14 :uint32;
    }

    struct Layer
    {
        shapeIndex @0 :uint32;
        red @1 :float32;
        green @2 :float32;
        blue @3 :float32;
        alpha @4 :float32;
    }

    struct RuntimeResource
    {
        const ResourceName :String = "he.scribe.vector_image.runtime_resource";

        paint :group
        {
            layers @0 :Layer[];
        }

        metadata :group
        {
            sourceViewBoxMinX @1 :float32;
            sourceViewBoxMinY @2 :float32;
            sourceViewBoxWidth @3 :float32;
            sourceViewBoxHeight @4 :float32;
            sourceBoundsMinX @5 :float32;
            sourceBoundsMinY @6 :float32;
            sourceBoundsMaxX @7 :float32;
            sourceBoundsMaxY @8 :float32;
        }

        fill :group
        {
            curveData @9 :Blob;
            bandData @10 :Blob;
            curveTextureWidth @11 :uint32;
            curveTextureHeight @12 :uint32;
            bandTextureWidth @13 :uint32;
            bandTextureHeight @14 :uint32;
            bandOverlapEpsilon @15 :float32;
            shapes @16 :ShapeRenderData[];
        }

        stroke :group
        {
            pointScale @17 :float32;
            points @18 :StrokePoint[];
            commands @19 :StrokeCommand[];
        }
    }

    struct ImportSourceResource
    {
        sourceBytes @0 :Blob;
    }

    flatteningTolerance @0 :float32 = 0.25;
}
