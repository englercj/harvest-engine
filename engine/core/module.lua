-- Copyright Chad Engler

return {
    name = "he_core",
    kind = "StaticLib",

    when_included = function ()
        includedirs { "include" }
        include_modules { "fmt" }
    end,

    when_linked = function ()
        links { "he_core" }
        link_modules { "fmt" }
    end,

    module_project = function ()
        includedirs { "src" }
        include_modules { "he_core" }

        files { "include/**", "src/**" }

        platform_excludes()
    end,

    test_project = function ()
        includedirs { "src" }
        include_modules { "he_core" }

        files { "test/**" }
    end,
}
