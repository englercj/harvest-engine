// Copyright Chad Engler

@0xea67347ea000c22e;

import "editor_attributes.hsc";

namespace he.editor.schema;

// A project that has been recently opened
struct RecentProject
{
    name @0 :String;                    // Name of the project.
    path @1 :String;                    // Absolute path to the project file.
    openTime @2 :uint64;                // Harvest timestamp when this project was last opened.
    recentAssets @3 :List<uint8[16]>;
}

// Setting that are local to the user
struct Settings
{
    recentProjects @0 :List<RecentProject> $EditorHidden;
}
