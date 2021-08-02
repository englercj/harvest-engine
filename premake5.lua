-- Copyright Chad Engler

include "scripts/_setup.lua"

he_workspace(sln_name)
    startproject "editor"

    group "contrib"
        import_plugins {
            "contrib/fmt",
        }

    group "engine"
        import_plugins {
            "engine/core",
            "engine/math",
        }

    group "tests"
        create_all_module_test_projects()
        import_plugins { "engine/test_runner" }
