// Copyright Chad Engler

@0x9d7f21020ee32dce;

import "he/editor/schema/display_attributes.hsc";
import "he/schema/schema.hsc";

namespace he.editor;

// A project file that can be opened by the editor
struct Project
{
    id @0 :he.schema.Uuid $Display.ReadOnly;                // unique identifier of the project
    name @1 :String;                                        // user-defined human-friendly name
    plugins @2 :String[] $Toml.Name("import_plugins");      // plugins this project imports
}
