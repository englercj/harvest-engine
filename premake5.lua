-- Copyright Chad Engler

include "scripts/_setup.lua"

he_workspace()
    startproject "editor"

    import_plugins {
        "contrib/fmt",
        "engine/core",
        "engine/math",
        "engine/window",
    }
