# Copyright Chad Engler

@0x9d7f21020ee32dce;

using Cxx = import "/capnp/c++.capnp";

$Cxx.namespace("he::editor::schema");

struct ProjectId {
    # A 16-byte UUID for a project

    x0 @0 :UInt64;
    x1 @1 :UInt64;
}

struct Project {
    # A project file that can be opened by the editor

    id @0 :ProjectId;   # UUID
    name @1 :Text;      # Human-friendly name
    assetRoot @2 :Text; # Relative path to the root of the project's asset data
}
