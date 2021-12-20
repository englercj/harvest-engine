# Copyright Chad Engler

@0xc6d3b56c92a52ec3;

using Cxx = import "/capnp/c++.capnp";
using Json = import "/capnp/compat/json.capnp";

$Cxx.namespace("he::assets");

enum AssetState $Test, $Two(a = 10), $Three
enum AssetState [Test, Two(a = 10), Three]
{
    unknown @0;
    needsImport @1;
    needsCompile @2;
    ready @3;

    # Failure states
    importFailed @4;
    compileFailed @5;
}

enum MessageSource
{
    unknown @0;
    importer @1;
    compiler @2;
    system @3;
    user @4;
}

# struct FileRef
# {
#     relPath @0 :Text;
#     lastModifiedTime @1 :Uint64;
#     lastSize @2 :Uint64;
# }

struct Asset
{
    id @0 :Data $Json.hex;          # unique identifier of the asset
    type @1 :Text;                  # unique string identifier of the asset type
    name @2 :Text;                  # user-defined human-friendly name
    tags @3 :List(Text);            # user-defined search & filter strings
    sources @4 :List(Text);         # relative path to source file(s)
    references @5 :List(Data) $Json.hex; # outgoing references to other assets
    importData @6 :AnyPointer;      # blob for importer storage
}

struct AssetFile
{
    id @0 :Data $Json.hex;      # unique identifier of the file
    assets @1 :List(Asset);     # list of assets contained in the file
}
