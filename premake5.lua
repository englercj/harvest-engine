-- Copyright Chad Engler

include "scripts/_setup.lua"

he_workspace(sln_name)
    startproject "editor"

    import_plugins {
        "contrib/fmt",
        "engine/core",
        "engine/math",
        "engine/test_runner",
        "engine/window",
    }
