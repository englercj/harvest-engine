// Copyright Chad Engler

@0xc6d3b56c92a52ec3;

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

struct Asset
{
    id @0 :uint8[16];               // unique identifier of the asset
    type @1 :String;                // unique string identifier of the asset type
    name @2 :String;                // user-defined human-friendly name
    tags @3 :List<String>;          // user-defined search & filter strings
    sources @4 :List<String>;       // relative path to source file(s)
    references @5 :List<uint8[16]>; // outgoing references to other assets
    importData @6 :AnyPointer;      // Importer can place any structure it wants in this space
}

struct AssetFile
{
    id @0 :uint8[16];       // unique identifier of the file
    assets @1 :List<Asset>; // list of assets contained in the file
}
