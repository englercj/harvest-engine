-- Copyright Chad Engler

include "scripts/_setup.lua"

he_workspace(sln_name)
    startproject "editor"

    group "contrib"
        import_modules {
            "contrib/fmt",
        }

    group "engine"
        import_modules {
            "engine/core",
        }

    group "tests"
        import_all_module_tests()
        import_modules {
            "engine/test_runner",
        }
