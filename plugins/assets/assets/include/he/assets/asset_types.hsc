// Copyright Chad Engler

@0xc6d3b56c92a52ec3;

import "he/schema/schema.hsc";
import "he/editor/schema/editor_attributes.hsc";

namespace he.assets;

alias Display = he.editor.Display;

// ------------------------------------------------------------------------------------------------
// Base Asset Structures

// Marks a structure as the definition for an asset type.
// That structure is expected to have a constant named `AssetTypeName` that is a unique
// string that will be used as the asset type identifier.
attribute AssetType(struct) :void;

// Marks a field as referring to an asset of a particular type.
// Only meaningful for `he.schema.Uuid` fields.
attribute AssetRef(field) :String;

// A reference held by an asset to another object: either an asset, or a file.
struct AssetReference
{
    data :union
    {
        asset @0 :he.schema.Uuid;
        file @1 :String;
    }
}

// A source editable asset object in the Harvest asset system.
struct Asset
{
    uuid @0 :he.schema.Uuid $Display.ReadOnly;          // unique identifier of the asset
    type @1 :String $Display.ReadOnly;                  // unique string identifier of the asset type
    name @2 :String;                                    // user-defined human-friendly name
    tags @3 :String[];                                  // user-defined search & filter strings
    references @4 :AssetReference[] $Display.Hidden;    // outgoing references to other assets and data files
    data @5 :AnyStruct $Display.ShowOnlyChildren;       // Data for the asset, structure type is deduced based on the value of the `type` member above
    importData @6 :AnyStruct $Display.Hidden;           // Any data the importer would like to attach to the asset
}

// The asset file structure that is stored in *.he_asset files
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
    const AssetTypeIcon :String = "\xf3\xb0\x8b\xa9"; // ICON_MDI_IMAGE
    const PixelsResourceName :String = "he.asset.texture2d.pixels";
    const Ktx2ResourceName :String = "he.asset.texture2d.ktx2";

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
        scale @3 :float32 = 1.0 $Display.Clamp({ data = { float = { min = 0.000125, max = 4.0 } } }) $Display.Description("Set mipmap filter kernel's scale, lower=sharper, higher=more blurry");
        filter @4 :MipMapFilter = MipMapFilter.Kaiser $Display.Description("The filtering mechanism to use when generating mips.");
        sRGB @5 :bool = false $Display.Name("sRGB") $Display.Description("Convert image to linear before filtering, then back to sRGB.");
        renormalize @6 :bool = false $Display.Description("Renormalize normal map to unit length vectors after filtering.");
        clamp @7 :bool = false $Display.Description("Use clamp addressing on borders, instead of wrapping.");
        sampleFirst @8 :bool = false $Display.Description("Always first/largest mip level when generating a mip. Results in slower mip map generation.");
        smallestDimension @9 :int32 = 1 $Display.Clamp({ data = { int = { min = 1, max = 16384 } } }) $Display.Description("Set smallest pixel dimension for generated mipmaps.");
    }

    compression :group
    {
        format @10 :CompressionFormat = CompressionFormat.ETC1S;
        quality @11 :CompressionQuality = CompressionQuality.Normal;
        level @12 :int8 = 12 $Display.Clamp({ data = { int = { min = -7, max = 22 } } }) $Display.Description("");
    }
}

struct Texture2DArray $AssetType $Display.Description("An array of two dimensional textures.")
{
    const AssetTypeName :String = "he.asset.texture2d_array";

    textures @0 :he.schema.Uuid[] $AssetRef(Texture2D.AssetTypeName);
}
