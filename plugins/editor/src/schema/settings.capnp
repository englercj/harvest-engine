# Copyright Chad Engler

@0xea67347ea000c22e;

using Cxx = import "/capnp/c++.capnp";
using EditorOptions = import "editor_options.capnp";

$Cxx.namespace("he::editor::schema");

struct AssetId {
    # A 16-byte UUID for an asset

    x0 @0 :UInt64;
    x1 @1 :UInt64;
}

struct RecentProject {
    # A project that has been recently opened

    name @0 :Text;          # Name of the project.
    path @1 :Text;          # Absolute path to the project file.
    openTime @2 :UInt64;    # Harvest timestamp when this project was last opened.
    recentAssets @3 :List(AssetId);
}

struct Settings {
    # Setting that are local to the user

    recentProjects @0 :List(RecentProject) $EditorOptions.hidden;
}
