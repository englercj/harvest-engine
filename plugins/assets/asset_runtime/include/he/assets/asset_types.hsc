// Copyright Chad Engler

@0xc6d3b56c92a52ec3;

import "he/schema/schema.hsc";

namespace he.assets.schema;

// ------------------------------------------------------------------------------------------------
// Common Structures

struct Vec2f
{
    x @0 :float32;
    y @1 :float32;
}

struct Vec3f
{
    x @0 :float32;
    y @1 :float32;
    z @2 :float32;
}

struct ScalarRange
{
    data :union
    {
        int8 :group
        {
            min @0 :int8;
            max @1 :int8;
        }

        int16 :group
        {
            min @2 :int16;
            max @3 :int16;
        }

        int32 :group
        {
            min @4 :int32;
            max @5 :int32;
        }

        int64 :group
        {
            min @6 :int64;
            max @7 :int64;
        }

        uint8 :group
        {
            min @8 :uint8;
            max @9 :uint8;
        }

        uint16 :group
        {
            min @10 :uint16;
            max @11 :uint16;
        }

        uint32 :group
        {
            min @12 :uint32;
            max @13 :uint32;
        }

        uint64 :group
        {
            min @14 :uint64;
            max @15 :uint64;
        }

        float32 :group
        {
            min @16 :float32;
            max @17 :float32;
        }

        float64 :group
        {
            min @18 :float64;
            max @19 :float64;
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Display Attributes

// TODO: This should probably live somewhere else? It isn't asset-specific.
// It can't live in the editor though, because we need it here and can't depend on editor...

struct Display
{
    // hides an asset type from the list of types that the editor can create directly.
    attribute ImportOnly(struct) :void;

    // Hides the field from display. The data is still persisted and available to code.
    // The editor property grids will skip fields with this attribute.
    attribute Hidden(field) :void;

    // Shows the field, but does not allow it to be edited. The data can still be changed by code.
    attribute ReadOnly(field) :void;

    // Human-friendly display name for the target.
    // The editor uses this value as an override for the name during display.
    attribute Name(enum, enumerator, field, struct) :String;

    // Human-friendly description text for the target.
    // The editor will use this value as a tooltip.
    attribute Description(enum, enumerator, field, struct) :String;

    // Marks a field as referring to an asset of a particular type.
    // Only meaningful for `he.schema.Uuid` fields.
    attribute Asset(field) :String;

    // Allows for multi-line editing of a String field.
    attribute Multiline(field) :void;

    // Promotes fields of the type to be displayed as if they are fields of the parent.
    attribute Promote(field) :void;

    // Displays a slider edit control for this field.
    attribute Slider(field) :ScalarRange;

    // Sets a minimum and maximum value that can be selected in the UI for an integer value.
    attribute Clamp(field) :ScalarRange;
}

// ------------------------------------------------------------------------------------------------
// Base Asset Structures

// Marks a structure as the definition for an asset type.
// That structure is expected to have a constant named `AssetTypeName` that is a unique
// string that will be used as the asset type identifier.
attribute AssetType(struct) :void;

struct Asset
{
    uuid @0 :he.schema.Uuid $Display.ReadOnly;          // unique identifier of the asset
    type @1 :String $Display.ReadOnly;                  // unique string identifier of the asset type
    name @2 :String;                                    // user-defined human-friendly name
    tags @3 :String[];                                  // user-defined search & filter strings
    references @4 :he.schema.Uuid[] $Display.Hidden;    // outgoing references to other assets
    data @5 :AnyStruct;                                 // Data for the asset, type is deduced based on `type`
    importData @6 :AnyStruct $Display.Hidden;           // Data for the importer, stores import settings for re-importing
}

struct AssetFile
{
    uuid @0 :he.schema.Uuid;    // unique identifier of the file
    assets @1 :Asset[];         // list of assets contained in the file
    source @2 :String;          // relative path to source file for the assets
}

// ------------------------------------------------------------------------------------------------
// Built-in Asset Types

struct Texture2D $AssetType $Display.ImportOnly $Display.Description("A two dimensional texture.")
{
    const AssetTypeName :String = "he.asset.texture2d";
    const PixelsResourceName :String = "he.asset.texture.pixels";
    const Ktx2ResourceName :String = "he.asset.texture.ktx2";

    struct PixelsResource
    {
        width @0 :uint32;
        height @1 :uint32;
        source @2 :Blob;
        mips @3 :Blob[];
    }

    enum MipMapFilter
    {
        Box @0;
        Tent @1;
        Bell @2;
        BSpline @3;
        Mitchell @4;
        Blackman @5;
        Lanczos3 @6;
        Lanczos4 @7;
        Lanczos6 @8;
        Lanczos12 @9;
        Kaiser @10;
        Gaussian @11;
        CatmullRom @12;
        QuadraticInterp @13;
        QuadraticApprox @14;
        QuadraticMix @15;
    }

    enum CompressionFormat
    {
        ETC1S @0 $Display.Description("Highly compressed, similar to BC1/4.");
        UASTC @1 $Display.Description("Extremely high quality, similar to BC7.");
    }

    enum CompressionQuality
    {
        VeryLow @0;
        Low @1;
        Normal @2 $Display.Description("Balanced choice between compressed size and quality.");
        High @3;
        VeryHigh @4;
    }

    flipY @0 :bool = false $Display.Description("When true the imported image is flipped along the Y axis.");
    sRGB @1 :bool = true $Display.Name("sRGB") $Display.Description("When true the sRGB color space is used, otherwise linear space is used.");

    mipMapping :group
    {
        generate @2 :bool = true $Display.Description("Enables automatic generation of mips.");
        scale @3 :float32 = 1.0 $Display.Clamp({ data = { float32 = { min = 0.000125, max = 4.0 } } }) $Display.Description("Set mipmap filter kernel's scale, lower=sharper, higher=more blurry");
        filter @4 :MipMapFilter = MipMapFilter.Kaiser $Display.Description("The filtering mechanism to use when generating mips.");
        sRGB @5 :bool = false $Display.Name("sRGB") $Display.Description("Convert image to linear before filtering, then back to sRGB.");
        renormalize @6 :bool = false $Display.Description("Renormalize normal map to unit length vectors after filtering.");
        clamp @7 :bool = false $Display.Description("Use clamp addressing on borders, instead of wrapping.");
        sampleFirst @8 :bool = false $Display.Description("Always first/largest mip level when generating a mip. Results in slower mip map generation.");
        smallestDimension @9 :int32 = 1 $Display.Clamp({ data = { int32 = { min = 1, max = 16384 } } }) $Display.Description("Set smallest pixel dimension for generated mipmaps.");
    }

    compression :group
    {
        format @10 :CompressionFormat = CompressionFormat.ETC1S;
        quality @11 :CompressionQuality = CompressionQuality.Normal;
        level @12 :int8 = 12 $Display.Clamp({ data = { int8 = { min = -7, max = 22 } } }) $Display.Description("");
    }
}

struct Texture2DArray $AssetType $Display.Description("An array of two dimensional textures.")
{
    const AssetTypeName :String = "he.asset.texture2d_array";

    textures @0 :he.schema.Uuid[] $Display.Asset(Texture2D.AssetTypeName);
}
