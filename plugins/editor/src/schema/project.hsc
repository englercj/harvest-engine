// Copyright Chad Engler

@0x9d7f21020ee32dce;

namespace he.editor.schema;

struct Tester
{
    a @0 :int8 = 1;
}

// A project file that can be opened by the editor
struct Project
{
    id @0 :uint8[16];       // Unique identifier for the project
    name @1 :String;        // Human-friendly name
    assetRoot @2 :String;   // Relative path to the root of the project's asset data
    derp @3 :int8 = 2;
    herp @4 :void;
    lerp @5 :Tester = { a = 2 };

    test :union
    {
        a @6 :int64;
        b @7 :String;
        c @8 :List<uint8>;
        d @9 :uint32;
    }
}
