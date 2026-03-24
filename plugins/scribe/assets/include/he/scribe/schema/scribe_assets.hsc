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
        maxAdvanceWidth @4 :uint32;
        maxAdvanceHeight @5 :uint32;
        capHeight @6 :int32;
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
            sourceFormat @1 :SourceFormat;
            sourceBytes @2 :Blob;
        }

        curveData @3 :Blob;
        bandData @4 :Blob;

        paint :group
        {
            defaultPaletteIndex @5 :uint32;
            palettes @6 :Palette[];
            colorGlyphs @7 :ColorGlyph[];
            layers @8 :ColorGlyphLayer[];
        }

        metadata :group
        {
            glyphCount @9 :uint32;
            unitsPerEm @10 :uint32;
            ascender @11 :int32;
            descender @12 :int32;
            lineHeight @13 :int32;
            capHeight @14 :int32;
            hasColorGlyphs @15 :bool;
        }

        render :group
        {
            curveTextureWidth @16 :uint32;
            curveTextureHeight @17 :uint32;
            bandTextureWidth @18 :uint32;
            bandTextureHeight @19 :uint32;
            bandOverlapEpsilon @20 :float32;
            glyphs @21 :GlyphRenderData[];
        }
    }

    struct ImportSourceResource
    {
        sourceBytes @0 :Blob;
    }

    faceIndex @0 :uint32 = 0;
    familyName @1 :String;
    styleName @2 :String;
    postscriptName @3 :String;
    sourceFormat @4 :SourceFormat = SourceFormat.Unknown;
    glyphCount @5 :uint32 = 0;
    metrics @6 :Metrics;
    isScalable @7 :bool = true;
    hasColorGlyphs @8 :bool = false;
    hasKerning @9 :bool = false;
    hasHorizontalLayout @10 :bool = true;
    hasVerticalLayout @11 :bool = false;
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

        curveData @0 :Blob;
        bandData @1 :Blob;

        paint :group
        {
            layers @2 :Layer[];
        }

        metadata :group
        {
            sourceViewBoxMinX @3 :float32;
            sourceViewBoxMinY @4 :float32;
            sourceViewBoxWidth @5 :float32;
            sourceViewBoxHeight @6 :float32;
            sourceBoundsMinX @7 :float32;
            sourceBoundsMinY @8 :float32;
            sourceBoundsMaxX @9 :float32;
            sourceBoundsMaxY @10 :float32;
        }

        render :group
        {
            curveTextureWidth @11 :uint32;
            curveTextureHeight @12 :uint32;
            bandTextureWidth @13 :uint32;
            bandTextureHeight @14 :uint32;
            bandOverlapEpsilon @15 :float32;
            shapes @16 :ShapeRenderData[];
        }
    }

    struct ImportSourceResource
    {
        sourceBytes @0 :Blob;
    }

    flatteningTolerance @0 :float32 = 0.25;
}
