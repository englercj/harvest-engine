-- Copyright Chad Engler

return {
    name = "fmt",
    kind = "StaticLib",
    github = "fmtlib/fmt#8.0.1",

    when_included = function ()
        includedirs { "include" }
    end,

    when_linked = function ()
        links { "fmt" }
    end,

    module_project = function ()
        include_modules { "fmt" }

        files {
            path.join("include/core.h"),
            path.join("include/compile.h"),
            path.join("include/format.h"),
            path.join("include/xchar.h"),
            path.join("src/format.cc"),
        }
    end,
}
