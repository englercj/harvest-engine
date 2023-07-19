// Copyright Chad Engler

@0x9d7f21020ee32dce;

namespace he.editor;

// A project file that can be opened by the editor
struct Project
{
    name @0 :String;                // Human-friendly name
    import_plugins @1 :String[];    // List of plugins this project imports
}
