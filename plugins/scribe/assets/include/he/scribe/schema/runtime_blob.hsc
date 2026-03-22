// Copyright Chad Engler

@0x970edff4ba4b6a8c;

import "he/schema/schema.hsc";

namespace he.scribe;

enum FontSourceFormat
{
    Unknown @0;
    TrueType @1;
    OpenTypeCff @2;
    TrueTypeCollection @3;
    OpenTypeCollection @4;
}

enum RuntimeBlobKind
{
    Unknown @0;
    FontFace @1;
    FontFamily @2;
    VectorImage @3;
}

enum FillRule
{
    NonZero @0;
    EvenOdd @1;
}

struct RuntimeBlobHeader
{
    formatVersion @0 :uint32;
    kind @1 :RuntimeBlobKind;
    flags @2 :uint32;
}

struct FontFaceMetrics
{
    unitsPerEm @0 :uint32;
    ascender @1 :int32;
    descender @2 :int32;
    lineHeight @3 :int32;
    maxAdvanceWidth @4 :uint32;
    maxAdvanceHeight @5 :uint32;
    capHeight @6 :int32;
}

struct FontFaceImportMetadata
{
    faceIndex @0 :uint32;
    sourceFormat @1 :FontSourceFormat;
    familyName @2 :String;
    styleName @3 :String;
    postscriptName @4 :String;
    glyphCount @5 :uint32;
    metrics @6 :FontFaceMetrics;
    isScalable @7 :bool;
    hasColorGlyphs @8 :bool;
    hasKerning @9 :bool;
    hasHorizontalLayout @10 :bool;
    hasVerticalLayout @11 :bool;
}

struct FontFaceShapingData
{
    faceIndex @0 :uint32;
    sourceFormat @1 :FontSourceFormat;
    sourceBytes @2 :Blob;
}

struct FontFaceGlyphRenderData
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
    flags @15 :uint32;
}

struct FontFacePaletteColor
{
    red @0 :float32;
    green @1 :float32;
    blue @2 :float32;
    alpha @3 :float32;
}

struct FontFacePalette
{
    flags @0 :uint32;
    colors @1 :FontFacePaletteColor[];
}

struct FontFaceColorGlyphLayer
{
    glyphIndex @0 :uint32;
    paletteEntryIndex @1 :uint32;
    flags @2 :uint32;
    alphaScale @3 :float32;
    transform00 @4 :float32;
    transform01 @5 :float32;
    transform10 @6 :float32;
    transform11 @7 :float32;
    transformTx @8 :float32;
    transformTy @9 :float32;
}

struct FontFaceColorGlyph
{
    firstLayer @0 :uint32;
    layerCount @1 :uint32;
}

struct FontFaceRenderData
{
    curveTextureWidth @0 :uint32;
    curveTextureHeight @1 :uint32;
    bandTextureWidth @2 :uint32;
    bandTextureHeight @3 :uint32;
    bandOverlapEpsilon @4 :float32;
    glyphs @5 :FontFaceGlyphRenderData[];
}

struct FontFacePaintData
{
    defaultPaletteIndex @0 :uint32;
    palettes @1 :FontFacePalette[];
    colorGlyphs @2 :FontFaceColorGlyph[];
    layers @3 :FontFaceColorGlyphLayer[];
}

struct FontFamilyRuntimeData
{
    faceAssets @0 :he.schema.Uuid[];
}

struct VectorImageRuntimeMetadata
{
    sourceViewBoxMinX @0 :float32;
    sourceViewBoxMinY @1 :float32;
    sourceViewBoxWidth @2 :float32;
    sourceViewBoxHeight @3 :float32;
    sourceBoundsMinX @4 :float32;
    sourceBoundsMinY @5 :float32;
    sourceBoundsMaxX @6 :float32;
    sourceBoundsMaxY @7 :float32;
}

struct VectorImageShapeRenderData
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
    flags @13 :uint32;
}

struct VectorImageRenderData
{
    curveTextureWidth @0 :uint32;
    curveTextureHeight @1 :uint32;
    bandTextureWidth @2 :uint32;
    bandTextureHeight @3 :uint32;
    bandOverlapEpsilon @4 :float32;
    shapes @5 :VectorImageShapeRenderData[];
}

struct VectorImageLayer
{
    shapeIndex @0 :uint32;
    red @1 :float32;
    green @2 :float32;
    blue @3 :float32;
    alpha @4 :float32;
}

struct VectorImagePaintData
{
    layers @0 :VectorImageLayer[];
}

struct CompiledFontFaceBlob
{
    const ResourceName :String = "he.scribe.font_face.runtime_blob";

    header @0 :RuntimeBlobHeader;
    shapingData @1 :Blob;
    curveData @2 :Blob;
    bandData @3 :Blob;
    paintData @4 :Blob;
    metadataData @5 :Blob;
    renderData @6 :Blob;
}

struct CompiledFontFamilyBlob
{
    const ResourceName :String = "he.scribe.font_family.runtime_blob";

    header @0 :RuntimeBlobHeader;
    familyData @1 :Blob;
}

struct CompiledVectorImageBlob
{
    const ResourceName :String = "he.scribe.vector_image.runtime_blob";

    header @0 :RuntimeBlobHeader;
    curveData @1 :Blob;
    bandData @2 :Blob;
    paintData @3 :Blob;
    metadataData @4 :Blob;
    renderData @5 :Blob;
}

struct ScribeFontFace
{
    const AssetTypeName :String = "he.scribe.font_face";
    const ImportSourceResourceName :String = "he.scribe.font_face.import_source";
    const ImportMetadataResourceName :String = "he.scribe.font_face.import_metadata";
    const RuntimeBlobResourceName :String = "he.scribe.font_face.runtime_blob";

    struct ImportSourceResource
    {
        sourceFormat @0 :FontSourceFormat;
        sourceBytes @1 :Blob;
        sourceFileName @2 :String;
        faceCount @3 :uint32;
    }

    struct ImportMetadataResource
    {
        metadata @0 :FontFaceImportMetadata;
    }

    faceIndex @0 :uint32 = 0;
    preserveSourceBytesForShaping @1 :bool = true;
    familyName @2 :String;
    styleName @3 :String;
    postscriptName @4 :String;
    sourceFormat @5 :FontSourceFormat = FontSourceFormat.Unknown;
    glyphCount @6 :uint32 = 0;
    metrics @7 :FontFaceMetrics;
    isScalable @8 :bool = true;
    hasColorGlyphs @9 :bool = false;
    hasKerning @10 :bool = false;
    hasHorizontalLayout @11 :bool = true;
    hasVerticalLayout @12 :bool = false;
}

struct ScribeFontFamily
{
    const AssetTypeName :String = "he.scribe.font_family";
    const RuntimeBlobResourceName :String = "he.scribe.font_family.runtime_blob";

    faces @0 :he.schema.Uuid[];
}

struct ScribeImage
{
    const AssetTypeName :String = "he.scribe.image";
    const ImportSourceResourceName :String = "he.scribe.image.import_source";
    const ImportMetadataResourceName :String = "he.scribe.image.import_metadata";
    const RuntimeBlobResourceName :String = "he.scribe.vector_image.runtime_blob";

    struct ImportSourceResource
    {
        sourceBytes @0 :Blob;
        sourceFileName @1 :String;
    }

    struct ImportMetadataResource
    {
        sourceViewBoxMinX @0 :float32;
        sourceViewBoxMinY @1 :float32;
        sourceViewBoxWidth @2 :float32;
        sourceViewBoxHeight @3 :float32;
    }

    flatteningTolerance @0 :float32 = 0.25;
    preserveStrokes @1 :bool = true;
}
