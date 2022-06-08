// Copyright Chad Engler

@0xc6d3b56c92a52ec3;

import "he/schema/schema.hsc";

namespace he.assets;

enum AssetState
{
    Unknown @0;
    NeedsImport @1;
    NeedsCompile @2;
    Ready @3;

    // Failure states
    ImportFailed @4;
    CompileFailed @5;
}

struct AssetTypeId
{
    name @0 :String;
    value @1 :uint32;
}

struct Asset
{
    uuid @0 :he.schema.Uuid;            // unique identifier of the asset
    type @1 :AssetTypeId;               // unique string identifier of the asset type
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
