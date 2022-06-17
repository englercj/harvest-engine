// Copyright Chad Engler

@0x9d7f21020ee32dce;

import "he/schema/schema.hsc";

namespace he.editor.schema;

// A project file that can be opened by the editor
struct Project
{
    id @0 :he.schema.Uuid;  // Unique identifier for the project
    name @1 :String;        // Human-friendly name
    assetRoot @2 :String;   // Relative path to the root of the project's asset data
}
