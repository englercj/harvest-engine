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

enum MessageSource
{
    Unknown @0;
    Importer @1;
    Compiler @2;
    System @3;
    User @4;
}

struct AssetId
{
    value @0 :uint8[16] $Toml.Hex;
}

struct AssetTypeId
{
    name @0 :String;
    value @1 :uint32;
}

struct Asset
{
    id @0 :AssetId;             // unique identifier of the asset
    type @1 :AssetTypeId;       // unique string identifier of the asset type
    name @2 :String;            // user-defined human-friendly name
    tags @3 :String[];          // user-defined search & filter strings
    sources @4 :String[];       // relative path to source file(s)
    references @5 :AssetId[];   // outgoing references to other assets
    importData @6 :Blob;        // Importer can place any data it wants in this space
}

struct AssetFileId
{
    value @0 :uint8[16] $Toml.Hex;
}

struct AssetFile
{
    id @0 :AssetFileId;     // unique identifier of the file
    assets @1 :Asset[];     // list of assets contained in the file
}
