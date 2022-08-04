// Copyright Chad Engler

@0xc6d3b56c92a52ec3;

import "he/schema/schema.hsc";

namespace he.assets.schema;

// ------------------------------------------------------------------------------------------------
// Display Attributes

// TODO: This should probably live somewhere else? It isn't asset-specific.
// It can't live in the editor though, because we need it here and can't depend on editor...

struct Display
{
    attribute ImportOnly(struct) :void;

    // Hides the field from display. The data is still persisted and available to code.
    // The editor property grids will skip fields with this attribute.
    attribute Hidden(field) :void;

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

    // Sets a minimum value that can be selected in the UI for an integer value.
    attribute MinInt(field) :int64;

    // Sets a maximum value that can be selected in the UI for an integer value.
    attribute MaxInt(field) :int64;

    // Sets a minimum value that can be selected in the UI for an integer value.
    attribute MinUint(field) :uint64;

    // Sets a maximum value that can be selected in the UI for an integer value.
    attribute MaxUint(field) :uint64;

    // Sets a minimum value that can be selected in the UI for an integer value.
    attribute MinFloat(field) :float64;

    // Sets a maximum value that can be selected in the UI for an integer value.
    attribute MaxFloat(field) :float64;
}

// ------------------------------------------------------------------------------------------------
// Base Asset Structures

// Marks a structure as the definition for an asset type.
// That structure is expected to have a constant named `AssetTypeName` that is a unique
// string that will be used as the asset type identifier.
attribute AssetType(struct) :void;

struct Asset
{
    uuid @0 :he.schema.Uuid;            // unique identifier of the asset
    type @1 :String;                    // unique string identifier of the asset type
    name @2 :String;                    // user-defined human-friendly name
    tags @3 :String[];                  // user-defined search & filter strings
    references @4 :he.schema.Uuid[];    // outgoing references to other assets
    data @5 :AnyStruct;                 // Data for the asset, type is deduced based on `type`
    importData @6 :AnyStruct;           // Data for the importer, stores import settings for re-importing
}

struct AssetFile
{
    uuid @0 :he.schema.Uuid;    // unique identifier of the file
    assets @1 :Asset[];         // list of assets contained in the file
    source @2 :String;          // relative path to source file for the assets
}

// ------------------------------------------------------------------------------------------------
// Built-in Asset Types

struct Texture2D $AssetType $Display.Description("A two dimensional texture.")
{
    const AssetTypeName :String = "he.asset.texture2d";
    const PixelsResourceName :String = "he.asset.texture.pixels";
    const Ktx2ResourceName :String = "he.asset.texture.ktx2";

    struct CompressionSettings
    {
        enum Format
        {
            ETC1S @0 $Display.Description("Highly compressed, similar to BC1/4.");
            UASTC @1 $Display.Description("Extremely high quality, similar to BC7.");
        }

        enum Quality
        {
            VeryLow @0;
            Low @1;
            Normal @2 $Display.Description("Balanced choice between compressed size and quality.");
            High @3;
            VeryHigh @4;
        }

        format @0 :Format = Format.ETC1S;
        quality @1 :Quality = Quality.Normal;
        level @2 :int8 = 12 $Display.MinInt(-7) $Display.MaxInt(22);
    }

    struct MipMapSettings
    {
        enum Filter
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

        generate @0 :bool = true $Display.Description("Enables automatic generation of mips.");
        scale @1 :float32 = 1.0 $Display.MinFloat(0.000125) $Display.MaxFloat(4.0) $Display.Description("Set mipmap filter kernel's scale, lower=sharper, higher=more blurry");
        filter @2 :Filter = Filter.Kaiser $Display.Description("The filtering mechanism to use when generating mips.");
        sRGB @3 :bool = false $Display.Description("Convert image to linear before filtering, then back to sRGB.");
        premultiply @4 :bool = true $Display.Description("");
        renormalize @5 :bool = false $Display.Description("Renormalize normal map to unit length vectors after filtering.");
        clamp @6 :bool = false $Display.Description("Use clamp addressing on borders, instead of wrapping.");
        fast @7 :bool = true $Display.Description("Use faster mipmap generation by resampling from previous mip, not always first/largest mip level.");
        smallestDimension @8 :int32 = 1 $Display.MinInt(1) $Display.MaxInt(16384) $Display.Description("Set smallest pixel dimension for generated mipmaps.");
    }

    struct PixelsResource
    {
        width @0 :uint32;
        height @1 :uint32;
        source @2 :Blob;
        mips @3 :Blob[];
    }

    flipY @0 :bool = false $Display.Description("When true the imported image is flipped along the Y axis.");
    sRGB @1 :bool = true $Display.Description("When true the sRGB color space is used, otherwise linear space is used.");
    mipMapping @2 :MipMapSettings;
    compression @3 :CompressionSettings;
}

struct Texture2DArray $AssetType $Display.Description("An array of two dimensional textures.")
{
    const AssetTypeName :String = "he.asset.texture2d_array";

    textures @0 :he.schema.Uuid[] $Display.Asset(Texture2D.AssetTypeName);
}
