# Copyright Chad Engler

@0x9d7f21020ee32dce;

using Cxx = import "/capnp/c++.capnp";
using Json = import "/capnp/compat/json.capnp";

$Cxx.namespace("he::editor::schema");

struct Tester
{
    a @0 :Int8 = 1;
}

# A project file that can be opened by the editor
struct Project
{
    id0 @0 :Int64;  # unique identifier for the project
    id1 @1 :Int64;  # unique identifier for the project
    name @2 :Text;          # Human-friendly name
    assetRoot @3 :Text;     # Relative path to the root of the project's asset data
    derp @4 :Int8 = 2;
    herp @5 :Void;
    lerp @6 :Tester = (a = 2);

    test :union
    {
        a @7 :Int64;
        b @8 :Text;
        c @9 :List(UInt8);
        d @10 :UInt32;
    }
}
