// Copyright Chad Engler

@0xc6d3b56c92a52ec3;

import "he/schema/schema.hsc";

namespace he.assets.schema;

// ------------------------------------------------------------------------------------------------
// Asset Attributes

attribute AssetType(struct): String;

// ------------------------------------------------------------------------------------------------
// Base Asset Structures

struct Asset
{
    uuid @0 :he.schema.Uuid;            // unique identifier of the asset
    type @1 :String;                    // unique string identifier of the asset type
    name @2 :String;                    // user-defined human-friendly name
    tags @3 :String[];                  // user-defined search & filter strings
    references @4 :he.schema.Uuid[];    // outgoing references to other assets
    importData @5 :Blob;                // Importer can place any data it wants in this space
}

struct AssetFile
{
    uuid @0 :he.schema.Uuid;    // unique identifier of the file
    assets @1 :Asset[];         // list of assets contained in the file
    source @2 :String;          // relative path to source file for the assets
}

// ------------------------------------------------------------------------------------------------
// Built-in Asset Types

struct Texture $AssetType("he.asset.texture")
{
}
