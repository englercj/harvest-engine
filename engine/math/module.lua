return {
    name = "he_math",
    kind = "StaticLib",

    when_included = function ()
        includedirs { "include" }
        include_modules { "he_core" }
    end,

    when_linked = function ()
        link_modules { "he_core" }
    end,

    module_project = function ()
        includedirs { "src" }
        include_modules { "he_math" }

        files { "include/**", "src/**" }
    end,

    test_project = function ()
        includedirs { "src" }
        include_modules { "he_math" }

        files { "test/**" }
    end,
}
