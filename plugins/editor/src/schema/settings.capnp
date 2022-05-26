# Copyright Chad Engler

@0xea67347ea000c22e;

using Cxx = import "/capnp/c++.capnp";
using EditorOptions = import "editor_options.capnp";

$Cxx.namespace("he::editor::schema");

# A project that has been recently opened
struct RecentProject
{
    name @0 :Text = "test";              # Name of the project.
    path @1 :Text;              # Absolute path to the project file.
    openTime @2 :UInt64;        # Harvest timestamp when this project was last opened.
    recentAssets @3 :List(Data);
}

# Setting that are local to the user
struct Settings
{
    recentProjects @0 :List(RecentProject) $EditorOptions.hidden;
}
